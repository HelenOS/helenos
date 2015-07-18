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
#include <bd_srv.h>
#include <block.h>
#include <errno.h>
#include <io/log.h>
#include <loc.h>
#include <stdio.h>
#include <stdlib.h>
#include <task.h>
#include <vbd.h>

#include "disk.h"
#include "types/vbd.h"

static list_t vbds_disks; /* of vbds_disk_t */
static list_t vbds_parts; /* of vbds_part_t */

static int vbds_bd_open(bd_srvs_t *, bd_srv_t *);
static int vbds_bd_close(bd_srv_t *);
static int vbds_bd_read_blocks(bd_srv_t *, aoff64_t, size_t, void *, size_t);
static int vbds_bd_sync_cache(bd_srv_t *, aoff64_t, size_t);
static int vbds_bd_write_blocks(bd_srv_t *, aoff64_t, size_t, const void *,
    size_t);
static int vbds_bd_get_block_size(bd_srv_t *, size_t *);
static int vbds_bd_get_num_blocks(bd_srv_t *, aoff64_t *);

static int vbds_bsa_translate(vbds_part_t *, aoff64_t, size_t, aoff64_t *);

static bd_ops_t vbds_bd_ops = {
	.open = vbds_bd_open,
	.close = vbds_bd_close,
	.read_blocks = vbds_bd_read_blocks,
	.sync_cache = vbds_bd_sync_cache,
	.write_blocks = vbds_bd_write_blocks,
	.get_block_size = vbds_bd_get_block_size,
	.get_num_blocks = vbds_bd_get_num_blocks
};

static vbds_part_t *bd_srv_part(bd_srv_t *bd)
{
	return (vbds_part_t *)bd->srvs->sarg;
}

void vbds_disks_init(void)
{
	list_initialize(&vbds_disks);
	list_initialize(&vbds_parts);
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

static int vbds_part_by_id(vbds_part_id_t partid, vbds_part_t **rpart)
{
	log_msg(LOG_DEFAULT, LVL_NOTE, "vbds_part_by_id(%zu)", partid);

	list_foreach(vbds_parts, lparts, vbds_part_t, part) {
		log_msg(LOG_DEFAULT, LVL_NOTE, "%zu == %zu ?", part->id, partid);
		if (part->id == partid) {
			log_msg(LOG_DEFAULT, LVL_NOTE, "Found match.");
			*rpart = part;
			return EOK;
		}
	}

	log_msg(LOG_DEFAULT, LVL_NOTE, "No match.");
	return ENOENT;
}

/** Add partition to our inventory based on liblabel partition structure */
static int vbds_part_add(vbds_disk_t *disk, label_part_t *lpart,
    vbds_part_t **rpart)
{
	vbds_part_t *part;
	service_id_t psid;
	label_part_info_t lpinfo;
	char *name;
	int rc;

	log_msg(LOG_DEFAULT, LVL_NOTE, "vbds_part_add(%s, %p)",
	    disk->svc_name, lpart);

	label_part_get_info(lpart, &lpinfo);

	part = calloc(1, sizeof(vbds_part_t));
	if (part == NULL) {
		log_msg(LOG_DEFAULT, LVL_ERROR, "Out of memory.");
		return ENOMEM;
	}

	/* XXX Proper service name */
	rc = asprintf(&name, "%sp%u", disk->svc_name, lpart->index);
	if (rc < 0) {
		log_msg(LOG_DEFAULT, LVL_ERROR, "Out of memory.");
		return ENOMEM;
	}

	rc = loc_service_register(name, &psid);
	if (rc != EOK) {
		log_msg(LOG_DEFAULT, LVL_ERROR, "Failed registering "
		    "service %s.", name);
		free(name);
		free(part);
		return EIO;
	}

	free(name);

	part->lpart = lpart;
	part->disk = disk;
	part->id = (vbds_part_id_t)psid;
	part->block0 = lpinfo.block0;
	part->nblocks = lpinfo.nblocks;

	bd_srvs_init(&part->bds);
	part->bds.ops = &vbds_bd_ops;
	part->bds.sarg = part;

	list_append(&part->ldisk, &disk->parts);
	list_append(&part->lparts, &vbds_parts);

	if (rpart != NULL)
		*rpart = part;
	return EOK;
}

/** Remove partition from our inventory leaving only the underlying liblabel
 * partition structure.
 */
static int vbds_part_remove(vbds_part_t *part, label_part_t **rlpart)
{
	label_part_t *lpart;
	int rc;

	log_msg(LOG_DEFAULT, LVL_NOTE, "vbds_part_remove(%p)", part);

	lpart = part->lpart;

	if (part->open_cnt > 0)
		return EBUSY;

	rc = loc_service_unregister((service_id_t)part->id);
	if (rc != EOK)
		return EIO;

	list_remove(&part->ldisk);
	list_remove(&part->lparts);
	free(part);

	if (rlpart != NULL)
		*rlpart = lpart;
	return EOK;
}

int vbds_disk_add(service_id_t sid)
{
	label_t *label = NULL;
	label_part_t *part;
	vbds_disk_t *disk = NULL;
	bool block_inited = false;
	size_t block_size;
	int rc;

	log_msg(LOG_DEFAULT, LVL_NOTE, "vbds_disk_add(%zu)", sid);

	/* Check for duplicates */
	rc = vbds_disk_by_svcid(sid, &disk);
	if (rc == EOK)
		return EEXISTS;

	disk = calloc(1, sizeof(vbds_disk_t));
	if (disk == NULL)
		return ENOMEM;

	rc = loc_service_get_name(sid, &disk->svc_name);
	if (rc != EOK) {
		log_msg(LOG_DEFAULT, LVL_ERROR, "Failed getting disk service name.");
		rc = EIO;
		goto error;
	}

	log_msg(LOG_DEFAULT, LVL_NOTE, "block_init(%zu)", sid);
	rc = block_init(EXCHANGE_SERIALIZE, sid, 2048);
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

	block_inited = true;

	rc = label_open(sid, &label);
	if (rc != EOK) {
		log_msg(LOG_DEFAULT, LVL_NOTE, "Label in disk %s not recognized.",
		    disk->svc_name);
		rc = EIO;
		goto error;
	}

	disk->svc_id = sid;
	disk->label = label;
	disk->block_size = block_size;

	list_initialize(&disk->parts);
	list_append(&disk->ldisks, &vbds_disks);

	log_msg(LOG_DEFAULT, LVL_NOTE, "Recognized disk label. Adding partitions.");

	part = label_part_first(label);
	while (part != NULL) {
		rc = vbds_part_add(disk, part, NULL);
		if (rc != EOK) {
			log_msg(LOG_DEFAULT, LVL_ERROR, "Failed adding partitio "
			    "(disk %s)", disk->svc_name);
		}

		part = label_part_next(part);
	}

	return EOK;
error:
	label_close(label);
	if (block_inited) {
		log_msg(LOG_DEFAULT, LVL_NOTE, "block_fini(%zu)", sid);
		block_fini(sid);
	}
	if (disk != NULL)
		free(disk->svc_name);
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
	log_msg(LOG_DEFAULT, LVL_NOTE, "block_fini(%zu)", sid);
	block_fini(sid);
	free(disk);
	return EOK;
}

int vbds_disk_info(service_id_t sid, vbd_disk_info_t *info)
{
	vbds_disk_t *disk;
	label_info_t linfo;
	int rc;

	log_msg(LOG_DEFAULT, LVL_NOTE, "vbds_disk_info(%zu)", sid);

	rc = vbds_disk_by_svcid(sid, &disk);
	if (rc != EOK)
		return rc;

	rc = label_get_info(disk->label, &linfo);

	info->ltype = linfo.ltype;
	info->flags = linfo.flags;
	info->ablock0 = linfo.ablock0;
	info->anblocks = linfo.anblocks;
	info->block_size = disk->block_size;
	return EOK;
}

int vbds_get_parts(service_id_t sid, service_id_t *id_buf, size_t buf_size,
    size_t *act_size)
{
	vbds_disk_t *disk;
	size_t act_cnt;
	size_t buf_cnt;
	int rc;

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
			id_buf[pos] = part->id;
		pos++;
	}

	return EOK;
}


int vbds_label_create(service_id_t sid, label_type_t ltype)
{
	label_t *label;
	vbds_disk_t *disk;
	bool block_inited = false;
	size_t block_size;
	int rc;

	log_msg(LOG_DEFAULT, LVL_NOTE, "vbds_label_create(%zu)", sid);

	log_msg(LOG_DEFAULT, LVL_NOTE, "vbds_label_create(%zu) - chkdup", sid);

	/* Check for duplicates */
	rc = vbds_disk_by_svcid(sid, &disk);
	if (rc == EOK)
		return EEXISTS;

	log_msg(LOG_DEFAULT, LVL_NOTE, "vbds_label_create(%zu) - alloc", sid);

	disk = calloc(1, sizeof(vbds_disk_t));
	if (disk == NULL)
		return ENOMEM;

	rc = loc_service_get_name(sid, &disk->svc_name);
	if (rc != EOK) {
		log_msg(LOG_DEFAULT, LVL_ERROR, "Failed getting disk service name.");
		rc = EIO;
		goto error;
	}

	log_msg(LOG_DEFAULT, LVL_NOTE, "block_init(%zu)", sid);
	rc = block_init(EXCHANGE_SERIALIZE, sid, 2048);
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

	block_inited = true;

	log_msg(LOG_DEFAULT, LVL_NOTE, "vbds_label_create(%zu) - label_create", sid);

	rc = label_create(sid, ltype, &label);
	if (rc != EOK)
		goto error;

	disk->svc_id = sid;
	disk->label = label;
	disk->block_size = block_size;
	list_initialize(&disk->parts);

	list_append(&disk->ldisks, &vbds_disks);

	log_msg(LOG_DEFAULT, LVL_NOTE, "vbds_label_create(%zu) - success", sid);
	return EOK;
error:
	log_msg(LOG_DEFAULT, LVL_NOTE, "vbds_label_create(%zu) - failure", sid);
	if (block_inited) {
		log_msg(LOG_DEFAULT, LVL_NOTE, "block_fini(%zu)", sid);
		block_fini(sid);
	}
	if (disk != NULL)
		free(disk->svc_name);
	free(disk);
	return rc;
}

int vbds_label_delete(service_id_t sid)
{
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
	log_msg(LOG_DEFAULT, LVL_NOTE, "block_fini(%zu)", sid);
	block_fini(sid);
	free(disk);
	return EOK;
}

int vbds_part_get_info(vbds_part_id_t partid, vbd_part_info_t *pinfo)
{
	vbds_part_t *part;
	label_part_info_t lpinfo;
	int rc;

	rc = vbds_part_by_id(partid, &part);
	if (rc != EOK)
		return rc;

	label_part_get_info(part->lpart, &lpinfo);

	pinfo->index = lpinfo.index;
	pinfo->pkind = lpinfo.pkind;
	pinfo->block0 = lpinfo.block0;
	pinfo->nblocks = lpinfo.nblocks;
	return EOK;
}

int vbds_part_create(service_id_t sid, vbd_part_spec_t *pspec,
    vbds_part_id_t *rpart)
{
	vbds_disk_t *disk;
	vbds_part_t *part;
	label_part_spec_t lpspec;
	label_part_t *lpart;
	int rc;

	rc = vbds_disk_by_svcid(sid, &disk);
	if (rc != EOK) {
		log_msg(LOG_DEFAULT, LVL_NOTE, "Partition %zu not found",
		    sid);
		goto error;
	}

	label_pspec_init(&lpspec);
	lpspec.index = pspec->index;
	lpspec.block0 = pspec->block0;
	lpspec.nblocks = pspec->nblocks;
	lpspec.pkind = pspec->pkind;
	lpspec.ptype = pspec->ptype;

	rc = label_part_create(disk->label, &lpspec, &lpart);
	if (rc != EOK) {
		log_msg(LOG_DEFAULT, LVL_ERROR, "Error creating partition.");
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
		*rpart = part->id;
	return EOK;
error:
	return rc;
}

int vbds_part_delete(vbds_part_id_t partid)
{
	vbds_part_t *part;
	vbds_disk_t *disk;
	label_part_t *lpart;
	int rc;

	rc = vbds_part_by_id(partid, &part);
	if (rc != EOK)
		return rc;

	disk = part->disk;

	rc = vbds_part_remove(part, &lpart);
	if (rc != EOK)
		return rc;

	rc = label_part_destroy(lpart);
	if (rc != EOK) {
		log_msg(LOG_DEFAULT, LVL_ERROR, "Failed deleting partition");

		/* Try rolling back */
		rc = vbds_part_add(disk, lpart, NULL);
		if (rc != EOK)
			log_msg(LOG_DEFAULT, LVL_ERROR, "Failed rolling back.");

		return EIO;
	}

	return EOK;
}

static int vbds_bd_open(bd_srvs_t *bds, bd_srv_t *bd)
{
	vbds_part_t *part = bd_srv_part(bd);

	log_msg(LOG_DEFAULT, LVL_NOTE, "vbds_bd_open()");
	part->open_cnt++;
	return EOK;
}

static int vbds_bd_close(bd_srv_t *bd)
{
	vbds_part_t *part = bd_srv_part(bd);

	log_msg(LOG_DEFAULT, LVL_NOTE, "vbds_bd_close()");
	part->open_cnt--;
	return EOK;
}

static int vbds_bd_read_blocks(bd_srv_t *bd, aoff64_t ba, size_t cnt,
    void *buf, size_t size)
{
	vbds_part_t *part = bd_srv_part(bd);
	aoff64_t gba;

	log_msg(LOG_DEFAULT, LVL_NOTE, "vbds_bd_read_blocks()");

	if (cnt * part->disk->block_size < size)
		return EINVAL;

	if (vbds_bsa_translate(part, ba, cnt, &gba) != EOK)
		return ELIMIT;

	return block_read_direct(part->disk->svc_id, gba, cnt, buf);
}

static int vbds_bd_sync_cache(bd_srv_t *bd, aoff64_t ba, size_t cnt)
{
	vbds_part_t *part = bd_srv_part(bd);
	aoff64_t gba;

	log_msg(LOG_DEFAULT, LVL_NOTE, "vbds_bd_sync_cache()");

	/* XXX Allow full-disk sync? */
	if (ba != 0 || cnt != 0) {
		if (vbds_bsa_translate(part, ba, cnt, &gba) != EOK)
			return ELIMIT;
	} else {
		gba = 0;
	}

	return block_sync_cache(part->disk->svc_id, gba, cnt);
}

static int vbds_bd_write_blocks(bd_srv_t *bd, aoff64_t ba, size_t cnt,
    const void *buf, size_t size)
{
	vbds_part_t *part = bd_srv_part(bd);
	aoff64_t gba;

	log_msg(LOG_DEFAULT, LVL_NOTE, "vbds_bd_write_blocks()");

	if (cnt * part->disk->block_size < size)
		return EINVAL;

	if (vbds_bsa_translate(part, ba, cnt, &gba) != EOK)
		return ELIMIT;

	return block_write_direct(part->disk->svc_id, gba, cnt, buf);
}

static int vbds_bd_get_block_size(bd_srv_t *bd, size_t *rsize)
{
	vbds_part_t *part = bd_srv_part(bd);

	log_msg(LOG_DEFAULT, LVL_NOTE, "vbds_bd_get_block_size()");
	*rsize = part->disk->block_size;
	return EOK;
}

static int vbds_bd_get_num_blocks(bd_srv_t *bd, aoff64_t *rnb)
{
	vbds_part_t *part = bd_srv_part(bd);

	log_msg(LOG_DEFAULT, LVL_NOTE, "vbds_bd_get_num_blocks()");
	*rnb = part->nblocks;
	return EOK;
}

void vbds_bd_conn(ipc_callid_t iid, ipc_call_t *icall, void *arg)
{
	vbds_part_t *part;
	int rc;
	service_id_t partid;

	log_msg(LOG_DEFAULT, LVL_NOTE, "vbds_bd_conn()");

	partid = IPC_GET_ARG1(*icall);

	log_msg(LOG_DEFAULT, LVL_NOTE, "vbds_bd_conn() - partid=%zu", partid);

	rc = vbds_part_by_id(partid, &part);
	if (rc != EOK) {
		log_msg(LOG_DEFAULT, LVL_NOTE, "vbd_bd_conn() - partition "
		    "not found.");
		async_answer_0(iid, EINVAL);
		return;
	}

	log_msg(LOG_DEFAULT, LVL_NOTE, "vbds_bd_conn() - call bd_conn");
	bd_conn(iid, icall, &part->bds);
}

/** Translate block segment address with range checking. */
static int vbds_bsa_translate(vbds_part_t *part, aoff64_t ba, size_t cnt,
    aoff64_t *gba)
{
	if (ba + cnt > part->nblocks)
		return ELIMIT;

	*gba = part->block0 + ba;
	return EOK;
}

/** @}
 */
