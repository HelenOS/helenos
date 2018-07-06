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

#include <adt/list.h>
#include <errno.h>
#include <fibril_synch.h>
#include <io/log.h>
#include <loc.h>
#include <stdbool.h>
#include <stdlib.h>
#include <str.h>
#include <str_error.h>
#include <vfs/vfs.h>

#include "empty.h"
#include "mkfs.h"
#include "part.h"
#include "types/part.h"

static errno_t vol_part_add_locked(vol_parts_t *, service_id_t);
static void vol_part_remove_locked(vol_part_t *);
static errno_t vol_part_find_by_id_ref_locked(vol_parts_t *, service_id_t,
    vol_part_t **);

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

static const char *fstype_str(vol_fstype_t fstype)
{
	struct fsname_type *fst;

	fst = &fstab[0];
	while (fst->name != NULL) {
		if (fst->fstype == fstype)
			return fst->name;
		++fst;
	}

	assert(false);
	return NULL;
}

/** Check for new and removed partitions */
static errno_t vol_part_check_new(vol_parts_t *parts)
{
	bool already_known;
	bool still_exists;
	category_id_t part_cat;
	service_id_t *svcs;
	size_t count, i;
	link_t *cur, *next;
	vol_part_t *part;
	errno_t rc;

	fibril_mutex_lock(&parts->lock);

	rc = loc_category_get_id("partition", &part_cat, IPC_FLAG_BLOCKING);
	if (rc != EOK) {
		log_msg(LOG_DEFAULT, LVL_ERROR, "Failed resolving category 'partition'.");
		fibril_mutex_unlock(&parts->lock);
		return ENOENT;
	}

	rc = loc_category_get_svcs(part_cat, &svcs, &count);
	if (rc != EOK) {
		log_msg(LOG_DEFAULT, LVL_ERROR, "Failed getting list of partition "
		    "devices.");
		fibril_mutex_unlock(&parts->lock);
		return EIO;
	}

	/* Check for new partitions */
	for (i = 0; i < count; i++) {
		already_known = false;

		// XXX Make this faster
		list_foreach(parts->parts, lparts, vol_part_t, part) {
			if (part->svc_id == svcs[i]) {
				already_known = true;
				break;
			}
		}

		if (!already_known) {
			log_msg(LOG_DEFAULT, LVL_NOTE, "Found partition '%lu'",
			    (unsigned long) svcs[i]);
			rc = vol_part_add_locked(parts, svcs[i]);
			if (rc != EOK) {
				log_msg(LOG_DEFAULT, LVL_ERROR, "Could not add "
				    "partition.");
			}
		}
	}

	/* Check for removed partitions */
	cur = list_first(&parts->parts);
	while (cur != NULL) {
		next = list_next(cur, &parts->parts);
		part = list_get_instance(cur, vol_part_t, lparts);

		still_exists = false;
		// XXX Make this faster
		for (i = 0; i < count; i++) {
			if (part->svc_id == svcs[i]) {
				still_exists = true;
				break;
			}
		}

		if (!still_exists) {
			log_msg(LOG_DEFAULT, LVL_NOTE, "Partition '%zu' is gone",
			    part->svc_id);
			vol_part_remove_locked(part);
		}

		cur = next;
	}

	free(svcs);

	fibril_mutex_unlock(&parts->lock);
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

	atomic_set(&part->refcnt, 1);
	link_initialize(&part->lparts);
	part->parts = NULL;
	part->pcnt = vpc_empty;

	return part;
}

static void vol_part_delete(vol_part_t *part)
{
	log_msg(LOG_DEFAULT, LVL_ERROR, "Freeing partition %p", part);
	if (part == NULL)
		return;

	free(part->cur_mp);
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

	assert(fibril_mutex_is_locked(&part->parts->lock));

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

/** Determine if partition is allowed to be mounted by default.
 *
 * @param part Partition
 * @return @c true iff partition is allowed to be mounted by default
 */
static bool vol_part_allow_mount_by_def(vol_part_t *part)
{
	/* CDFS is safe to mount (after all, it is read-only) */
	if (part->pcnt == vpc_fs && part->fstype == fs_cdfs)
		return true;

	/* For other file systems disallow mounting from ATA hard drive */
	if (str_str(part->svc_name, "\\ata-c") != NULL)
		return false;

	/* Allow otherwise (e.g. USB mass storage) */
	return true;
}

static errno_t vol_part_mount(vol_part_t *part)
{
	char *mp;
	int err;
	errno_t rc;

	if (str_size(part->label) < 1) {
		/* Don't mount nameless volumes */
		log_msg(LOG_DEFAULT, LVL_NOTE, "Not mounting nameless partition.");
		return EOK;
	}

	if (!vol_part_allow_mount_by_def(part)) {
		/* Don't mount partition by default */
		log_msg(LOG_DEFAULT, LVL_NOTE, "Not mounting per default policy.");
		return EOK;
	}

	log_msg(LOG_DEFAULT, LVL_NOTE, "Determine MP label='%s'", part->label);
	err = asprintf(&mp, "/vol/%s", part->label);
	if (err < 0) {
		log_msg(LOG_DEFAULT, LVL_ERROR, "Out of memory");
		return ENOMEM;
	}

	log_msg(LOG_DEFAULT, LVL_NOTE, "Create mount point '%s'", mp);
	rc = vfs_link_path(mp, KIND_DIRECTORY, NULL);
	if (rc != EOK) {
		log_msg(LOG_DEFAULT, LVL_ERROR, "Error creating mount point '%s'",
		    mp);
		free(mp);
		return EIO;
	}

	log_msg(LOG_DEFAULT, LVL_NOTE, "Call vfs_mount_path mp='%s' fstype='%s' svc_name='%s'",
	    mp, fstype_str(part->fstype), part->svc_name);
	rc = vfs_mount_path(mp, fstype_str(part->fstype),
	    part->svc_name, "", 0, 0);
	if (rc != EOK) {
		log_msg(LOG_DEFAULT, LVL_NOTE, "Failed mounting to %s", mp);
	}
	log_msg(LOG_DEFAULT, LVL_NOTE, "Mount to %s -> %d\n", mp, rc);

	part->cur_mp = mp;
	part->cur_mp_auto = true;

	return rc;
}

static errno_t vol_part_add_locked(vol_parts_t *parts, service_id_t sid)
{
	vol_part_t *part;
	errno_t rc;

	assert(fibril_mutex_is_locked(&parts->lock));
	log_msg(LOG_DEFAULT, LVL_NOTE, "vol_part_add_locked(%zu)", sid);

	/* Check for duplicates */
	rc = vol_part_find_by_id_ref_locked(parts, sid, &part);
	if (rc == EOK) {
		vol_part_del_ref(part);
		return EEXIST;
	}

	log_msg(LOG_DEFAULT, LVL_NOTE, "partition %zu is new", sid);

	part = vol_part_new();
	if (part == NULL)
		return ENOMEM;

	part->svc_id = sid;
	part->parts = parts;

	rc = loc_service_get_name(sid, &part->svc_name);
	if (rc != EOK) {
		log_msg(LOG_DEFAULT, LVL_ERROR, "Failed getting service name.");
		goto error;
	}

	rc = vol_part_probe(part);
	if (rc != EOK)
		goto error;

	rc = vol_part_mount(part);
	if (rc != EOK)
		goto error;

	list_append(&part->lparts, &parts->parts);

	log_msg(LOG_DEFAULT, LVL_NOTE, "Added partition %zu", part->svc_id);

	return EOK;

error:
	vol_part_delete(part);
	return rc;
}

static void vol_part_remove_locked(vol_part_t *part)
{
	assert(fibril_mutex_is_locked(&part->parts->lock));
	log_msg(LOG_DEFAULT, LVL_NOTE, "vol_part_remove_locked(%zu)",
	    part->svc_id);

	list_remove(&part->lparts);

	log_msg(LOG_DEFAULT, LVL_NOTE, "Removed partition.");
	vol_part_del_ref(part);
}

errno_t vol_part_add(vol_parts_t *parts, service_id_t sid)
{
	errno_t rc;

	fibril_mutex_lock(&parts->lock);
	rc = vol_part_add_locked(parts, sid);
	fibril_mutex_unlock(&parts->lock);

	return rc;
}

static void vol_part_cat_change_cb(void *arg)
{
	vol_parts_t *parts = (vol_parts_t *) arg;

	(void) vol_part_check_new(parts);
}

errno_t vol_parts_create(vol_parts_t **rparts)
{
	vol_parts_t *parts;

	parts = calloc(1, sizeof(vol_parts_t));
	if (parts == NULL)
		return ENOMEM;

	fibril_mutex_initialize(&parts->lock);
	list_initialize(&parts->parts);

	*rparts = parts;
	return EOK;
}

void vol_parts_destroy(vol_parts_t *parts)
{
	if (parts == NULL)
		return;

	assert(list_empty(&parts->parts));
	free(parts);
}

errno_t vol_part_discovery_start(vol_parts_t *parts)
{
	errno_t rc;

	rc = loc_register_cat_change_cb(vol_part_cat_change_cb, parts);
	if (rc != EOK) {
		log_msg(LOG_DEFAULT, LVL_ERROR, "Failed registering callback "
		    "for partition discovery: %s.", str_error(rc));
		return rc;
	}

	return vol_part_check_new(parts);
}

/** Get list of partitions as array of service IDs. */
errno_t vol_part_get_ids(vol_parts_t *parts, service_id_t *id_buf,
    size_t buf_size, size_t *act_size)
{
	size_t act_cnt;
	size_t buf_cnt;

	fibril_mutex_lock(&parts->lock);

	buf_cnt = buf_size / sizeof(service_id_t);

	act_cnt = list_count(&parts->parts);
	*act_size = act_cnt * sizeof(service_id_t);

	if (buf_size % sizeof(service_id_t) != 0) {
		fibril_mutex_unlock(&parts->lock);
		return EINVAL;
	}

	size_t pos = 0;
	list_foreach(parts->parts, lparts, vol_part_t, part) {
		if (pos < buf_cnt)
			id_buf[pos] = part->svc_id;
		pos++;
	}

	fibril_mutex_unlock(&parts->lock);
	return EOK;
}

static errno_t vol_part_find_by_id_ref_locked(vol_parts_t *parts,
    service_id_t sid, vol_part_t **rpart)
{
	assert(fibril_mutex_is_locked(&parts->lock));

	list_foreach(parts->parts, lparts, vol_part_t, part) {
		if (part->svc_id == sid) {
			/* Add reference */
			atomic_inc(&part->refcnt);
			*rpart = part;
			return EOK;
		}
	}

	return ENOENT;
}

errno_t vol_part_find_by_id_ref(vol_parts_t *parts, service_id_t sid,
    vol_part_t **rpart)
{
	errno_t rc;

	fibril_mutex_lock(&parts->lock);
	rc = vol_part_find_by_id_ref_locked(parts, sid, rpart);
	fibril_mutex_unlock(&parts->lock);

	return rc;
}

void vol_part_del_ref(vol_part_t *part)
{
	if (atomic_predec(&part->refcnt) == 0)
		vol_part_delete(part);
}

errno_t vol_part_eject_part(vol_part_t *part)
{
	int rc;

	log_msg(LOG_DEFAULT, LVL_DEBUG, "vol_part_eject_part()");

	if (part->cur_mp == NULL) {
		log_msg(LOG_DEFAULT, LVL_DEBUG, "Attempt to mount unmounted "
		    "partition.");
		return EINVAL;
	}

	rc = vfs_unmount_path(part->cur_mp);
	if (rc != EOK) {
		log_msg(LOG_DEFAULT, LVL_ERROR, "Failed unmounting partition "
		    "from %s", part->cur_mp);
		return rc;
	}

	if (part->cur_mp_auto) {
		rc = vfs_unlink_path(part->cur_mp);
		if (rc != EOK) {
			log_msg(LOG_DEFAULT, LVL_ERROR, "Failed deleting "
			    "mount directory %s.", part->cur_mp);
		}
	}

	free(part->cur_mp);
	part->cur_mp = NULL;
	part->cur_mp_auto = false;

	return EOK;
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

	fibril_mutex_lock(&part->parts->lock);

	rc = volsrv_part_mkfs(part->svc_id, fstype, label);
	if (rc != EOK) {
		log_msg(LOG_DEFAULT, LVL_DEBUG, "vol_part_mkfs_part() - failed %s",
		    str_error(rc));
		fibril_mutex_unlock(&part->parts->lock);
		return rc;
	}

	/*
	 * Re-probe the partition to update information. This is needed since
	 * the FS can make conversions of the volume label (e.g. make it
	 * uppercase).
	 */
	rc = vol_part_probe(part);
	if (rc != EOK) {
		fibril_mutex_unlock(&part->parts->lock);
		return rc;
	}

	rc = vol_part_mount(part);
	if (rc != EOK) {
		fibril_mutex_unlock(&part->parts->lock);
		return rc;
	}

	fibril_mutex_unlock(&part->parts->lock);
	return EOK;
}

errno_t vol_part_get_info(vol_part_t *part, vol_part_info_t *pinfo)
{
	memset(pinfo, 0, sizeof(*pinfo));

	pinfo->pcnt = part->pcnt;
	pinfo->fstype = part->fstype;
	str_cpy(pinfo->label, sizeof(pinfo->label), part->label);
	if (part->cur_mp != NULL)
		str_cpy(pinfo->cur_mp, sizeof(pinfo->cur_mp), part->cur_mp);
	pinfo->cur_mp_auto = part->cur_mp_auto;
	return EOK;
}

/** @}
 */
