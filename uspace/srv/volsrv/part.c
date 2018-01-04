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
#include <str_error.h>
#include <fibril_synch.h>
#include <io/log.h>
#include <loc.h>
#include <stdlib.h>
#include <str.h>
#include <vfs/vfs.h>

#include "empty.h"
#include "mkfs.h"
#include "part.h"
#include "types/part.h"

static errno_t vol_part_add_locked(service_id_t);
static LIST_INITIALIZE(vol_parts); /* of vol_part_t */
static FIBRIL_MUTEX_INITIALIZE(vol_parts_lock);

struct fsname_type {
	const char *name;
	vol_fstype_t fstype;
};

static struct fsname_type fstab[] = {
	{ "ext4fs", fs_ext4 },
	{ "cdfs", fs_cdfs },
	{ "exfat", fs_exfat },
	{ "fat", fs_fat },
	{ "mfs", fs_minix },
	{ NULL, 0 }
};

/** Check for new partitions */
static errno_t vol_part_check_new(void)
{
	bool already_known;
	category_id_t part_cat;
	service_id_t *svcs;
	size_t count, i;
	errno_t rc;

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

static errno_t vol_part_probe(vol_part_t *part)
{
	bool empty;
	vfs_fs_probe_info_t info;
	struct fsname_type *fst;
	char *label;
	errno_t rc;

	log_msg(LOG_DEFAULT, LVL_NOTE, "Probe partition %s", part->svc_name);

	assert(fibril_mutex_is_locked(&vol_parts_lock));

	fst = &fstab[0];
	while (fst->name != NULL) {
		rc = vfs_fsprobe(fst->name, part->svc_id, &info);
		if (rc == EOK)
			break;
		++fst;
	}

	if (fst->name != NULL) {
		log_msg(LOG_DEFAULT, LVL_NOTE, "Found %s, label '%s'",
		    fst->name, info.label);
		label = str_dup(info.label);
		if (label == NULL) {
			rc = ENOMEM;
			goto error;
		}

		part->pcnt = vpc_fs;
		part->fstype = fst->fstype;
		part->label = label;
	} else {
		log_msg(LOG_DEFAULT, LVL_NOTE, "Partition does not contain "
		    "a recognized file system.");

		rc = volsrv_part_is_empty(part->svc_id, &empty);
		if (rc != EOK) {
			log_msg(LOG_DEFAULT, LVL_ERROR, "Failed determining if "
			    "partition is empty.");
			rc = EIO;
			goto error;
		}

		label = str_dup("");
		if (label == NULL) {
			rc = ENOMEM;
			goto error;
		}

		part->pcnt = empty ? vpc_empty : vpc_unknown;
		part->label = label;
	}

	return EOK;

error:
	return rc;
}

static errno_t vol_part_add_locked(service_id_t sid)
{
	vol_part_t *part;
	errno_t rc;

	assert(fibril_mutex_is_locked(&vol_parts_lock));

	/* Check for duplicates */
	rc = vol_part_find_by_id(sid, &part);
	if (rc == EOK)
		return EEXIST;

	log_msg(LOG_DEFAULT, LVL_DEBUG, "vol_part_add_locked()");
	part = vol_part_new();
	if (part == NULL)
		return ENOMEM;

	part->svc_id = sid;

	rc = loc_service_get_name(sid, &part->svc_name);
	if (rc != EOK) {
		log_msg(LOG_DEFAULT, LVL_ERROR, "Failed getting service name.");
		goto error;
	}

	rc = vol_part_probe(part);
	if (rc != EOK)
		goto error;

	list_append(&part->lparts, &vol_parts);

	log_msg(LOG_DEFAULT, LVL_NOTE, "Added partition %zu", part->svc_id);

	return EOK;

error:
	vol_part_delete(part);
	return rc;
}

errno_t vol_part_add(service_id_t sid)
{
	errno_t rc;

	fibril_mutex_lock(&vol_parts_lock);
	rc = vol_part_add_locked(sid);
	fibril_mutex_unlock(&vol_parts_lock);

	return rc;
}

static void vol_part_cat_change_cb(void)
{
	(void) vol_part_check_new();
}

errno_t vol_part_init(void)
{
	return EOK;
}

errno_t vol_part_discovery_start(void)
{
	errno_t rc;

	rc = loc_register_cat_change_cb(vol_part_cat_change_cb);
	if (rc != EOK) {
		log_msg(LOG_DEFAULT, LVL_ERROR, "Failed registering callback "
		    "for partition discovery: %s.", str_error(rc));
		return rc;
	}

	return vol_part_check_new();
}

/** Get list of partitions as array of service IDs. */
errno_t vol_part_get_ids(service_id_t *id_buf, size_t buf_size, size_t *act_size)
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

errno_t vol_part_find_by_id(service_id_t sid, vol_part_t **rpart)
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

errno_t vol_part_empty_part(vol_part_t *part)
{
	errno_t rc;

	log_msg(LOG_DEFAULT, LVL_DEBUG, "vol_part_empty_part()");

	rc = volsrv_part_empty(part->svc_id);
	if (rc != EOK) {
		log_msg(LOG_DEFAULT, LVL_DEBUG, "vol_part_empty_part() - failed %s",
		    str_error(rc));
		return rc;
	}

	part->pcnt = vpc_empty;
	return EOK;
}

errno_t vol_part_mkfs_part(vol_part_t *part, vol_fstype_t fstype,
    const char *label)
{
	errno_t rc;

	log_msg(LOG_DEFAULT, LVL_DEBUG, "vol_part_mkfs_part()");

	fibril_mutex_lock(&vol_parts_lock);

	rc = volsrv_part_mkfs(part->svc_id, fstype, label);
	if (rc != EOK) {
		log_msg(LOG_DEFAULT, LVL_DEBUG, "vol_part_mkfs_part() - failed %s",
		    str_error(rc));
		fibril_mutex_unlock(&vol_parts_lock);
		return rc;
	}

	/*
	 * Re-probe the partition to update information. This is needed since
	 * the FS can make conversions of the volume label (e.g. make it
	 * uppercase).
	 */
	rc = vol_part_probe(part);
	if (rc != EOK) {
		fibril_mutex_unlock(&vol_parts_lock);
		return rc;
	}

	fibril_mutex_unlock(&vol_parts_lock);
	return EOK;
}

errno_t vol_part_get_info(vol_part_t *part, vol_part_info_t *pinfo)
{
	pinfo->pcnt = part->pcnt;
	pinfo->fstype = part->fstype;
	str_cpy(pinfo->label, sizeof(pinfo->label), part->label);
	return EOK;
}

/** @}
 */
