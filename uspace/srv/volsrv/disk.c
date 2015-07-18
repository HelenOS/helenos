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
 * @file Disk device handling
 * @brief
 */

#include <stdbool.h>
#include <errno.h>
#include <fibril_synch.h>
#include <io/log.h>
#include <loc.h>
#include <stdlib.h>
#include <str.h>
#include <vbd.h>

#include "disk.h"
#include "types/disk.h"

static int vol_disk_add(service_id_t);

static LIST_INITIALIZE(vol_disks); /* of vol_disk_t */
static FIBRIL_MUTEX_INITIALIZE(vol_disks_lock);
static vbd_t *vbd;

/** Check for new disk devices */
static int vol_disk_check_new(void)
{
	bool already_known;
	category_id_t disk_cat;
	service_id_t *svcs;
	size_t count, i;
	int rc;

	fibril_mutex_lock(&vol_disks_lock);

	rc = loc_category_get_id("disk", &disk_cat, IPC_FLAG_BLOCKING);
	if (rc != EOK) {
		log_msg(LOG_DEFAULT, LVL_ERROR, "Failed resolving category 'disk'.");
		fibril_mutex_unlock(&vol_disks_lock);
		return ENOENT;
	}

	rc = loc_category_get_svcs(disk_cat, &svcs, &count);
	if (rc != EOK) {
		log_msg(LOG_DEFAULT, LVL_ERROR, "Failed getting list of disk "
		    "devices.");
		fibril_mutex_unlock(&vol_disks_lock);
		return EIO;
	}

	for (i = 0; i < count; i++) {
		already_known = false;

		list_foreach(vol_disks, ldisks, vol_disk_t, disk) {
			if (disk->svc_id == svcs[i]) {
				already_known = true;
				break;
			}
		}

		if (!already_known) {
			log_msg(LOG_DEFAULT, LVL_NOTE, "Found disk '%lu'",
			    (unsigned long) svcs[i]);
			rc = vol_disk_add(svcs[i]);
			if (rc != EOK) {
				log_msg(LOG_DEFAULT, LVL_ERROR, "Could not add "
				    "disk.");
			}
		}
	}

	fibril_mutex_unlock(&vol_disks_lock);
	return EOK;
}

static vol_disk_t *vol_disk_new(void)
{
	vol_disk_t *disk = calloc(1, sizeof(vol_disk_t));

	if (disk == NULL) {
		log_msg(LOG_DEFAULT, LVL_ERROR, "Failed allocating disk "
		    "structure. Out of memory.");
		return NULL;
	}

	link_initialize(&disk->ldisks);
	disk->dcnt = dc_empty;

	return disk;
}

static void vol_disk_delete(vol_disk_t *disk)
{
	if (disk == NULL)
		return;

	free(disk->svc_name);
	free(disk);
}

static int vol_disk_add(service_id_t sid)
{
	vol_disk_t *disk;
	vbd_disk_info_t dinfo;
	int rc;

	assert(fibril_mutex_is_locked(&vol_disks_lock));

	log_msg(LOG_DEFAULT, LVL_NOTE, "vol_disk_add()");
	disk = vol_disk_new();
	if (disk == NULL)
		return ENOMEM;

	disk->svc_id = sid;

	rc = loc_service_get_name(sid, &disk->svc_name);
	if (rc != EOK) {
		log_msg(LOG_DEFAULT, LVL_ERROR, "Failed getting service name.");
		goto error;
	}

	log_msg(LOG_DEFAULT, LVL_NOTE, "Probe disk %s", disk->svc_name);

	rc = vbd_disk_add(vbd, sid);
	if (rc == EOK) {
		log_msg(LOG_DEFAULT, LVL_NOTE, "Disk %s accepted by VBD.",
		    disk->svc_name);

		rc = vbd_disk_info(vbd, sid, &dinfo);
		log_msg(LOG_DEFAULT, LVL_NOTE, "Got disk info.");
		if (rc != EOK) {
			log_msg(LOG_DEFAULT, LVL_NOTE, "Cannot get disk label "
			    "information.");
			rc = EIO;
			goto error;
		}

		disk->dcnt = dc_label;
		disk->ltype = dinfo.ltype;
	} else {
		log_msg(LOG_DEFAULT, LVL_NOTE, "Disk %s not accepted by VBD.",
		    disk->svc_name);
		disk->dcnt = dc_unknown;
	}

	list_append(&disk->ldisks, &vol_disks);

	return EOK;

error:
	vol_disk_delete(disk);
	return rc;
}

static void vol_disk_cat_change_cb(void)
{
	(void) vol_disk_check_new();
}

int vol_disk_init(void)
{
	int rc;

	rc = vbd_create(&vbd);
	if (rc != EOK) {
		log_msg(LOG_DEFAULT, LVL_ERROR, "Failed initializing VBD.");
		return EIO;
	}

	return EOK;
}

int vol_disk_discovery_start(void)
{
	int rc;

	rc = loc_register_cat_change_cb(vol_disk_cat_change_cb);
	if (rc != EOK) {
		log_msg(LOG_DEFAULT, LVL_ERROR, "Failed registering callback "
		    "for disk discovery (%d).", rc);
		return rc;
	}

	return vol_disk_check_new();
}

/** Get list of disks as array of service IDs. */
int vol_disk_get_ids(service_id_t *id_buf, size_t buf_size, size_t *act_size)
{
	size_t act_cnt;
	size_t buf_cnt;

	fibril_mutex_lock(&vol_disks_lock);

	buf_cnt = buf_size / sizeof(service_id_t);

	act_cnt = list_count(&vol_disks);
	*act_size = act_cnt * sizeof(service_id_t);

	if (buf_size % sizeof(service_id_t) != 0) {
		fibril_mutex_unlock(&vol_disks_lock);
		return EINVAL;
	}

	size_t pos = 0;
	list_foreach(vol_disks, ldisks, vol_disk_t, disk) {
		if (pos < buf_cnt)
			id_buf[pos] = disk->svc_id;
		pos++;
	}

	fibril_mutex_unlock(&vol_disks_lock);
	return EOK;
}

int vol_disk_find_by_id(service_id_t sid, vol_disk_t **rdisk)
{
	list_foreach(vol_disks, ldisks, vol_disk_t, disk) {
		if (disk->svc_id == sid) {
			*rdisk = disk;
			/* XXX Add reference */
			return EOK;
		}
	}

	return ENOENT;
}

int vol_disk_label_create(vol_disk_t *disk, label_type_t ltype)
{
	int rc;

	rc = vbd_label_create(vbd, disk->svc_id, ltype);
	if (rc != EOK)
		return rc;

	disk->dcnt = dc_label;
	disk->ltype = ltype;

	return EOK;
}

int vol_disk_empty_disk(vol_disk_t *disk)
{
	int rc;

	if (disk->dcnt == dc_label) {
		rc = vbd_label_delete(vbd, disk->svc_id);
		if (rc != EOK)
			return rc;
	}

	disk->dcnt = dc_empty;

	return EOK;
}

int vol_disk_get_info(vol_disk_t *disk, vol_disk_info_t *dinfo)
{
	vbd_disk_info_t vdinfo;
	int rc;

	dinfo->dcnt = disk->dcnt;

	if (disk->dcnt == dc_label) {
		rc = vbd_disk_info(vbd, disk->svc_id, &vdinfo);
		if (rc != EOK)
			return rc;

		dinfo->ltype = vdinfo.ltype;
		dinfo->flags = vdinfo.flags;
	}

	return EOK;
}


/** @}
 */
