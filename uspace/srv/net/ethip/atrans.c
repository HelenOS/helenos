/*
 * SPDX-FileCopyrightText: 2021 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup inet
 * @{
 */
/**
 * @file
 * @brief
 */

#include <adt/list.h>
#include <errno.h>
#include <fibril_synch.h>
#include <inet/eth_addr.h>
#include <inet/iplink_srv.h>
#include <stdlib.h>

#include "atrans.h"
#include "ethip.h"

/** Address translation list (of ethip_atrans_t) */
static FIBRIL_MUTEX_INITIALIZE(atrans_list_lock);
static LIST_INITIALIZE(atrans_list);
static FIBRIL_CONDVAR_INITIALIZE(atrans_cv);

static ethip_atrans_t *atrans_find(addr32_t ip_addr)
{
	list_foreach(atrans_list, atrans_list, ethip_atrans_t, atrans) {
		if (atrans->ip_addr == ip_addr)
			return atrans;
	}

	return NULL;
}

errno_t atrans_add(addr32_t ip_addr, eth_addr_t *mac_addr)
{
	ethip_atrans_t *atrans;
	ethip_atrans_t *prev;

	atrans = calloc(1, sizeof(ethip_atrans_t));
	if (atrans == NULL)
		return ENOMEM;

	atrans->ip_addr = ip_addr;
	atrans->mac_addr = *mac_addr;

	fibril_mutex_lock(&atrans_list_lock);
	prev = atrans_find(ip_addr);
	if (prev != NULL) {
		list_remove(&prev->atrans_list);
		free(prev);
	}

	list_append(&atrans->atrans_list, &atrans_list);
	fibril_mutex_unlock(&atrans_list_lock);
	fibril_condvar_broadcast(&atrans_cv);

	return EOK;
}

errno_t atrans_remove(addr32_t ip_addr)
{
	ethip_atrans_t *atrans;

	fibril_mutex_lock(&atrans_list_lock);
	atrans = atrans_find(ip_addr);
	if (atrans == NULL) {
		fibril_mutex_unlock(&atrans_list_lock);
		return ENOENT;
	}

	list_remove(&atrans->atrans_list);
	fibril_mutex_unlock(&atrans_list_lock);
	free(atrans);

	return EOK;
}

static errno_t atrans_lookup_locked(addr32_t ip_addr, eth_addr_t *mac_addr)
{
	ethip_atrans_t *atrans = atrans_find(ip_addr);
	if (atrans == NULL)
		return ENOENT;

	*mac_addr = atrans->mac_addr;
	return EOK;
}

errno_t atrans_lookup(addr32_t ip_addr, eth_addr_t *mac_addr)
{
	errno_t rc;

	fibril_mutex_lock(&atrans_list_lock);
	rc = atrans_lookup_locked(ip_addr, mac_addr);
	fibril_mutex_unlock(&atrans_list_lock);

	return rc;
}

static void atrans_lookup_timeout_handler(void *arg)
{
	bool *timedout = (bool *)arg;

	fibril_mutex_lock(&atrans_list_lock);
	*timedout = true;
	fibril_mutex_unlock(&atrans_list_lock);
	fibril_condvar_broadcast(&atrans_cv);
}

errno_t atrans_lookup_timeout(addr32_t ip_addr, usec_t timeout,
    eth_addr_t *mac_addr)
{
	fibril_timer_t *t;
	bool timedout;
	errno_t rc;

	t = fibril_timer_create(NULL);
	if (t == NULL)
		return ENOMEM;

	timedout = false;
	fibril_timer_set(t, timeout, atrans_lookup_timeout_handler, &timedout);

	fibril_mutex_lock(&atrans_list_lock);

	while ((rc = atrans_lookup_locked(ip_addr, mac_addr)) == ENOENT &&
	    !timedout) {
		fibril_condvar_wait(&atrans_cv, &atrans_list_lock);
	}

	fibril_mutex_unlock(&atrans_list_lock);
	(void) fibril_timer_clear(t);
	fibril_timer_destroy(t);

	return rc;
}

/** @}
 */
