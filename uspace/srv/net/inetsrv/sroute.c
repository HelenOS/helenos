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
#include <io/log.h>
#include <ipc/loc.h>
#include <sif.h>
#include <stdio.h>
#include <stdlib.h>
#include <str.h>
#include "sroute.h"
#include "inetsrv.h"
#include "inet_link.h"

static FIBRIL_MUTEX_INITIALIZE(sroute_list_lock);
static LIST_INITIALIZE(sroute_list);
static sysarg_t sroute_id = 0;

inet_sroute_t *inet_sroute_new(void)
{
	inet_sroute_t *sroute = calloc(1, sizeof(inet_sroute_t));

	if (sroute == NULL) {
		log_msg(LOG_DEFAULT, LVL_ERROR, "Failed allocating static route object. "
		    "Out of memory.");
		return NULL;
	}

	link_initialize(&sroute->sroute_list);
	fibril_mutex_lock(&sroute_list_lock);
	sroute->id = ++sroute_id;
	fibril_mutex_unlock(&sroute_list_lock);

	return sroute;
}

void inet_sroute_delete(inet_sroute_t *sroute)
{
	if (sroute->name != NULL)
		free(sroute->name);
	free(sroute);
}

void inet_sroute_add(inet_sroute_t *sroute)
{
	fibril_mutex_lock(&sroute_list_lock);
	list_append(&sroute->sroute_list, &sroute_list);
	fibril_mutex_unlock(&sroute_list_lock);
}

void inet_sroute_remove(inet_sroute_t *sroute)
{
	fibril_mutex_lock(&sroute_list_lock);
	list_remove(&sroute->sroute_list);
	fibril_mutex_unlock(&sroute_list_lock);
}

/** Find static route object matching address @a addr.
 *
 * @param addr	Address
 */
inet_sroute_t *inet_sroute_find(inet_addr_t *addr)
{
	ip_ver_t addr_ver = inet_addr_get(addr, NULL, NULL);

	inet_sroute_t *best = NULL;
	uint8_t best_bits = 0;

	fibril_mutex_lock(&sroute_list_lock);

	list_foreach(sroute_list, sroute_list, inet_sroute_t, sroute) {
		uint8_t dest_bits;
		ip_ver_t dest_ver = inet_naddr_get(&sroute->dest, NULL, NULL,
		    &dest_bits);

		/* Skip comparison with different address family */
		if (addr_ver != dest_ver)
			continue;

		/* Look for the most specific route */
		if ((best != NULL) && (best_bits >= dest_bits))
			continue;

		if (inet_naddr_compare_mask(&sroute->dest, addr)) {
			log_msg(LOG_DEFAULT, LVL_DEBUG, "inet_sroute_find: found candidate %p",
			    sroute);

			best = sroute;
			best_bits = dest_bits;
		}
	}

	if (best == NULL)
		log_msg(LOG_DEFAULT, LVL_DEBUG, "inet_sroute_find: Not found");

	fibril_mutex_unlock(&sroute_list_lock);

	return best;
}

/** Find static route with a specific name.
 *
 * @param name	Address object name
 * @return	Address object
 */
inet_sroute_t *inet_sroute_find_by_name(const char *name)
{
	log_msg(LOG_DEFAULT, LVL_DEBUG, "inet_sroute_find_by_name('%s')",
	    name);

	fibril_mutex_lock(&sroute_list_lock);

	list_foreach(sroute_list, sroute_list, inet_sroute_t, sroute) {
		if (str_cmp(sroute->name, name) == 0) {
			fibril_mutex_unlock(&sroute_list_lock);
			log_msg(LOG_DEFAULT, LVL_DEBUG, "inet_sroute_find_by_name: found %p",
			    sroute);
			return sroute;
		}
	}

	log_msg(LOG_DEFAULT, LVL_DEBUG, "inet_sroute_find_by_name: Not found");
	fibril_mutex_unlock(&sroute_list_lock);

	return NULL;
}

/** Find static route with the given ID.
 *
 * @param id	Address object ID
 * @return	Address object
 */
inet_sroute_t *inet_sroute_get_by_id(sysarg_t id)
{
	log_msg(LOG_DEFAULT, LVL_DEBUG, "inet_sroute_get_by_id(%zu)", (size_t)id);

	fibril_mutex_lock(&sroute_list_lock);

	list_foreach(sroute_list, sroute_list, inet_sroute_t, sroute) {
		if (sroute->id == id) {
			fibril_mutex_unlock(&sroute_list_lock);
			return sroute;
		}
	}

	fibril_mutex_unlock(&sroute_list_lock);

	return NULL;
}

/** Get IDs of all static routes. */
errno_t inet_sroute_get_id_list(sysarg_t **rid_list, size_t *rcount)
{
	sysarg_t *id_list;
	size_t count, i;

	fibril_mutex_lock(&sroute_list_lock);
	count = list_count(&sroute_list);

	id_list = calloc(count, sizeof(sysarg_t));
	if (id_list == NULL) {
		fibril_mutex_unlock(&sroute_list_lock);
		return ENOMEM;
	}

	i = 0;
	list_foreach(sroute_list, sroute_list, inet_sroute_t, sroute) {
		id_list[i++] = sroute->id;
	}

	fibril_mutex_unlock(&sroute_list_lock);

	*rid_list = id_list;
	*rcount = count;

	return EOK;
}

/** Load static route from SIF node.
 *
 * @param nroute SIF node to load static route from
 * @return EOK on success or an error code
 */
static errno_t inet_sroute_load(sif_node_t *nroute)
{
	errno_t rc;
	const char *sid;
	const char *sdest;
	const char *srouter;
	const char *name;
	char *endptr;
	int id;
	inet_naddr_t dest;
	inet_addr_t router;
	inet_sroute_t *sroute;

	sid = sif_node_get_attr(nroute, "id");
	if (sid == NULL)
		return EIO;

	sdest = sif_node_get_attr(nroute, "dest");
	if (sdest == NULL)
		return EIO;

	srouter = sif_node_get_attr(nroute, "router");
	if (srouter == NULL)
		return EIO;

	name = sif_node_get_attr(nroute, "name");
	if (name == NULL)
		return EIO;

	id = strtoul(sid, &endptr, 10);
	if (*endptr != '\0')
		return EIO;

	rc = inet_naddr_parse(sdest, &dest, NULL);
	if (rc != EOK)
		return EIO;

	rc = inet_addr_parse(srouter, &router, NULL);
	if (rc != EOK)
		return EIO;

	sroute = inet_sroute_new();
	if (sroute == NULL)
		return ENOMEM;

	sroute->id = id;
	sroute->dest = dest;
	sroute->router = router;
	sroute->name = str_dup(name);

	if (sroute->name == NULL) {
		inet_sroute_delete(sroute);
		return ENOMEM;
	}

	inet_sroute_add(sroute);
	return EOK;
}

/** Load static routes from SIF node.
 *
 * @param nroutes SIF node to load static routes from
 * @return EOK on success or an error code
 */
errno_t inet_sroutes_load(sif_node_t *nroutes)
{
	sif_node_t *nroute;
	const char *ntype;
	errno_t rc;

	nroute = sif_node_first_child(nroutes);
	while (nroute != NULL) {
		ntype = sif_node_get_type(nroute);
		if (str_cmp(ntype, "route") != 0) {
			rc = EIO;
			goto error;
		}

		rc = inet_sroute_load(nroute);
		if (rc != EOK)
			goto error;

		nroute = sif_node_next_child(nroute);
	}

	return EOK;
error:
	return rc;
}

/** Save static route to SIF node.
 *
 * @param sroute Static route
 * @param nroute SIF node to save static route to
 * @return EOK on success or an error code
 */
static errno_t inet_sroute_save(inet_sroute_t *sroute, sif_node_t *nroute)
{
	char *str = NULL;
	errno_t rc;
	int rv;

	log_msg(LOG_DEFAULT, LVL_DEBUG, "inet_sroute_save(%p, %p)",
	    sroute, nroute);

	/* id */

	rv = asprintf(&str, "%zu", sroute->id);
	if (rv < 0) {
		str = NULL;
		rc = ENOMEM;
		goto error;
	}

	rc = sif_node_set_attr(nroute, "id", str);
	if (rc != EOK)
		goto error;

	free(str);
	str = NULL;

	/* dest */

	rc = inet_naddr_format(&sroute->dest, &str);
	if (rc != EOK)
		goto error;

	rc = sif_node_set_attr(nroute, "dest", str);
	if (rc != EOK)
		goto error;

	free(str);
	str = NULL;

	/* router */

	rc = inet_addr_format(&sroute->router, &str);
	if (rc != EOK)
		goto error;

	rc = sif_node_set_attr(nroute, "router", str);
	if (rc != EOK)
		goto error;

	/* name */

	rc = sif_node_set_attr(nroute, "name", sroute->name);
	if (rc != EOK)
		goto error;

	free(str);

	return rc;
error:
	if (str != NULL)
		free(str);
	return rc;
}

/** Save static routes to SIF node.
 *
 * @param nroutes SIF node to save static routes to
 * @return EOK on success or an error code
 */
errno_t inet_sroutes_save(sif_node_t *nroutes)
{
	sif_node_t *nroute;
	errno_t rc;

	log_msg(LOG_DEFAULT, LVL_DEBUG, "inet_sroutes_save()");

	fibril_mutex_lock(&sroute_list_lock);

	list_foreach(sroute_list, sroute_list, inet_sroute_t, sroute) {
		if (sroute->temp == false) {
			rc = sif_node_append_child(nroutes, "route", &nroute);
			if (rc != EOK)
				goto error;

			rc = inet_sroute_save(sroute, nroute);
			if (rc != EOK)
				goto error;
		}
	}

	fibril_mutex_unlock(&sroute_list_lock);
	return EOK;
error:
	fibril_mutex_unlock(&sroute_list_lock);
	return rc;
}

/** @}
 */
