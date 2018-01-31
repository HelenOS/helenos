/*
 * Copyright (c) 2013 Jiri Svoboda
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

/** @addtogroup nconfsrv
 * @{
 */
/**
 * @file
 * @brief
 */

#include <stdbool.h>
#include <errno.h>
#include <str_error.h>
#include <fibril_synch.h>
#include <inet/dhcp.h>
#include <inet/inetcfg.h>
#include <io/log.h>
#include <loc.h>
#include <stdlib.h>
#include <str.h>

#include "iplink.h"
#include "nconfsrv.h"

static errno_t ncs_link_add(service_id_t);

static LIST_INITIALIZE(ncs_links);
static FIBRIL_MUTEX_INITIALIZE(ncs_links_lock);

static errno_t ncs_link_check_new(void)
{
	bool already_known;
	category_id_t iplink_cat;
	service_id_t *svcs;
	size_t count, i;
	errno_t rc;

	fibril_mutex_lock(&ncs_links_lock);

	rc = loc_category_get_id("iplink", &iplink_cat, IPC_FLAG_BLOCKING);
	if (rc != EOK) {
		log_msg(LOG_DEFAULT, LVL_ERROR, "Failed resolving category 'iplink'.");
		fibril_mutex_unlock(&ncs_links_lock);
		return ENOENT;
	}

	rc = loc_category_get_svcs(iplink_cat, &svcs, &count);
	if (rc != EOK) {
		log_msg(LOG_DEFAULT, LVL_ERROR, "Failed getting list of IP links.");
		fibril_mutex_unlock(&ncs_links_lock);
		return EIO;
	}

	for (i = 0; i < count; i++) {
		already_known = false;

		list_foreach(ncs_links, link_list, ncs_link_t, ilink) {
			if (ilink->svc_id == svcs[i]) {
				already_known = true;
				break;
			}
		}

		if (!already_known) {
			log_msg(LOG_DEFAULT, LVL_NOTE, "Found IP link '%lu'",
			    (unsigned long) svcs[i]);
			rc = ncs_link_add(svcs[i]);
			if (rc != EOK)
				log_msg(LOG_DEFAULT, LVL_ERROR, "Could not add IP link.");
		}
	}

	fibril_mutex_unlock(&ncs_links_lock);
	return EOK;
}

static ncs_link_t *ncs_link_new(void)
{
	ncs_link_t *nlink = calloc(1, sizeof(ncs_link_t));

	if (nlink == NULL) {
		log_msg(LOG_DEFAULT, LVL_ERROR, "Failed allocating link structure. "
		    "Out of memory.");
		return NULL;
	}

	link_initialize(&nlink->link_list);

	return nlink;
}

static void ncs_link_delete(ncs_link_t *nlink)
{
	if (nlink->svc_name != NULL)
		free(nlink->svc_name);

	free(nlink);
}

static errno_t ncs_link_add(service_id_t sid)
{
	ncs_link_t *nlink;
	errno_t rc;

	assert(fibril_mutex_is_locked(&ncs_links_lock));

	log_msg(LOG_DEFAULT, LVL_DEBUG, "ncs_link_add()");
	nlink = ncs_link_new();
	if (nlink == NULL)
		return ENOMEM;

	nlink->svc_id = sid;

	rc = loc_service_get_name(sid, &nlink->svc_name);
	if (rc != EOK) {
		log_msg(LOG_DEFAULT, LVL_ERROR, "Failed getting service name.");
		goto error;
	}

	log_msg(LOG_DEFAULT, LVL_NOTE, "Configure link %s", nlink->svc_name);
	rc = inetcfg_link_add(sid);
	if (rc != EOK) {
		log_msg(LOG_DEFAULT, LVL_ERROR, "Failed configuring link "
		    "'%s'.\n", nlink->svc_name);
		goto error;
	}

	if (str_lcmp(nlink->svc_name, "net/eth", str_length("net/eth")) == 0) {
		rc = dhcp_link_add(sid);
		if (rc != EOK) {
			log_msg(LOG_DEFAULT, LVL_ERROR, "Failed configuring DHCP on "
			    " link '%s'.\n", nlink->svc_name);
			goto error;
		}
	}

	list_append(&nlink->link_list, &ncs_links);

	return EOK;

error:
	ncs_link_delete(nlink);
	return rc;
}

static void ncs_link_cat_change_cb(void)
{
	(void) ncs_link_check_new();
}

errno_t ncs_link_discovery_start(void)
{
	errno_t rc;

	rc = loc_register_cat_change_cb(ncs_link_cat_change_cb);
	if (rc != EOK) {
		log_msg(LOG_DEFAULT, LVL_ERROR, "Failed registering callback for IP link "
		    "discovery: %s.", str_error(rc));
		return rc;
	}

	return ncs_link_check_new();
}

ncs_link_t *ncs_link_get_by_id(sysarg_t link_id)
{
	fibril_mutex_lock(&ncs_links_lock);

	list_foreach(ncs_links, link_list, ncs_link_t, nlink) {
		if (nlink->svc_id == link_id) {
			fibril_mutex_unlock(&ncs_links_lock);
			return nlink;
		}
	}

	fibril_mutex_unlock(&ncs_links_lock);
	return NULL;
}

/** Get IDs of all links. */
errno_t ncs_link_get_id_list(sysarg_t **rid_list, size_t *rcount)
{
	sysarg_t *id_list;
	size_t count, i;

	fibril_mutex_lock(&ncs_links_lock);
	count = list_count(&ncs_links);

	id_list = calloc(count, sizeof(sysarg_t));
	if (id_list == NULL) {
		fibril_mutex_unlock(&ncs_links_lock);
		return ENOMEM;
	}

	i = 0;
	list_foreach(ncs_links, link_list, ncs_link_t, nlink) {
		id_list[i++] = nlink->svc_id;
		log_msg(LOG_DEFAULT, LVL_NOTE, "add link to list");
	}

	fibril_mutex_unlock(&ncs_links_lock);

	log_msg(LOG_DEFAULT, LVL_NOTE, "return %zu links", count);
	*rid_list = id_list;
	*rcount = count;

	return EOK;
}

/** @}
 */
