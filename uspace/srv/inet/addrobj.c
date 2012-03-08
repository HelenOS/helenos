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

#include <bitops.h>
#include <errno.h>
#include <fibril_synch.h>
#include <io/log.h>
#include <ipc/loc.h>
#include <stdlib.h>

#include "addrobj.h"
#include "inet.h"
#include "inet_link.h"

static FIBRIL_MUTEX_INITIALIZE(addr_list_lock);
static LIST_INITIALIZE(addr_list);
static sysarg_t addr_id = 0;

static uint32_t inet_netmask(int bits)
{
	assert(bits >= 1);
	assert(bits < 32);

	return BIT_RANGE(uint32_t, 31, 31 - (bits - 1));
}

inet_addrobj_t *inet_addrobj_new(void)
{
	inet_addrobj_t *addr = calloc(1, sizeof(inet_addrobj_t));

	if (addr == NULL) {
		log_msg(LVL_ERROR, "Failed allocating address object. "
		    "Out of memory.");
		return NULL;
	}

	link_initialize(&addr->addr_list);
	fibril_mutex_lock(&addr_list_lock);
	addr->id = ++addr_id;
	fibril_mutex_unlock(&addr_list_lock);

	return addr;
}

void inet_addrobj_delete(inet_addrobj_t *addr)
{
	if (addr->name != NULL)
		free(addr->name);
	free(addr);
}

void inet_addrobj_add(inet_addrobj_t *addr)
{
	fibril_mutex_lock(&addr_list_lock);
	list_append(&addr->addr_list, &addr_list);
	fibril_mutex_unlock(&addr_list_lock);
}

void inet_addrobj_remove(inet_addrobj_t *addr)
{
	fibril_mutex_lock(&addr_list_lock);
	list_remove(&addr->addr_list);
	fibril_mutex_unlock(&addr_list_lock);
}

/** Find address object matching address @a addr.
 *
 * @param addr	Address
 * @oaram find	iaf_net to find network (using mask),
 *		iaf_addr to find local address (exact match)
 */
inet_addrobj_t *inet_addrobj_find(inet_addr_t *addr, inet_addrobj_find_t find)
{
	uint32_t mask;

	log_msg(LVL_DEBUG, "inet_addrobj_find(%x)", (unsigned)addr->ipv4);

	fibril_mutex_lock(&addr_list_lock);

	list_foreach(addr_list, link) {
		inet_addrobj_t *naddr = list_get_instance(link,
		    inet_addrobj_t, addr_list);

		mask = inet_netmask(naddr->naddr.bits);
		if ((naddr->naddr.ipv4 & mask) == (addr->ipv4 & mask)) {
			fibril_mutex_unlock(&addr_list_lock);
			log_msg(LVL_DEBUG, "inet_addrobj_find: found %p",
			    addr);
			return naddr;
		}
	}

	log_msg(LVL_DEBUG, "inet_addrobj_find: Not found");
	fibril_mutex_unlock(&addr_list_lock);

	return NULL;
}

/** Find address object matching address @a addr.
 *
 * @param id	Address object ID
 * @return	Address object
 */
inet_addrobj_t *inet_addrobj_get_by_id(sysarg_t id)
{
	log_msg(LVL_DEBUG, "inet_addrobj_get_by_id(%zu)", (size_t)id);

	fibril_mutex_lock(&addr_list_lock);

	list_foreach(addr_list, link) {
		inet_addrobj_t *naddr = list_get_instance(link,
		    inet_addrobj_t, addr_list);

		if (naddr->id == id) {
			fibril_mutex_unlock(&addr_list_lock);
			return naddr;
		}
	}

	fibril_mutex_unlock(&addr_list_lock);

	return NULL;
}

/** Send datagram to directly reachable destination */
int inet_addrobj_send_dgram(inet_addrobj_t *addr, inet_dgram_t *dgram,
    uint8_t proto, uint8_t ttl, int df)
{
	inet_addr_t lsrc_addr;
	inet_addr_t *ldest_addr;

	lsrc_addr.ipv4 = addr->naddr.ipv4;
	ldest_addr = &dgram->dest;

	return inet_link_send_dgram(addr->ilink, &lsrc_addr, ldest_addr, dgram,
	    proto, ttl, df);
}

/** Get IDs of all address objects. */
int inet_addrobj_get_id_list(sysarg_t **rid_list, size_t *rcount)
{
	sysarg_t *id_list;
	size_t count, i;

	fibril_mutex_lock(&addr_list_lock);
	count = list_count(&addr_list);

	id_list = calloc(count, sizeof(sysarg_t));
	if (id_list == NULL) {
		fibril_mutex_unlock(&addr_list_lock);
		return ENOMEM;
	}

	i = 0;
	list_foreach(addr_list, link) {
		inet_addrobj_t *addr = list_get_instance(link,
		    inet_addrobj_t, addr_list);

		id_list[i++] = addr->id;
	}

	fibril_mutex_unlock(&addr_list_lock);

	*rid_list = id_list;
	*rcount = count;

	return EOK;
}

/** @}
 */
