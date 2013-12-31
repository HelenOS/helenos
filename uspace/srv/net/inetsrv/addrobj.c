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
#include <str.h>
#include "addrobj.h"
#include "inetsrv.h"
#include "inet_link.h"
#include "ndp.h"

static inet_addrobj_t *inet_addrobj_find_by_name_locked(const char *, inet_link_t *);

static FIBRIL_MUTEX_INITIALIZE(addr_list_lock);
static LIST_INITIALIZE(addr_list);
static sysarg_t addr_id = 0;

inet_addrobj_t *inet_addrobj_new(void)
{
	inet_addrobj_t *addr = calloc(1, sizeof(inet_addrobj_t));

	if (addr == NULL) {
		log_msg(LOG_DEFAULT, LVL_ERROR, "Failed allocating address object. "
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

int inet_addrobj_add(inet_addrobj_t *addr)
{
	inet_addrobj_t *aobj;

	fibril_mutex_lock(&addr_list_lock);
	aobj = inet_addrobj_find_by_name_locked(addr->name, addr->ilink);
	if (aobj != NULL) {
		/* Duplicate address name */
		fibril_mutex_unlock(&addr_list_lock);
		return EEXISTS;
	}

	list_append(&addr->addr_list, &addr_list);
	fibril_mutex_unlock(&addr_list_lock);

	return EOK;
}

void inet_addrobj_remove(inet_addrobj_t *addr)
{
	fibril_mutex_lock(&addr_list_lock);
	list_remove(&addr->addr_list);
	fibril_mutex_unlock(&addr_list_lock);
}

/** Find address object matching address @a addr.
 *
 * @param addr Address
 * @oaram find iaf_net to find network (using mask),
 *             iaf_addr to find local address (exact match)
 *
 */
inet_addrobj_t *inet_addrobj_find(inet_addr_t *addr, inet_addrobj_find_t find)
{
	fibril_mutex_lock(&addr_list_lock);
	
	list_foreach(addr_list, addr_list, inet_addrobj_t, naddr) {
		switch (find) {
		case iaf_net:
			if (inet_naddr_compare_mask(&naddr->naddr, addr)) {
				fibril_mutex_unlock(&addr_list_lock);
				log_msg(LOG_DEFAULT, LVL_DEBUG, "inet_addrobj_find: found %p",
				    naddr);
				return naddr;
			}
			break;
		case iaf_addr:
			if (inet_naddr_compare(&naddr->naddr, addr)) {
				fibril_mutex_unlock(&addr_list_lock);
				log_msg(LOG_DEFAULT, LVL_DEBUG, "inet_addrobj_find: found %p",
				    naddr);
				return naddr;
			}
			break;
		}
	}
	
	log_msg(LOG_DEFAULT, LVL_DEBUG, "inet_addrobj_find: Not found");
	fibril_mutex_unlock(&addr_list_lock);
	
	return NULL;
}

/** Find address object on a link, with a specific name.
 *
 * @param name	Address object name
 * @param ilink	Inet link
 * @return	Address object
 */
static inet_addrobj_t *inet_addrobj_find_by_name_locked(const char *name, inet_link_t *ilink)
{
	assert(fibril_mutex_is_locked(&addr_list_lock));

	log_msg(LOG_DEFAULT, LVL_DEBUG, "inet_addrobj_find_by_name_locked('%s', '%s')",
	    name, ilink->svc_name);

	list_foreach(addr_list, addr_list, inet_addrobj_t, naddr) {
		if (naddr->ilink == ilink && str_cmp(naddr->name, name) == 0) {
			log_msg(LOG_DEFAULT, LVL_DEBUG, "inet_addrobj_find_by_name_locked: found %p",
			    naddr);
			return naddr;
		}
	}

	log_msg(LOG_DEFAULT, LVL_DEBUG, "inet_addrobj_find_by_name_locked: Not found");

	return NULL;
}


/** Find address object on a link, with a specific name.
 *
 * @param name	Address object name
 * @param ilink	Inet link
 * @return	Address object
 */
inet_addrobj_t *inet_addrobj_find_by_name(const char *name, inet_link_t *ilink)
{
	inet_addrobj_t *aobj;

	log_msg(LOG_DEFAULT, LVL_DEBUG, "inet_addrobj_find_by_name('%s', '%s')",
	    name, ilink->svc_name);

	fibril_mutex_lock(&addr_list_lock);
	aobj = inet_addrobj_find_by_name_locked(name, ilink);
	fibril_mutex_unlock(&addr_list_lock);

	return aobj;
}

/** Find address object with the given ID.
 *
 * @param id	Address object ID
 * @return	Address object
 */
inet_addrobj_t *inet_addrobj_get_by_id(sysarg_t id)
{
	log_msg(LOG_DEFAULT, LVL_DEBUG, "inet_addrobj_get_by_id(%zu)", (size_t)id);

	fibril_mutex_lock(&addr_list_lock);

	list_foreach(addr_list, addr_list, inet_addrobj_t, naddr) {
		if (naddr->id == id) {
			fibril_mutex_unlock(&addr_list_lock);
			return naddr;
		}
	}

	fibril_mutex_unlock(&addr_list_lock);

	return NULL;
}

/** Send datagram from address object */
int inet_addrobj_send_dgram(inet_addrobj_t *addr, inet_addr_t *ldest,
    inet_dgram_t *dgram, uint8_t proto, uint8_t ttl, int df)
{
	inet_addr_t lsrc_addr;
	inet_naddr_addr(&addr->naddr, &lsrc_addr);

	addr32_t lsrc_v4;
	addr128_t lsrc_v6;
	ip_ver_t lsrc_ver = inet_addr_get(&lsrc_addr, &lsrc_v4, &lsrc_v6);

	addr32_t ldest_v4;
	addr128_t ldest_v6;
	ip_ver_t ldest_ver = inet_addr_get(ldest, &ldest_v4, &ldest_v6);

	if (lsrc_ver != ldest_ver)
		return EINVAL;

	int rc;
	addr48_t ldest_mac;

	switch (ldest_ver) {
	case ip_v4:
		return inet_link_send_dgram(addr->ilink, lsrc_v4, ldest_v4,
		    dgram, proto, ttl, df);
	case ip_v6:
		/*
		 * Translate local destination IPv6 address.
		 */
		rc = ndp_translate(lsrc_v6, ldest_v6, ldest_mac, addr->ilink);
		if (rc != EOK)
			return rc;

		return inet_link_send_dgram6(addr->ilink, ldest_mac, dgram,
		    proto, ttl, df);
	default:
		assert(false);
		break;
	}

	return ENOTSUP;
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
	list_foreach(addr_list, addr_list, inet_addrobj_t, addr) {
		id_list[i++] = addr->id;
	}

	fibril_mutex_unlock(&addr_list_lock);

	*rid_list = id_list;
	*rcount = count;

	return EOK;
}

/** @}
 */
