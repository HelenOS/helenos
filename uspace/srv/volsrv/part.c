/*
 * Copyright (c) 2015 Jiri Svoboda
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

/** @addtogroup volsrv
 * @{
 */
/**
 * @file Partition handling
 * @brief
 */

#include <stdbool.h>
#include <errno.h>
#include <fibril_synch.h>
#include <io/log.h>
#include <loc.h>
#include <stdlib.h>
#include <str.h>

#include "empty.h"
#include "part.h"
#include "types/part.h"

static int vol_part_add_locked(service_id_t);
static LIST_INITIALIZE(vol_parts); /* of vol_part_t */
static FIBRIL_MUTEX_INITIALIZE(vol_parts_lock);

/** Check for new partitions */
static int vol_part_check_new(void)
{
	bool already_known;
	category_id_t part_cat;
	service_id_t *svcs;
	size_t count, i;
	int rc;

	fibril_mutex_lock(&vol_parts_lock);

	rc = loc_category_get_id("partition", &part_cat, IPC_FLAG_BLOCKING);
	if (rc != EOK) {
		log_msg(LOG_DEFAULT, LVL_ERROR, "Failed resolving category 'partition'.");
		fibril_mutex_unlock(&vol_parts_lock);
		return ENOENT;
	}

	rc = loc_category_get_svcs(part_cat, &svcs, &count);
	if (rc != EOK) {
		log_msg(LOG_DEFAULT, LVL_ERROR, "Failed getting list of partition "
		    "devices.");
		fibril_mutex_unlock(&vol_parts_lock);
		return EIO;
	}

	for (i = 0; i < count; i++) {
		already_known = false;

		list_foreach(vol_parts, lparts, vol_part_t, part) {
			if (part->svc_id == svcs[i]) {
				already_known = true;
				break;
			}
		}

		if (!already_known) {
			log_msg(LOG_DEFAULT, LVL_NOTE, "Found partition '%lu'",
			    (unsigned long) svcs[i]);
			rc = vol_part_add_locked(svcs[i]);
			if (rc != EOK) {
				log_msg(LOG_DEFAULT, LVL_ERROR, "Could not add "
				    "partition.");
			}
		}
	}

	fibril_mutex_unlock(&vol_parts_lock);
	return EOK;
}

static vol_part_t *vol_part_new(void)
{
	vol_part_t *part = calloc(1, sizeof(vol_part_t));

	if (part == NULL) {
		log_msg(LOG_DEFAULT, LVL_ERROR, "Failed allocating partition "
		    "structure. Out of memory.");
		return NULL;
	}

	link_initialize(&part->lparts);
	part->pcnt = vpc_empty;

	return part;
}

static void vol_part_delete(vol_part_t *part)
{
	if (part == NULL)
		return;

	free(part->svc_name);
	free(part);
}

static int vol_part_add_locked(service_id_t sid)
{
	vol_part_t *part;
	bool empty;
	int rc;

	assert(fibril_mutex_is_locked(&vol_parts_lock));

	/* Check for duplicates */
	rc = vol_part_find_by_id(sid, &part);
	if (rc == EOK)
		return EEXIST;

	log_msg(LOG_DEFAULT, LVL_NOTE, "vol_part_add()");
	part = vol_part_new();
	if (part == NULL)
		return ENOMEM;

	part->svc_id = sid;

	rc = loc_service_get_name(sid, &part->svc_name);
	if (rc != EOK) {
		log_msg(LOG_DEFAULT, LVL_ERROR, "Failed getting service name.");
		goto error;
	}

	log_msg(LOG_DEFAULT, LVL_NOTE, "Probe partition %s", part->svc_name);
	rc = vol_part_is_empty(sid, &empty);
	if (rc != EOK) {
		log_msg(LOG_DEFAULT, LVL_ERROR, "Failed determining if "
		    "partition is empty.");
		goto error;
	}

	part->pcnt = empty ? vpc_empty : vpc_unknown;
	list_append(&part->lparts, &vol_parts);

	log_msg(LOG_DEFAULT, LVL_NOTE, "Added partition %zu", part->svc_id);

	return EOK;

error:
	vol_part_delete(part);
	return rc;
}

int vol_part_add(service_id_t sid)
{
	int rc;

	fibril_mutex_lock(&vol_parts_lock);
	rc = vol_part_add_locked(sid);
	fibril_mutex_unlock(&vol_parts_lock);

	return rc;
}

static void vol_part_cat_change_cb(void)
{
	(void) vol_part_check_new();
}

int vol_part_init(void)
{
	return EOK;
}

int vol_part_discovery_start(void)
{
	int rc;

	rc = loc_register_cat_change_cb(vol_part_cat_change_cb);
	if (rc != EOK) {
		log_msg(LOG_DEFAULT, LVL_ERROR, "Failed registering callback "
		    "for partition discovery (%d).", rc);
		return rc;
	}

	return vol_part_check_new();
}

/** Get list of partitions as array of service IDs. */
int vol_part_get_ids(service_id_t *id_buf, size_t buf_size, size_t *act_size)
{
	size_t act_cnt;
	size_t buf_cnt;

	fibril_mutex_lock(&vol_parts_lock);

	buf_cnt = buf_size / sizeof(service_id_t);

	act_cnt = list_count(&vol_parts);
	*act_size = act_cnt * sizeof(service_id_t);

	if (buf_size % sizeof(service_id_t) != 0) {
		fibril_mutex_unlock(&vol_parts_lock);
		return EINVAL;
	}

	size_t pos = 0;
	list_foreach(vol_parts, lparts, vol_part_t, part) {
		if (pos < buf_cnt)
			id_buf[pos] = part->svc_id;
		pos++;
	}

	fibril_mutex_unlock(&vol_parts_lock);
	return EOK;
}

int vol_part_find_by_id(service_id_t sid, vol_part_t **rpart)
{
	list_foreach(vol_parts, lparts, vol_part_t, part) {
		if (part->svc_id == sid) {
			*rpart = part;
			/* XXX Add reference */
			return EOK;
		}
	}

	return ENOENT;
}

int vol_part_empty_part(vol_part_t *part)
{
	int rc;

	log_msg(LOG_DEFAULT, LVL_NOTE, "vol_part_empty_part()");

	rc = vol_part_empty(part->svc_id);
	if (rc != EOK) {
		log_msg(LOG_DEFAULT, LVL_NOTE, "vol_part_empty_part() - failed %d",
		    rc);
		return rc;
	}

	log_msg(LOG_DEFAULT, LVL_NOTE, "vol_part_empty_part() - success");
	part->pcnt = vpc_empty;
	return EOK;
}

int vol_part_get_info(vol_part_t *part, vol_part_info_t *pinfo)
{
	pinfo->pcnt = part->pcnt;
	pinfo->fstype = part->fstype;
	return EOK;
}

/** @}
 */
