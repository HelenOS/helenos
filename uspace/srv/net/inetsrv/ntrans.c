/*
 * Copyright (c) 2021 Jiri Svoboda
 * Copyright (c) 2013 Antonin Steinhauser
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
#include <inet/eth_addr.h>
#include <inet/iplink_srv.h>
#include <stdlib.h>
#include "ntrans.h"

/** Address translation list (of inet_ntrans_t) */
static FIBRIL_MUTEX_INITIALIZE(ntrans_list_lock);
static LIST_INITIALIZE(ntrans_list);
static FIBRIL_CONDVAR_INITIALIZE(ntrans_cv);

/** Look for address in translation table
 *
 * @param ip_addr IPv6 address
 *
 * @return inet_ntrans_t with the address on success
 * @return NULL if nothing found
 */
static inet_ntrans_t *ntrans_find(addr128_t ip_addr)
{
	list_foreach(ntrans_list, ntrans_list, inet_ntrans_t, ntrans) {
		if (addr128_compare(ntrans->ip_addr, ip_addr))
			return ntrans;
	}

	return NULL;
}

/** Add entry to translation table
 *
 * @param ip_addr  IPv6 address of the new entry
 * @param mac_addr MAC address of the new entry
 *
 * @return EOK on success
 * @return ENOMEM if not enough memory
 *
 */
errno_t ntrans_add(addr128_t ip_addr, eth_addr_t *mac_addr)
{
	inet_ntrans_t *ntrans;
	inet_ntrans_t *prev;

	ntrans = calloc(1, sizeof(inet_ntrans_t));
	if (ntrans == NULL)
		return ENOMEM;

	addr128(ip_addr, ntrans->ip_addr);
	ntrans->mac_addr = *mac_addr;

	fibril_mutex_lock(&ntrans_list_lock);
	prev = ntrans_find(ip_addr);
	if (prev != NULL) {
		list_remove(&prev->ntrans_list);
		free(prev);
	}

	list_append(&ntrans->ntrans_list, &ntrans_list);
	fibril_mutex_unlock(&ntrans_list_lock);
	fibril_condvar_broadcast(&ntrans_cv);

	return EOK;
}

/** Remove entry from translation table
 *
 * @param ip_addr IPv6 address of the entry to be removed
 *
 * @return EOK on success
 * @return ENOENT when no such address found
 *
 */
errno_t ntrans_remove(addr128_t ip_addr)
{
	inet_ntrans_t *ntrans;

	fibril_mutex_lock(&ntrans_list_lock);
	ntrans = ntrans_find(ip_addr);
	if (ntrans == NULL) {
		fibril_mutex_unlock(&ntrans_list_lock);
		return ENOENT;
	}

	list_remove(&ntrans->ntrans_list);
	fibril_mutex_unlock(&ntrans_list_lock);
	free(ntrans);

	return EOK;
}

/** Translate IPv6 address to MAC address using the translation table
 *
 * @param ip_addr  IPv6 address to be translated
 * @param mac_addr MAC address to be assigned
 *
 * @return EOK on success
 * @return ENOENT when no such address found
 *
 */
errno_t ntrans_lookup(addr128_t ip_addr, eth_addr_t *mac_addr)
{
	fibril_mutex_lock(&ntrans_list_lock);
	inet_ntrans_t *ntrans = ntrans_find(ip_addr);
	if (ntrans == NULL) {
		fibril_mutex_unlock(&ntrans_list_lock);
		return ENOENT;
	}

	fibril_mutex_unlock(&ntrans_list_lock);
	*mac_addr = ntrans->mac_addr;
	return EOK;
}

/** Wait on translation table CV for some time
 *
 * @param timeout Timeout in microseconds
 *
 * @return EOK if woken up by another fibril
 * @return ETIMEOUT if timed out
 *
 */
errno_t ntrans_wait_timeout(usec_t timeout)
{
	fibril_mutex_lock(&ntrans_list_lock);
	errno_t rc = fibril_condvar_wait_timeout(&ntrans_cv, &ntrans_list_lock,
	    timeout);
	fibril_mutex_unlock(&ntrans_list_lock);

	return rc;
}

/** @}
 */
