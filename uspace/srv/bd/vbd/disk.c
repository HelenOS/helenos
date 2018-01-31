/*
 * Copyright (c) 2016 Jiri Svoboda
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

/** @addtogroup vbd
 * @{
 */
/**
 * @file
 */

#include <adt/list.h>
#include <bd_srv.h>
#include <block.h>
#include <errno.h>
#include <str_error.h>
#include <io/log.h>
#include <label/empty.h>
#include <label/label.h>
#include <loc.h>
#include <stdio.h>
#include <stdlib.h>
#include <task.h>
#include <vbd.h>

#include "disk.h"
#include "types/vbd.h"

static fibril_mutex_t vbds_disks_lock;
static list_t vbds_disks; /* of vbds_disk_t */
static fibril_mutex_t vbds_parts_lock;
static list_t vbds_parts; /* of vbds_part_t */

static category_id_t part_cid;

static errno_t vbds_disk_parts_add(vbds_disk_t *, label_t *);
static errno_t vbds_disk_parts_remove(vbds_disk_t *, vbds_rem_flag_t);

static errno_t vbds_bd_open(bd_srvs_t *, bd_srv_t *);
static errno_t vbds_bd_close(bd_srv_t *);
static errno_t vbds_bd_read_blocks(bd_srv_t *, aoff64_t, size_t, void *, size_t);
static errno_t vbds_bd_sync_cache(bd_srv_t *, aoff64_t, size_t);
static errno_t vbds_bd_write_blocks(bd_srv_t *, aoff64_t, size_t, const void *,
    size_t);
static errno_t vbds_bd_get_block_size(bd_srv_t *, size_t *);
static errno_t vbds_bd_get_num_blocks(bd_srv_t *, aoff64_t *);

static errno_t vbds_bsa_translate(vbds_part_t *, aoff64_t, size_t, aoff64_t *);

static errno_t vbds_part_svc_register(vbds_part_t *);
static errno_t vbds_part_svc_unregister(vbds_part_t *);
static errno_t vbds_part_indices_update(vbds_disk_t *);

static vbd_part_id_t vbds_part_id = 1;

static errno_t vbds_label_get_bsize(void *, size_t *);
static errno_t vbds_label_get_nblocks(void *, aoff64_t *);
static errno_t vbds_label_read(void *, aoff64_t, size_t, void *);
static errno_t vbds_label_write(void *, aoff64_t, size_t, const void *);

/** Block device operations provided by VBD */
static bd_ops_t vbds_bd_ops = {
	.open = vbds_bd_open,
	.close = vbds_bd_close,
	.read_blocks = vbds_bd_read_blocks,
	.sync_cache = vbds_bd_sync_cache,
	.write_blocks = vbds_bd_write_blocks,
	.get_block_size = vbds_bd_get_block_size,
	.get_num_blocks = vbds_bd_get_num_blocks
};

/** Provide disk access to liblabel */
static label_bd_ops_t vbds_label_bd_ops = {
	.get_bsize = vbds_label_get_bsize,
	.get_nblocks = vbds_label_get_nblocks,
	.read = vbds_label_read,
	.write = vbds_label_write
};

static vbds_part_t *bd_srv_part(bd_srv_t *bd)
{
	return (vbds_part_t *)bd->srvs->sarg;
}

static vbds_disk_t *vbds_disk_first(void)
{
	link_t *link;

	link = list_first(&vbds_disks);
	if (link == NULL)
		return NULL;

	return list_get_instance(link, vbds_disk_t, ldisks);
}

static vbds_disk_t *vbds_disk_next(vbds_disk_t *disk)
{
	link_t *link;

	if (disk == NULL)
		return NULL;

	link = list_next(&disk->ldisks, &vbds_disks);
	if (link == NULL)
		return NULL;

	return list_get_instance(link, vbds_disk_t, ldisks);
}

errno_t vbds_disks_init(void)
{
	errno_t rc;

	fibril_mutex_initialize(&vbds_disks_lock);
	list_initialize(&vbds_disks);
	fibril_mutex_initialize(&vbds_parts_lock);
	list_initialize(&vbds_parts);

	rc = loc_category_get_id("partition", &part_cid, 0);
	if (rc != EOK) {
		log_msg(LOG_DEFAULT, LVL_ERROR, "Error looking up partition "
		    "category.");
		return EIO;
	}

	return EOK;
}

/** Check for new/removed disk devices */
static errno_t vbds_disks_check_new(void)
{
	bool already_known;
	category_id_t disk_cat;
	service_id_t *svcs;
	size_t count, i;
	vbds_disk_t *cur, *next;
	errno_t rc;

	log_msg(LOG_DEFAULT, LVL_DEBUG, "vbds_disks_check_new()");

	fibril_mutex_lock(&vbds_disks_lock);

	rc = loc_category_get_id("disk", &disk_cat, IPC_FLAG_BLOCKING);
	if (rc != EOK) {
		log_msg(LOG_DEFAULT, LVL_ERROR, "Failed resolving category 'disk'.");
		fibril_mutex_unlock(&vbds_disks_lock);
		return ENOENT;
	}

	rc = loc_category_get_svcs(disk_cat, &svcs, &count);
	if (rc != EOK) {
		log_msg(LOG_DEFAULT, LVL_ERROR, "Failed getting list of disk "
		    "devices.");
		fibril_mutex_unlock(&vbds_disks_lock);
		return EIO;
	}

	list_foreach(vbds_disks, ldisks, vbds_disk_t, disk)
		disk->present = false;

	for (i = 0; i < count; i++) {
		already_known = false;

		list_foreach(vbds_disks, ldisks, vbds_disk_t, disk) {
			if (disk->svc_id == svcs[i]) {
				already_known = true;
				disk->present = true;
				break;
			}
		}

		if (!already_known) {
			log_msg(LOG_DEFAULT, LVL_NOTE, "Found disk '%lu'",
			    (unsigned long) svcs[i]);
			rc = vbds_disk_add(svcs[i]);
			if (rc != EOK) {
				log_msg(LOG_DEFAULT, LVL_ERROR, "Could not add "
				    "disk.");
			}
		} else {
			log_msg(LOG_DEFAULT, LVL_DEBUG, "Disk %lu already known",
			    (unsigned long) svcs[i]);
		}
	}

	cur = vbds_disk_first();
	while (cur != NULL) {
		next = vbds_disk_next(cur);
		if (!cur->present) {
			log_msg(LOG_DEFAULT, LVL_NOTE, "Disk '%lu' is gone",
			    (unsigned long) cur->svc_id);
			rc = vbds_disk_remove(cur->svc_id);
			if (rc != EOK) {
				log_msg(LOG_DEFAULT, LVL_ERROR, "Could not "
				    "remove disk.");
			}
		}

		cur = next;
	}

	fibril_mutex_unlock(&vbds_disks_lock);
	return EOK;
}


static errno_t vbds_disk_by_svcid(service_id_t sid, vbds_disk_t **rdisk)
{
	list_foreach(vbds_disks, ldisks, vbds_disk_t, disk) {
		if (disk->svc_id == sid) {
			*rdisk = disk;
			return EOK;
		}
	}

	return ENOENT;
}

static void vbds_part_add_ref(vbds_part_t *part)
{
	log_msg(LOG_DEFAULT, LVL_DEBUG2, "vbds_part_add_ref");
	atomic_inc(&part->refcnt);
}

static void vbds_part_del_ref(vbds_part_t *part)
{
	log_msg(LOG_DEFAULT, LVL_DEBUG2, "vbds_part_del_ref");
	if (atomic_predec(&part->refcnt) == 0) {
		log_msg(LOG_DEFAULT, LVL_DEBUG2, " - free part");
		free(part);
	}
}

static errno_t vbds_part_by_pid(vbds_part_id_t partid, vbds_part_t **rpart)
{
	log_msg(LOG_DEFAULT, LVL_DEBUG, "vbds_part_by_pid(%zu)", partid);

	fibril_mutex_lock(&vbds_parts_lock);

	list_foreach(vbds_parts, lparts, vbds_part_t, part) {
		log_msg(LOG_DEFAULT, LVL_DEBUG, "%zu == %zu ?", part->pid, partid);
		if (part->pid == partid) {
			log_msg(LOG_DEFAULT, LVL_DEBUG, "Found match.");
			vbds_part_add_ref(part);
			fibril_mutex_unlock(&vbds_parts_lock);
			*rpart = part;
			return EOK;
		}
	}

	fibril_mutex_unlock(&vbds_parts_lock);
	log_msg(LOG_DEFAULT, LVL_DEBUG, "No match.");
	return ENOENT;
}

static errno_t vbds_part_by_svcid(service_id_t svcid, vbds_part_t **rpart)
{
	log_msg(LOG_DEFAULT, LVL_DEBUG, "vbds_part_by_svcid(%zu)", svcid);

	fibril_mutex_lock(&vbds_parts_lock);

	list_foreach(vbds_parts, lparts, vbds_part_t, part) {
		log_msg(LOG_DEFAULT, LVL_DEBUG, "%zu == %zu ?", part->svc_id, svcid);
		if (part->svc_id == svcid) {
			log_msg(LOG_DEFAULT, LVL_DEBUG, "Found match.");
			vbds_part_add_ref(part);
			fibril_mutex_unlock(&vbds_parts_lock);
			*rpart = part;
			return EOK;
		}
	}

	fibril_mutex_unlock(&vbds_parts_lock);
	log_msg(LOG_DEFAULT, LVL_DEBUG, "No match.");
	return ENOENT;
}

/** Add partition to our inventory based on liblabel partition structure */
static errno_t vbds_part_add(vbds_disk_t *disk, label_part_t *lpart,
    vbds_part_t **rpart)
{
	vbds_part_t *part;
	service_id_t psid = 0;
	label_part_info_t lpinfo;
	errno_t rc;

	log_msg(LOG_DEFAULT, LVL_DEBUG, "vbds_part_add(%s, %p)",
	    disk->svc_name, lpart);

	label_part_get_info(lpart, &lpinfo);

	part = calloc(1, sizeof(vbds_part_t));
	if (part == NULL) {
		log_msg(LOG_DEFAULT, LVL_ERROR, "Out of memory.");
		return ENOMEM;
	}

	fibril_rwlock_initialize(&part->lock);

	part->lpart = lpart;
	part->disk = disk;
	part->pid = ++vbds_part_id;
	part->svc_id = psid;
	part->block0 = lpinfo.block0;
	part->nblocks = lpinfo.nblocks;
	atomic_set(&part->refcnt, 1);

	bd_srvs_init(&part->bds);
	part->bds.ops = &vbds_bd_ops;
	part->bds.sarg = part;

	if (lpinfo.pkind != lpk_extended) {
		rc = vbds_part_svc_register(part);
		if (rc != EOK) {
			free(part);
			return EIO;
		}
	}

	list_append(&part->ldisk, &disk->parts);
	fibril_mutex_lock(&vbds_parts_lock);
	list_append(&part->lparts, &vbds_parts);
	fibril_mutex_unlock(&vbds_parts_lock);

	log_msg(LOG_DEFAULT, LVL_DEBUG, "vbds_part_add -> %p", part);

	if (rpart != NULL)
		*rpart = part;
	return EOK;
}

/** Remove partition from our inventory leaving only the underlying liblabel
 * partition structure.
 *
 * @param part   Partition
 * @param flag   If set to @c vrf_force, force removal even if partition is in use
 * @param rlpart Place to store pointer to liblabel partition
 *
 */
static errno_t vbds_part_remove(vbds_part_t *part, vbds_rem_flag_t flag,
    label_part_t **rlpart)
{
	label_part_t *lpart;
	errno_t rc;

	log_msg(LOG_DEFAULT, LVL_DEBUG, "vbds_part_remove(%p)", part);

	fibril_rwlock_write_lock(&part->lock);
	lpart = part->lpart;

	if ((flag & vrf_force) == 0 && part->open_cnt > 0) {
		fibril_rwlock_write_unlock(&part->lock);
		log_msg(LOG_DEFAULT, LVL_DEBUG, "part->open_cnt = %d",
		    part->open_cnt);
		return EBUSY;
	}

	if (part->svc_id != 0) {
		rc = vbds_part_svc_unregister(part);
		if (rc != EOK) {
			fibril_rwlock_write_unlock(&part->lock);
			return EIO;
		}
	}

	list_remove(&part->ldisk);
	fibril_mutex_lock(&vbds_parts_lock);
	list_remove(&part->lparts);
	fibril_mutex_unlock(&vbds_parts_lock);

	vbds_part_del_ref(part);
	part->lpart = NULL;
	fibril_rwlock_write_unlock(&part->lock);

	if (rlpart != NULL)
		*rlpart = lpart;
	return EOK;
}

/** Remove all disk partitions from our inventory leaving only the underlying
 * liblabel partition structures. */
static errno_t vbds_disk_parts_add(vbds_disk_t *disk, label_t *label)
{
	label_part_t *part;
	errno_t rc;

	part = label_part_first(label);
	while (part != NULL) {
		rc = vbds_part_add(disk, part, NULL);
		if (rc != EOK) {
			log_msg(LOG_DEFAULT, LVL_ERROR, "Failed adding partition "
			    "(disk %s)", disk->svc_name);
			return rc;
		}

		part = label_part_next(part);
	}

	return EOK;
}

/** Remove all disk partitions from our inventory leaving only the underlying
 * liblabel partition structures. */
static errno_t vbds_disk_parts_remove(vbds_disk_t *disk, vbds_rem_flag_t flag)
{
	link_t *link;
	vbds_part_t *part;
	errno_t rc;

	link = list_first(&disk->parts);
	while (link != NULL) {
		part = list_get_instance(link, vbds_part_t, ldisk);
		rc = vbds_part_remove(part, flag, NULL);
		if (rc != EOK)
			return rc;

		link = list_first(&disk->parts);
	}

	return EOK;
}

static void vbds_disk_cat_change_cb(void)
{
	(void) vbds_disks_check_new();
}

errno_t vbds_disk_discovery_start(void)
{
	errno_t rc;

	rc = loc_register_cat_change_cb(vbds_disk_cat_change_cb);
	if (rc != EOK) {
		log_msg(LOG_DEFAULT, LVL_ERROR, "Failed registering callback "
		    "for disk discovery: %s.", str_error(rc));
		return rc;
	}

	return vbds_disks_check_new();
}

errno_t vbds_disk_add(service_id_t sid)
{
	label_t *label = NULL;
	label_bd_t lbd;
	vbds_disk_t *disk = NULL;
	bool block_inited = false;
	size_t block_size;
	aoff64_t nblocks;
	errno_t rc;

	log_msg(LOG_DEFAULT, LVL_DEBUG, "vbds_disk_add(%zu)", sid);

	/* Check for duplicates */
	rc = vbds_disk_by_svcid(sid, &disk);
	if (rc == EOK)
		return EEXIST;

	disk = calloc(1, sizeof(vbds_disk_t));
	if (disk == NULL)
		return ENOMEM;

	/* Must be set before calling label_open */
	disk->svc_id = sid;

	rc = loc_service_get_name(sid, &disk->svc_name);
	if (rc != EOK) {
		log_msg(LOG_DEFAULT, LVL_ERROR, "Failed getting disk service name.");
		rc = EIO;
		goto error;
	}

	log_msg(LOG_DEFAULT, LVL_DEBUG, "block_init(%zu)", sid);
	rc = block_init(sid, 2048);
	if (rc != EOK) {
		log_msg(LOG_DEFAULT, LVL_ERROR, "Failed opening block device %s.",
		    disk->svc_name);
		rc = EIO;
		goto error;
	}

	rc = block_get_bsize(sid, &block_size);
	if (rc != EOK) {
		log_msg(LOG_DEFAULT, LVL_ERROR, "Failed getting block size of %s.",
		    disk->svc_name);
		rc = EIO;
		goto error;
	}

	rc = block_get_nblocks(sid, &nblocks);
	if (rc != EOK) {
		log_msg(LOG_DEFAULT, LVL_ERROR, "Failed getting number of "
		    "blocks of %s.", disk->svc_name);
		rc = EIO;
		goto error;
	}

	block_inited = true;

	lbd.ops = &vbds_label_bd_ops;
	lbd.arg = (void *) disk;

	rc = label_open(&lbd, &label);
	if (rc != EOK) {
		log_msg(LOG_DEFAULT, LVL_ERROR, "Failed to open label in disk %s.",
		    disk->svc_name);
		rc = EIO;
		goto error;
	}

	disk->label = label;
	disk->block_size = block_size;
	disk->nblocks = nblocks;
	disk->present = true;

	list_initialize(&disk->parts);
	list_append(&disk->ldisks, &vbds_disks);

	log_msg(LOG_DEFAULT, LVL_NOTE, "Recognized disk label. Adding partitions.");

	(void) vbds_disk_parts_add(disk, label);
	return EOK;
error:
	label_close(label);
	if (block_inited) {
		log_msg(LOG_DEFAULT, LVL_DEBUG, "block_fini(%zu)", sid);
		block_fini(sid);
	}
	if (disk != NULL)
		free(disk->svc_name);
	free(disk);
	return rc;
}

errno_t vbds_disk_remove(service_id_t sid)
{
	vbds_disk_t *disk;
	errno_t rc;

	log_msg(LOG_DEFAULT, LVL_DEBUG, "vbds_disk_remove(%zu)", sid);

	rc = vbds_disk_by_svcid(sid, &disk);
	if (rc != EOK)
		return rc;

	rc = vbds_disk_parts_remove(disk, vrf_force);
	if (rc != EOK) {
		log_msg(LOG_DEFAULT, LVL_ERROR, "Failed removing disk.");
		return rc;
	}

	list_remove(&disk->ldisks);
	label_close(disk->label);
	log_msg(LOG_DEFAULT, LVL_DEBUG, "block_fini(%zu)", sid);
	block_fini(sid);
	free(disk);
	return EOK;
}

/** Get list of disks as array of service IDs. */
errno_t vbds_disk_get_ids(service_id_t *id_buf, size_t buf_size, size_t *act_size)
{
	size_t act_cnt;
	size_t buf_cnt;

	fibril_mutex_lock(&vbds_disks_lock);

	buf_cnt = buf_size / sizeof(service_id_t);

	act_cnt = list_count(&vbds_disks);
	*act_size = act_cnt * sizeof(service_id_t);

	if (buf_size % sizeof(service_id_t) != 0) {
		fibril_mutex_unlock(&vbds_disks_lock);
		return EINVAL;
	}

	size_t pos = 0;
	list_foreach(vbds_disks, ldisks, vbds_disk_t, disk) {
		if (pos < buf_cnt)
			id_buf[pos] = disk->svc_id;
		pos++;
	}

	fibril_mutex_unlock(&vbds_disks_lock);
	return EOK;
}

errno_t vbds_disk_info(service_id_t sid, vbd_disk_info_t *info)
{
	vbds_disk_t *disk;
	label_info_t linfo;
	errno_t rc;

	log_msg(LOG_DEFAULT, LVL_DEBUG, "vbds_disk_info(%zu)", sid);

	rc = vbds_disk_by_svcid(sid, &disk);
	if (rc != EOK)
		return rc;

	rc = label_get_info(disk->label, &linfo);
	if (rc != EOK)
		return rc;

	info->ltype = linfo.ltype;
	info->flags = linfo.flags;
	info->ablock0 = linfo.ablock0;
	info->anblocks = linfo.anblocks;
	info->block_size = disk->block_size;
	info->nblocks = disk->nblocks;
	log_msg(LOG_DEFAULT, LVL_DEBUG, "vbds_disk_info - block_size=%zu",
	    info->block_size);
	return EOK;
}

errno_t vbds_get_parts(service_id_t sid, service_id_t *id_buf, size_t buf_size,
    size_t *act_size)
{
	vbds_disk_t *disk;
	size_t act_cnt;
	size_t buf_cnt;
	errno_t rc;

	rc = vbds_disk_by_svcid(sid, &disk);
	if (rc != EOK)
		return rc;

	buf_cnt = buf_size / sizeof(service_id_t);

	act_cnt = list_count(&disk->parts);
	*act_size = act_cnt * sizeof(service_id_t);

	if (buf_size % sizeof(service_id_t) != 0)
		return EINVAL;

	size_t pos = 0;
	list_foreach(disk->parts, ldisk, vbds_part_t, part) {
		if (pos < buf_cnt)
			id_buf[pos] = part->pid;
		pos++;
	}

	return EOK;
}

errno_t vbds_label_create(service_id_t sid, label_type_t ltype)
{
	label_t *label;
	label_bd_t lbd;
	label_info_t linfo;
	vbds_disk_t *disk;
	errno_t rc, rc2;

	log_msg(LOG_DEFAULT, LVL_DEBUG, "vbds_label_create(%zu)", sid);

	/* Find disk */
	rc = vbds_disk_by_svcid(sid, &disk);
	if (rc != EOK)
		return rc;

	log_msg(LOG_DEFAULT, LVL_DEBUG, "vbds_label_create(%zu) - label_close", sid);

	/* Verify that current label is a dummy label */
	rc = label_get_info(disk->label, &linfo);
	if (rc != EOK)
		return rc;

	if (linfo.ltype != lt_none) {
		log_msg(LOG_DEFAULT, LVL_ERROR, "Label already exists.");
		return EEXIST;
	}

	/* Close dummy label first */
	rc = vbds_disk_parts_remove(disk, vrf_none);
	if (rc != EOK) {
		log_msg(LOG_DEFAULT, LVL_ERROR, "Failed removing disk.");
		return rc;
	}

	label_close(disk->label);
	disk->label = NULL;

	log_msg(LOG_DEFAULT, LVL_DEBUG, "vbds_label_create(%zu) - label_create", sid);

	lbd.ops = &vbds_label_bd_ops;
	lbd.arg = (void *) disk;

	rc = label_create(&lbd, ltype, &label);
	if (rc != EOK)
		goto error;

	(void) vbds_disk_parts_add(disk, label);
	disk->label = label;

	log_msg(LOG_DEFAULT, LVL_DEBUG, "vbds_label_create(%zu) - success", sid);
	return EOK;
error:
	log_msg(LOG_DEFAULT, LVL_DEBUG, "vbds_label_create(%zu) - failure", sid);
	if (disk->label == NULL) {
		lbd.ops = &vbds_label_bd_ops;
		lbd.arg = (void *) disk;

		rc2 = label_open(&lbd, &label);
		if (rc2 != EOK) {
			log_msg(LOG_DEFAULT, LVL_ERROR, "Failed to open label in disk %s.",
			    disk->svc_name);
			return rc2;
		}

		disk->label = label;
		(void) vbds_disk_parts_add(disk, label);
	}

	return rc;
}

errno_t vbds_label_delete(service_id_t sid)
{
	vbds_disk_t *disk;
	label_t *label;
	label_bd_t lbd;
	errno_t rc;

	log_msg(LOG_DEFAULT, LVL_DEBUG, "vbds_label_delete(%zu)", sid);

	rc = vbds_disk_by_svcid(sid, &disk);
	if (rc != EOK)
		return rc;

	rc = vbds_disk_parts_remove(disk, vrf_none);
	if (rc != EOK) {
		log_msg(LOG_DEFAULT, LVL_ERROR, "Failed deleting label.");
		return rc;
	}

	rc = label_destroy(disk->label);
	if (rc != EOK) {
		log_msg(LOG_DEFAULT, LVL_ERROR, "Failed deleting label.");
		return rc;
	}

	disk->label = NULL;

	lbd.ops = &vbds_label_bd_ops;
	lbd.arg = (void *) disk;

	rc = label_open(&lbd, &label);
	if (rc != EOK) {
		log_msg(LOG_DEFAULT, LVL_ERROR, "Failed to open label in disk %s.",
		    disk->svc_name);
		return EIO;
	}

	(void) vbds_disk_parts_add(disk, label);
	disk->label = label;
	return EOK;
}

errno_t vbds_part_get_info(vbds_part_id_t partid, vbd_part_info_t *pinfo)
{
	vbds_part_t *part;
	label_part_info_t lpinfo;
	errno_t rc;

	rc = vbds_part_by_pid(partid, &part);
	if (rc != EOK)
		return rc;

	fibril_rwlock_read_lock(&part->lock);
	if (part->lpart == NULL) {
		fibril_rwlock_read_unlock(&part->lock);
		return ENOENT;
	}

	label_part_get_info(part->lpart, &lpinfo);

	pinfo->index = lpinfo.index;
	pinfo->pkind = lpinfo.pkind;
	pinfo->block0 = lpinfo.block0;
	pinfo->nblocks = lpinfo.nblocks;
	pinfo->svc_id = part->svc_id;
	vbds_part_del_ref(part);
	fibril_rwlock_read_unlock(&part->lock);

	return EOK;
}

errno_t vbds_part_create(service_id_t sid, vbd_part_spec_t *pspec,
    vbds_part_id_t *rpart)
{
	vbds_disk_t *disk;
	vbds_part_t *part;
	label_part_spec_t lpspec;
	label_part_t *lpart;
	errno_t rc;

	log_msg(LOG_DEFAULT, LVL_DEBUG, "vbds_part_create(%zu)", sid);

	rc = vbds_disk_by_svcid(sid, &disk);
	if (rc != EOK) {
		log_msg(LOG_DEFAULT, LVL_ERROR, "Disk %zu not found",
		    sid);
		goto error;
	}

	log_msg(LOG_DEFAULT, LVL_DEBUG, "vbds_part_crate(%zu): "
	    "index=%d block0=%" PRIu64 " nblocks=%" PRIu64
	    " hdr_blocks=%" PRIu64 " pkind=%d",
	    sid, pspec->index, pspec->block0, pspec->nblocks, pspec->hdr_blocks,
	    pspec->pkind);

	label_pspec_init(&lpspec);
	lpspec.index = pspec->index;
	lpspec.block0 = pspec->block0;
	lpspec.nblocks = pspec->nblocks;
	lpspec.hdr_blocks = pspec->hdr_blocks;
	lpspec.pkind = pspec->pkind;
	lpspec.ptype = pspec->ptype;

	rc = label_part_create(disk->label, &lpspec, &lpart);
	if (rc != EOK) {
		log_msg(LOG_DEFAULT, LVL_ERROR, "Error creating partition.");
		goto error;
	}

	rc = label_part_empty(lpart);
	if (rc != EOK) {
		log_msg(LOG_DEFAULT, LVL_ERROR, "Error emptying partition.");
		goto error;
	}

	rc = vbds_part_indices_update(disk);
	if (rc != EOK) {
		log_msg(LOG_DEFAULT, LVL_ERROR, "Error updating partition indices");
		goto error;
	}

	rc = vbds_part_add(disk, lpart, &part);
	if (rc != EOK) {
		log_msg(LOG_DEFAULT, LVL_ERROR, "Failed while creating "
		    "partition.");
		rc = label_part_destroy(lpart);
		if (rc != EOK)
			log_msg(LOG_DEFAULT, LVL_ERROR,
			    "Cannot roll back partition creation.");
		rc = EIO;
		goto error;
	}

	if (rpart != NULL)
		*rpart = part->pid;
	return EOK;
error:
	return rc;
}

errno_t vbds_part_delete(vbds_part_id_t partid)
{
	vbds_part_t *part;
	vbds_disk_t *disk;
	label_part_t *lpart;
	errno_t rc;

	rc = vbds_part_by_pid(partid, &part);
	if (rc != EOK)
		return rc;

	disk = part->disk;

	rc = vbds_part_remove(part, vrf_none, &lpart);
	if (rc != EOK)
		return rc;

	rc = label_part_destroy(lpart);
	vbds_part_del_ref(part);
	if (rc != EOK) {
		log_msg(LOG_DEFAULT, LVL_ERROR, "Failed deleting partition");

		/* Try rolling back */
		rc = vbds_part_add(disk, lpart, NULL);
		if (rc != EOK)
			log_msg(LOG_DEFAULT, LVL_ERROR, "Failed rolling back.");

		return EIO;
	}

	rc = vbds_part_indices_update(disk);
	if (rc != EOK) {
		log_msg(LOG_DEFAULT, LVL_ERROR, "Error updating partition indices");
		return EIO;
	}

	return EOK;
}

errno_t vbds_suggest_ptype(service_id_t sid, label_pcnt_t pcnt,
    label_ptype_t *ptype)
{
	vbds_disk_t *disk;
	errno_t rc;

	rc = vbds_disk_by_svcid(sid, &disk);
	if (rc != EOK) {
		log_msg(LOG_DEFAULT, LVL_DEBUG, "Disk %zu not found",
		    sid);
		goto error;
	}

	rc = label_suggest_ptype(disk->label, pcnt, ptype);
	if (rc != EOK) {
		log_msg(LOG_DEFAULT, LVL_DEBUG, "label_suggest_ptype() failed");
		goto error;
	}

	return EOK;
error:
	return rc;
}

static errno_t vbds_bd_open(bd_srvs_t *bds, bd_srv_t *bd)
{
	vbds_part_t *part = bd_srv_part(bd);

	log_msg(LOG_DEFAULT, LVL_DEBUG, "vbds_bd_open()");
	fibril_rwlock_write_lock(&part->lock);
	part->open_cnt++;
	fibril_rwlock_write_unlock(&part->lock);
	return EOK;
}

static errno_t vbds_bd_close(bd_srv_t *bd)
{
	vbds_part_t *part = bd_srv_part(bd);

	log_msg(LOG_DEFAULT, LVL_DEBUG, "vbds_bd_close()");

	/* Grabbing writer lock also forces all I/O to complete */

	fibril_rwlock_write_lock(&part->lock);
	part->open_cnt--;
	fibril_rwlock_write_unlock(&part->lock);
	return EOK;
}

static errno_t vbds_bd_read_blocks(bd_srv_t *bd, aoff64_t ba, size_t cnt,
    void *buf, size_t size)
{
	vbds_part_t *part = bd_srv_part(bd);
	aoff64_t gba;
	errno_t rc;

	log_msg(LOG_DEFAULT, LVL_DEBUG2, "vbds_bd_read_blocks()");
	fibril_rwlock_read_lock(&part->lock);

	if (cnt * part->disk->block_size < size) {
		fibril_rwlock_read_unlock(&part->lock);
		return EINVAL;
	}

	if (vbds_bsa_translate(part, ba, cnt, &gba) != EOK) {
		fibril_rwlock_read_unlock(&part->lock);
		return ELIMIT;
	}

	rc = block_read_direct(part->disk->svc_id, gba, cnt, buf);
	fibril_rwlock_read_unlock(&part->lock);

	return rc;
}

static errno_t vbds_bd_sync_cache(bd_srv_t *bd, aoff64_t ba, size_t cnt)
{
	vbds_part_t *part = bd_srv_part(bd);
	aoff64_t gba;
	errno_t rc;

	log_msg(LOG_DEFAULT, LVL_DEBUG2, "vbds_bd_sync_cache()");
	fibril_rwlock_read_lock(&part->lock);

	/* XXX Allow full-disk sync? */
	if (ba != 0 || cnt != 0) {
		if (vbds_bsa_translate(part, ba, cnt, &gba) != EOK) {
			fibril_rwlock_read_unlock(&part->lock);
			return ELIMIT;
		}
	} else {
		gba = 0;
	}

	rc = block_sync_cache(part->disk->svc_id, gba, cnt);
	fibril_rwlock_read_unlock(&part->lock);
	return rc;
}

static errno_t vbds_bd_write_blocks(bd_srv_t *bd, aoff64_t ba, size_t cnt,
    const void *buf, size_t size)
{
	vbds_part_t *part = bd_srv_part(bd);
	aoff64_t gba;
	errno_t rc;

	log_msg(LOG_DEFAULT, LVL_DEBUG2, "vbds_bd_write_blocks()");
	fibril_rwlock_read_lock(&part->lock);

	if (cnt * part->disk->block_size < size) {
		fibril_rwlock_read_unlock(&part->lock);
		return EINVAL;
	}

	if (vbds_bsa_translate(part, ba, cnt, &gba) != EOK) {
		fibril_rwlock_read_unlock(&part->lock);
		return ELIMIT;
	}

	rc = block_write_direct(part->disk->svc_id, gba, cnt, buf);
	fibril_rwlock_read_unlock(&part->lock);
	return rc;
}

static errno_t vbds_bd_get_block_size(bd_srv_t *bd, size_t *rsize)
{
	vbds_part_t *part = bd_srv_part(bd);

	log_msg(LOG_DEFAULT, LVL_DEBUG2, "vbds_bd_get_block_size()");

	fibril_rwlock_read_lock(&part->lock);
	*rsize = part->disk->block_size;
	fibril_rwlock_read_unlock(&part->lock);

	return EOK;
}

static errno_t vbds_bd_get_num_blocks(bd_srv_t *bd, aoff64_t *rnb)
{
	vbds_part_t *part = bd_srv_part(bd);

	log_msg(LOG_DEFAULT, LVL_DEBUG2, "vbds_bd_get_num_blocks()");

	fibril_rwlock_read_lock(&part->lock);
	*rnb = part->nblocks;
	fibril_rwlock_read_unlock(&part->lock);

	return EOK;
}

void vbds_bd_conn(ipc_callid_t iid, ipc_call_t *icall, void *arg)
{
	vbds_part_t *part;
	errno_t rc;
	service_id_t svcid;

	log_msg(LOG_DEFAULT, LVL_DEBUG, "vbds_bd_conn()");

	svcid = IPC_GET_ARG2(*icall);

	log_msg(LOG_DEFAULT, LVL_DEBUG, "vbds_bd_conn() - svcid=%zu", svcid);

	rc = vbds_part_by_svcid(svcid, &part);
	if (rc != EOK) {
		log_msg(LOG_DEFAULT, LVL_DEBUG, "vbd_bd_conn() - partition "
		    "not found.");
		async_answer_0(iid, EINVAL);
		return;
	}

	log_msg(LOG_DEFAULT, LVL_DEBUG, "vbds_bd_conn() - call bd_conn");
	bd_conn(iid, icall, &part->bds);
	vbds_part_del_ref(part);
}

/** Translate block segment address with range checking. */
static errno_t vbds_bsa_translate(vbds_part_t *part, aoff64_t ba, size_t cnt,
    aoff64_t *gba)
{
	if (ba + cnt > part->nblocks)
		return ELIMIT;

	*gba = part->block0 + ba;
	return EOK;
}

/** Register service for partition */
static errno_t vbds_part_svc_register(vbds_part_t *part)
{
	char *name;
	service_id_t psid;
	int idx;
	errno_t rc;

	idx = part->lpart->index;

	if (asprintf(&name, "%sp%u", part->disk->svc_name, idx) < 0) {
		log_msg(LOG_DEFAULT, LVL_ERROR, "Out of memory.");
		return ENOMEM;
	}

	log_msg(LOG_DEFAULT, LVL_DEBUG, "loc_service_register('%s')",
	    name);
	rc = loc_service_register(name, &psid);
	if (rc != EOK) {
		log_msg(LOG_DEFAULT, LVL_ERROR, "Failed registering "
		    "service %s: %s.", name, str_error(rc));
		free(name);
		free(part);
		return EIO;
	}

	rc = loc_service_add_to_cat(psid, part_cid);
	if (rc != EOK) {
		log_msg(LOG_DEFAULT, LVL_ERROR, "Failled adding partition "
		    "service %s to partition category.", name);
		free(name);
		free(part);

		rc = loc_service_unregister(psid);
		if (rc != EOK) {
			log_msg(LOG_DEFAULT, LVL_ERROR, "Error unregistering "
			    "service. Rollback failed.");
		}
		return EIO;
	}

	free(name);
	part->svc_id = psid;
	part->reg_idx = idx;
	return EOK;
}

/** Unregister service for partition */
static errno_t vbds_part_svc_unregister(vbds_part_t *part)
{
	errno_t rc;

	log_msg(LOG_DEFAULT, LVL_DEBUG, "vbds_part_svc_unregister("
	    "disk->svc_name='%s', id=%zu)", part->disk->svc_name, part->svc_id);

	rc = loc_service_unregister(part->svc_id);
	if (rc != EOK)
		return EIO;

	part->svc_id = 0;
	part->reg_idx = 0;
	return EOK;
}

/** Update service names for any partition whose index has changed. */
static errno_t vbds_part_indices_update(vbds_disk_t *disk)
{
	label_part_info_t lpinfo;
	errno_t rc;

	log_msg(LOG_DEFAULT, LVL_DEBUG, "vbds_part_indices_update()");

	fibril_mutex_lock(&vbds_parts_lock);

	/* First unregister services for partitions whose index has changed */
	list_foreach(disk->parts, ldisk, vbds_part_t, part) {
		fibril_rwlock_write_lock(&part->lock);
		if (part->svc_id != 0 && part->lpart->index != part->reg_idx) {
			rc = vbds_part_svc_unregister(part);
			if (rc != EOK) {
				fibril_rwlock_write_unlock(&part->lock);
				fibril_mutex_unlock(&vbds_parts_lock);
				log_msg(LOG_DEFAULT, LVL_ERROR, "Error "
				    "un-registering partition.");
				return EIO;
			}
		}

		fibril_rwlock_write_unlock(&part->lock);
	}

	/* Now re-register those services under the new indices */
	list_foreach(disk->parts, ldisk, vbds_part_t, part) {
		fibril_rwlock_write_lock(&part->lock);
		label_part_get_info(part->lpart, &lpinfo);
		if (part->svc_id == 0 && lpinfo.pkind != lpk_extended) {
			rc = vbds_part_svc_register(part);
			if (rc != EOK) {
				fibril_rwlock_write_unlock(&part->lock);
				fibril_mutex_unlock(&vbds_parts_lock);
				log_msg(LOG_DEFAULT, LVL_ERROR, "Error "
				    "re-registering partition.");
				return EIO;
			}
		}

		fibril_rwlock_write_unlock(&part->lock);
	}

	fibril_mutex_unlock(&vbds_parts_lock);

	return EOK;
}

/** Get block size wrapper for liblabel */
static errno_t vbds_label_get_bsize(void *arg, size_t *bsize)
{
	vbds_disk_t *disk = (vbds_disk_t *)arg;
	return block_get_bsize(disk->svc_id, bsize);
}

/** Get number of blocks wrapper for liblabel */
static errno_t vbds_label_get_nblocks(void *arg, aoff64_t *nblocks)
{
	vbds_disk_t *disk = (vbds_disk_t *)arg;
	return block_get_nblocks(disk->svc_id, nblocks);
}

/** Read blocks wrapper for liblabel */
static errno_t vbds_label_read(void *arg, aoff64_t ba, size_t cnt, void *buf)
{
	vbds_disk_t *disk = (vbds_disk_t *)arg;
	return block_read_direct(disk->svc_id, ba, cnt, buf);
}

/** Write blocks wrapper for liblabel */
static errno_t vbds_label_write(void *arg, aoff64_t ba, size_t cnt, const void *data)
{
	vbds_disk_t *disk = (vbds_disk_t *)arg;
	return block_write_direct(disk->svc_id, ba, cnt, data);
}

/** @}
 */
