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

/** @addtogroup vbd
 * @{
 */
/**
 * @file
 */

#include <adt/list.h>
#include <errno.h>
#include <io/log.h>
#include <stdlib.h>
#include <task.h>

#include "disk.h"
#include "types/vbd.h"

static list_t vbds_disks; /* of vbds_disk_t */

void vbds_disks_init(void)
{
	list_initialize(&vbds_disks);
}

static int vbds_disk_by_svcid(service_id_t sid, vbds_disk_t **rdisk)
{
	list_foreach(vbds_disks, ldisks, vbds_disk_t, disk) {
		if (disk->svc_id == sid) {
			*rdisk = disk;
			return EOK;
		}
	}

	return ENOENT;
}

static int vbds_part_add(vbds_disk_t *disk, label_part_t *lpart)
{
	vbds_part_t *part;

	log_msg(LOG_DEFAULT, LVL_NOTE, "vbds_part_add(%p, %p)",
	    disk, lpart);

	part = calloc(1, sizeof(vbds_part_t));
	if (part == NULL) {
		log_msg(LOG_DEFAULT, LVL_ERROR, "Out of memory.");
		return ENOMEM;
	}

	part->lpart = lpart;
	part->disk = disk;
	list_append(&part->ldisk, &disk->parts);

	return EOK;
}

int vbds_disk_add(service_id_t sid)
{
	label_t *label;
	label_part_t *part;
	vbds_disk_t *disk;
	int rc;

	log_msg(LOG_DEFAULT, LVL_NOTE, "vbds_disk_add(%zu)", sid);

	/* Check for duplicates */
	rc = vbds_disk_by_svcid(sid, &disk);
	if (rc == EOK)
		return EEXISTS;

	disk = calloc(1, sizeof(vbds_disk_t));
	if (disk == NULL)
		return ENOMEM;

	rc = label_open(sid, &label);
	if (rc != EOK)
		goto error;

	disk->svc_id = sid;
	disk->label = label;
	list_initialize(&disk->parts);
	list_append(&disk->ldisks, &vbds_disks);

	part = label_part_first(label);
	while (part != NULL) {
		rc = vbds_part_add(disk, part);
		if (rc != EOK) {
			log_msg(LOG_DEFAULT, LVL_ERROR, "Failed adding partition.");
		}

		part = label_part_next(part);
	}

	return EOK;
error:
	free(disk);
	return rc;
}

int vbds_disk_remove(service_id_t sid)
{
	vbds_disk_t *disk;
	int rc;

	log_msg(LOG_DEFAULT, LVL_NOTE, "vbds_disk_remove(%zu)", sid);

	rc = vbds_disk_by_svcid(sid, &disk);
	if (rc != EOK)
		return rc;

	list_remove(&disk->ldisks);
	label_close(disk->label);
	free(disk);
	return EOK;
}

int vbds_disk_info(service_id_t sid, vbds_disk_info_t *info)
{
	log_msg(LOG_DEFAULT, LVL_NOTE, "vbds_disk_info(%zu)", sid);
	info->ltype = lt_mbr;
	return EOK;
}

int vbds_label_create(service_id_t sid, label_type_t ltype)
{
	label_t *label;
	vbds_disk_t *disk;
	int rc;

	log_msg(LOG_DEFAULT, LVL_NOTE, "vbds_label_create(%zu)", sid);

	/* Check for duplicates */
	rc = vbds_disk_by_svcid(sid, &disk);
	if (rc == EOK)
		return EEXISTS;

	disk = calloc(1, sizeof(vbds_disk_t));
	if (disk == NULL)
		return ENOMEM;

	rc = label_create(sid, ltype, &label);
	if (rc != EOK)
		goto error;

	disk->svc_id = sid;
	disk->label = label;
	list_append(&disk->ldisks, &vbds_disks);
	return EOK;
error:
	free(disk);
	return rc;
}

int vbds_label_delete(service_id_t sid)
{
	return EOK;
	vbds_disk_t *disk;
	int rc;

	log_msg(LOG_DEFAULT, LVL_NOTE, "vbds_label_delete(%zu)", sid);

	rc = vbds_disk_by_svcid(sid, &disk);
	if (rc != EOK)
		return rc;

	rc = label_destroy(disk->label);
	if (rc != EOK) {
		log_msg(LOG_DEFAULT, LVL_ERROR, "Failed deleting label.");
		return rc;
	}

	list_remove(&disk->ldisks);
	free(disk);
	return EOK;
}

int vbds_part_get_info(vbds_part_id_t part, vbds_part_info_t *pinfo)
{
	return EOK;
}

int vbds_part_create(service_id_t disk, vbds_part_id_t *rpart)
{
	return EOK;
}

int vbds_part_delete(vbds_part_id_t part)
{
	return EOK;
}

/** @}
 */
