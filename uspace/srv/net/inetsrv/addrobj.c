/*
 * Copyright (c) 2024 Jiri Svoboda
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
#include <inet/eth_addr.h>
#include <io/log.h>
#include <ipc/loc.h>
#include <sif.h>
#include <stdio.h>
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

errno_t inet_addrobj_add(inet_addrobj_t *addr)
{
	inet_addrobj_t *aobj;

	fibril_mutex_lock(&addr_list_lock);
	aobj = inet_addrobj_find_by_name_locked(addr->name, addr->ilink);
	if (aobj != NULL) {
		/* Duplicate address name */
		fibril_mutex_unlock(&addr_list_lock);
		return EEXIST;
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

/** Count number of non-temporary address objects configured for link.
 *
 * @param ilink Inet link
 * @return Number of address objects configured for this link
 */
unsigned inet_addrobj_cnt_by_link(inet_link_t *ilink)
{
	unsigned cnt;

	log_msg(LOG_DEFAULT, LVL_DEBUG, "inet_addrobj_cnt_by_link()");

	fibril_mutex_lock(&addr_list_lock);

	cnt = 0;
	list_foreach(addr_list, addr_list, inet_addrobj_t, naddr) {
		if (naddr->ilink == ilink && naddr->temp == false)
			++cnt;
	}

	fibril_mutex_unlock(&addr_list_lock);
	return cnt;
}

/** Send datagram from address object */
errno_t inet_addrobj_send_dgram(inet_addrobj_t *addr, inet_addr_t *ldest,
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

	errno_t rc;
	eth_addr_t ldest_mac;

	switch (ldest_ver) {
	case ip_v4:
		return inet_link_send_dgram(addr->ilink, lsrc_v4, ldest_v4,
		    dgram, proto, ttl, df);
	case ip_v6:
		/*
		 * Translate local destination IPv6 address.
		 */
		rc = ndp_translate(lsrc_v6, ldest_v6, &ldest_mac, addr->ilink);
		if (rc != EOK)
			return rc;

		return inet_link_send_dgram6(addr->ilink, &ldest_mac, dgram,
		    proto, ttl, df);
	default:
		assert(false);
		break;
	}

	return ENOTSUP;
}

/** Get IDs of all address objects. */
errno_t inet_addrobj_get_id_list(sysarg_t **rid_list, size_t *rcount)
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

/** Load address object from SIF node.
 *
 * @param anode SIF node to load address object from
 * @return EOK on success or an error code
 */
static errno_t inet_addrobj_load(sif_node_t *anode)
{
	errno_t rc;
	const char *sid;
	const char *snaddr;
	const char *slink;
	const char *name;
	char *endptr;
	int id;
	inet_naddr_t naddr;
	inet_addrobj_t *addr;
	inet_link_t *link;

	log_msg(LOG_DEFAULT, LVL_DEBUG, "inet_addrobj_load()");

	sid = sif_node_get_attr(anode, "id");
	if (sid == NULL)
		return EIO;

	snaddr = sif_node_get_attr(anode, "naddr");
	if (snaddr == NULL)
		return EIO;

	slink = sif_node_get_attr(anode, "link");
	if (slink == NULL)
		return EIO;

	name = sif_node_get_attr(anode, "name");
	if (name == NULL)
		return EIO;

	log_msg(LOG_DEFAULT, LVL_NOTE, "inet_addrobj_load(): id='%s' "
	    "naddr='%s' link='%s' name='%s'", sid, snaddr, slink, name);

	id = strtoul(sid, &endptr, 10);
	if (*endptr != '\0')
		return EIO;

	rc = inet_naddr_parse(snaddr, &naddr, NULL);
	if (rc != EOK)
		return EIO;

	link = inet_link_get_by_svc_name(slink);
	if (link == NULL) {
		log_msg(LOG_DEFAULT, LVL_ERROR, "Link '%s' not found",
		    slink);
		return EIO;
	}

	addr = inet_addrobj_new();
	if (addr == NULL)
		return ENOMEM;

	addr->id = id;
	addr->naddr = naddr;
	addr->ilink = link;
	addr->name = str_dup(name);

	if (addr->name == NULL) {
		inet_addrobj_delete(addr);
		return ENOMEM;
	}

	inet_addrobj_add(addr);
	return EOK;
}

/** Load address objects from SIF node.
 *
 * @param naddrs SIF node to load address objects from
 * @return EOK on success or an error code
 */
errno_t inet_addrobjs_load(sif_node_t *naddrs)
{
	sif_node_t *naddr;
	const char *ntype;
	errno_t rc;

	naddr = sif_node_first_child(naddrs);
	while (naddr != NULL) {
		ntype = sif_node_get_type(naddr);
		if (str_cmp(ntype, "address") != 0) {
			rc = EIO;
			goto error;
		}

		rc = inet_addrobj_load(naddr);
		if (rc != EOK)
			goto error;

		naddr = sif_node_next_child(naddr);
	}

	return EOK;
error:
	return rc;

}

/** Save address object to SIF node.
 *
 * @param addr Address object
 * @param naddr SIF node to save addres to
 * @return EOK on success or an error code
 */
static errno_t inet_addrobj_save(inet_addrobj_t *addr, sif_node_t *naddr)
{
	char *str = NULL;
	errno_t rc;
	int rv;

	log_msg(LOG_DEFAULT, LVL_DEBUG, "inet_addrobj_save(%p, %p)",
	    addr, naddr);

	/* id */

	rv = asprintf(&str, "%zu", addr->id);
	if (rv < 0) {
		str = NULL;
		rc = ENOMEM;
		goto error;
	}

	rc = sif_node_set_attr(naddr, "id", str);
	if (rc != EOK)
		goto error;

	free(str);
	str = NULL;

	/* dest */

	rc = inet_naddr_format(&addr->naddr, &str);
	if (rc != EOK)
		goto error;

	rc = sif_node_set_attr(naddr, "naddr", str);
	if (rc != EOK)
		goto error;

	free(str);
	str = NULL;

	/* link */

	rc = sif_node_set_attr(naddr, "link", addr->ilink->svc_name);
	if (rc != EOK)
		goto error;

	/* name */

	rc = sif_node_set_attr(naddr, "name", addr->name);
	if (rc != EOK)
		goto error;

	free(str);

	return rc;
error:
	if (str != NULL)
		free(str);
	return rc;
}

/** Save address objects to SIF node.
 *
 * @param cnode SIF node to save address objects to
 * @return EOK on success or an error code
 */
errno_t inet_addrobjs_save(sif_node_t *cnode)
{
	sif_node_t *naddr;
	errno_t rc;

	log_msg(LOG_DEFAULT, LVL_DEBUG, "inet_addrobjs_save()");

	fibril_mutex_lock(&addr_list_lock);

	list_foreach(addr_list, addr_list, inet_addrobj_t, addr) {
		if (addr->temp == false) {
			rc = sif_node_append_child(cnode, "address", &naddr);
			if (rc != EOK)
				goto error;

			rc = inet_addrobj_save(addr, naddr);
			if (rc != EOK)
				goto error;
		}
	}

	fibril_mutex_unlock(&addr_list_lock);
	return EOK;
error:
	fibril_mutex_unlock(&addr_list_lock);
	return rc;
}

/** @}
 */
