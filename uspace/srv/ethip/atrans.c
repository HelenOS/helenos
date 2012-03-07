/*
 * Copyright (c) 2012 Jiri Svoboda
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * - Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 * - Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the distribution.
 * - The name of the author may not be used to endorse or promote products
 *   derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
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
#include <inet/iplink_srv.h>
#include <stdlib.h>

#include "atrans.h"
#include "ethip.h"

/** Address translation list (of ethip_atrans_t) */
static FIBRIL_MUTEX_INITIALIZE(atrans_list_lock);
static LIST_INITIALIZE(atrans_list);
static FIBRIL_CONDVAR_INITIALIZE(atrans_cv);

static ethip_atrans_t *atrans_find(iplink_srv_addr_t *ip_addr)
{
	list_foreach(atrans_list, link) {
		ethip_atrans_t *atrans = list_get_instance(link,
		    ethip_atrans_t, atrans_list);

		if (atrans->ip_addr.ipv4 == ip_addr->ipv4)
			return atrans;
	}

	return NULL;
}

int atrans_add(iplink_srv_addr_t *ip_addr, mac48_addr_t *mac_addr)
{
	ethip_atrans_t *atrans;
	ethip_atrans_t *prev;

	atrans = calloc(1, sizeof(ethip_atrans_t));
	if (atrans == NULL)
		return ENOMEM;

	atrans->ip_addr = *ip_addr;
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

int atrans_remove(iplink_srv_addr_t *ip_addr)
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

int atrans_lookup(iplink_srv_addr_t *ip_addr, mac48_addr_t *mac_addr)
{
	ethip_atrans_t *atrans;

	fibril_mutex_lock(&atrans_list_lock);
	atrans = atrans_find(ip_addr);
	if (atrans == NULL) {
		fibril_mutex_unlock(&atrans_list_lock);
		return ENOENT;
	}

	fibril_mutex_unlock(&atrans_list_lock);
	*mac_addr = atrans->mac_addr;
	return EOK;
}

int atrans_wait_timeout(suseconds_t timeout)
{
	int rc;

	fibril_mutex_lock(&atrans_list_lock);
	rc = fibril_condvar_wait_timeout(&atrans_cv, &atrans_list_lock,
	    timeout);
	fibril_mutex_unlock(&atrans_list_lock);

	return rc;
}

/** @}
 */
