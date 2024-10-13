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

/** @addtogroup libfdisk
 * @{
 */
/**
 * @file Disk management library.
 */

#include <adt/list.h>
#include <capa.h>
#include <errno.h>
#include <fdisk.h>
#include <loc.h>
#include <macros.h>
#include <mem.h>
#include <stdlib.h>
#include <str.h>
#include <vbd.h>
#include <vol.h>

static errno_t fdisk_dev_add_parts(fdisk_dev_t *);
static void fdisk_dev_remove_parts(fdisk_dev_t *);
static errno_t fdisk_part_spec_prepare(fdisk_dev_t *, fdisk_part_spec_t *,
    vbd_part_spec_t *);
static void fdisk_pri_part_insert_lists(fdisk_dev_t *, fdisk_part_t *);
static void fdisk_log_part_insert_lists(fdisk_dev_t *, fdisk_part_t *);
static errno_t fdisk_update_dev_info(fdisk_dev_t *);
static uint64_t fdisk_ba_align_up(fdisk_dev_t *, uint64_t);
static uint64_t fdisk_ba_align_down(fdisk_dev_t *, uint64_t);
static errno_t fdisk_part_get_max_free_range(fdisk_dev_t *, fdisk_spc_t, aoff64_t *,
    aoff64_t *);
static void fdisk_free_range_first(fdisk_dev_t *, fdisk_spc_t, fdisk_free_range_t *);
static bool fdisk_free_range_next(fdisk_free_range_t *);
static bool fdisk_free_range_get(fdisk_free_range_t *, aoff64_t *, aoff64_t *);

static void fdisk_dev_info_delete(fdisk_dev_info_t *info)
{
	if (info == NULL)
		return;

	free(info->svcname);
	free(info);
}

errno_t fdisk_create(fdisk_t **rfdisk)
{
	fdisk_t *fdisk = NULL;
	errno_t rc;

	fdisk = calloc(1, sizeof(fdisk_t));
	if (fdisk == NULL) {
		rc = ENOMEM;
		goto error;
	}

	rc = vol_create(&fdisk->vol);
	if (rc != EOK) {
		rc = EIO;
		goto error;
	}

	rc = vbd_create(&fdisk->vbd);
	if (rc != EOK) {
		rc = EIO;
		goto error;
	}

	*rfdisk = fdisk;
	return EOK;
error:
	fdisk_destroy(fdisk);

	return rc;
}

void fdisk_destroy(fdisk_t *fdisk)
{
	if (fdisk == NULL)
		return;

	vol_destroy(fdisk->vol);
	vbd_destroy(fdisk->vbd);
	free(fdisk);
}

errno_t fdisk_dev_list_get(fdisk_t *fdisk, fdisk_dev_list_t **rdevlist)
{
	fdisk_dev_list_t *devlist = NULL;
	fdisk_dev_info_t *info;
	service_id_t *svcs = NULL;
	size_t count, i;
	errno_t rc;

	devlist = calloc(1, sizeof(fdisk_dev_list_t));
	if (devlist == NULL)
		return ENOMEM;

	devlist->fdisk = fdisk;
	list_initialize(&devlist->devinfos);

	rc = vbd_get_disks(fdisk->vbd, &svcs, &count);
	if (rc != EOK) {
		rc = EIO;
		goto error;
	}

	for (i = 0; i < count; i++) {
		info = calloc(1, sizeof(fdisk_dev_info_t));
		if (info == NULL) {
			rc = ENOMEM;
			goto error;
		}

		info->svcid = svcs[i];
		info->devlist = devlist;
		list_append(&info->ldevlist, &devlist->devinfos);
	}

	*rdevlist = devlist;
	free(svcs);
	return EOK;
error:
	free(svcs);
	fdisk_dev_list_free(devlist);
	return rc;
}

void fdisk_dev_list_free(fdisk_dev_list_t *devlist)
{
	fdisk_dev_info_t *info;

	if (devlist == NULL)
		return;

	while (!list_empty(&devlist->devinfos)) {
		info = list_get_instance(list_first(
		    &devlist->devinfos), fdisk_dev_info_t,
		    ldevlist);

		list_remove(&info->ldevlist);
		fdisk_dev_info_delete(info);
	}

	free(devlist);
}

fdisk_dev_info_t *fdisk_dev_first(fdisk_dev_list_t *devlist)
{
	if (list_empty(&devlist->devinfos))
		return NULL;

	return list_get_instance(list_first(&devlist->devinfos),
	    fdisk_dev_info_t, ldevlist);
}

fdisk_dev_info_t *fdisk_dev_next(fdisk_dev_info_t *devinfo)
{
	link_t *lnext;

	lnext = list_next(&devinfo->ldevlist,
	    &devinfo->devlist->devinfos);
	if (lnext == NULL)
		return NULL;

	return list_get_instance(lnext, fdisk_dev_info_t,
	    ldevlist);
}

void fdisk_dev_info_get_svcid(fdisk_dev_info_t *info, service_id_t *rsid)
{
	*rsid = info->svcid;
}

errno_t fdisk_dev_info_get_svcname(fdisk_dev_info_t *info, char **rname)
{
	char *name;
	errno_t rc;

	if (info->svcname == NULL) {
		rc = loc_service_get_name(info->svcid,
		    &info->svcname);
		if (rc != EOK)
			return rc;
	}

	name = str_dup(info->svcname);
	if (name == NULL)
		return ENOMEM;

	*rname = name;
	return EOK;
}

errno_t fdisk_dev_info_capacity(fdisk_dev_info_t *info, capa_spec_t *capa)
{
	vbd_disk_info_t vinfo;
	errno_t rc;

	rc = vbd_disk_info(info->devlist->fdisk->vbd, info->svcid, &vinfo);
	if (rc != EOK)
		return EIO;

	capa_from_blocks(vinfo.nblocks, vinfo.block_size, capa);
	return EOK;
}

/** Add partition to our inventory. */
static errno_t fdisk_part_add(fdisk_dev_t *dev, vbd_part_id_t partid,
    fdisk_part_t **rpart)
{
	fdisk_part_t *part;
	vbd_part_info_t pinfo;
	vol_part_info_t vpinfo;
	errno_t rc;

	part = calloc(1, sizeof(fdisk_part_t));
	if (part == NULL)
		return ENOMEM;

	rc = vbd_part_get_info(dev->fdisk->vbd, partid, &pinfo);
	if (rc != EOK) {
		rc = EIO;
		goto error;
	}

	if (pinfo.svc_id != 0) {
		/*
		 * Normally vol service discovers the partition asynchronously.
		 * Here we need to make sure the partition is already known to it.
		 */
		rc = vol_part_add(dev->fdisk->vol, pinfo.svc_id);
		if (rc != EOK && rc != EEXIST) {
			rc = EIO;
			goto error;
		}

		rc = vol_part_info(dev->fdisk->vol, pinfo.svc_id, &vpinfo);
		if (rc != EOK) {
			rc = EIO;
			goto error;
		}

		part->pcnt = vpinfo.pcnt;
		part->fstype = vpinfo.fstype;
		part->label = str_dup(vpinfo.label);
	}

	part->dev = dev;
	part->index = pinfo.index;
	part->block0 = pinfo.block0;
	part->nblocks = pinfo.nblocks;
	part->pkind = pinfo.pkind;
	part->svc_id = pinfo.svc_id;

	switch (part->pkind) {
	case lpk_primary:
	case lpk_extended:
		fdisk_pri_part_insert_lists(dev, part);
		break;
	case lpk_logical:
		fdisk_log_part_insert_lists(dev, part);
		break;
	}

	list_append(&part->lparts, &dev->parts);

	if (part->pkind == lpk_extended)
		dev->ext_part = part;

	capa_from_blocks(part->nblocks, dev->dinfo.block_size,
	    &part->capacity);
	part->part_id = partid;

	if (rpart != NULL)
		*rpart = part;
	return EOK;
error:
	if (part != NULL)
		free(part->label);
	free(part);
	return rc;
}

/** Remove partition from our inventory. */
static void fdisk_part_remove(fdisk_part_t *part)
{
	list_remove(&part->lparts);
	if (link_used(&part->lpri_ba))
		list_remove(&part->lpri_ba);
	if (link_used(&part->lpri_idx))
		list_remove(&part->lpri_idx);
	if (link_used(&part->llog_ba))
		list_remove(&part->llog_ba);

	free(part->label);
	free(part);
}

static void fdisk_pri_part_insert_lists(fdisk_dev_t *dev, fdisk_part_t *part)
{
	link_t *link;
	fdisk_part_t *p;

	/* Insert to list by block address */
	link = list_first(&dev->pri_ba);
	while (link != NULL) {
		p = list_get_instance(link, fdisk_part_t, lpri_ba);
		if (p->block0 > part->block0) {
			list_insert_before(&part->lpri_ba, &p->lpri_ba);
			break;
		}

		link = list_next(link, &dev->pri_ba);
	}

	if (link == NULL)
		list_append(&part->lpri_ba, &dev->pri_ba);

	/* Insert to list by index */
	link = list_first(&dev->pri_idx);
	while (link != NULL) {
		p = list_get_instance(link, fdisk_part_t, lpri_idx);
		if (p->index > part->index) {
			list_insert_before(&part->lpri_idx, &p->lpri_idx);
			break;
		}

		link = list_next(link, &dev->pri_idx);
	}

	if (link == NULL)
		list_append(&part->lpri_idx, &dev->pri_idx);
}

static void fdisk_log_part_insert_lists(fdisk_dev_t *dev, fdisk_part_t *part)
{
	link_t *link;
	fdisk_part_t *p;

	/* Insert to list by block address */
	link = list_first(&dev->log_ba);
	while (link != NULL) {
		p = list_get_instance(link, fdisk_part_t, llog_ba);
		if (p->block0 > part->block0) {
			list_insert_before(&part->llog_ba, &p->llog_ba);
			break;
		}

		link = list_next(link, &dev->log_ba);
	}

	if (link == NULL)
		list_append(&part->llog_ba, &dev->log_ba);
}

static errno_t fdisk_dev_add_parts(fdisk_dev_t *dev)
{
	service_id_t *psids = NULL;
	size_t nparts, i;
	errno_t rc;

	rc = fdisk_update_dev_info(dev);
	if (rc != EOK) {
		rc = EIO;
		goto error;
	}

	rc = vbd_label_get_parts(dev->fdisk->vbd, dev->sid, &psids, &nparts);
	if (rc != EOK) {
		rc = EIO;
		goto error;
	}

	for (i = 0; i < nparts; i++) {
		rc = fdisk_part_add(dev, psids[i], NULL);
		if (rc != EOK) {
			goto error;
		}
	}

	free(psids);
	return EOK;
error:
	fdisk_dev_remove_parts(dev);
	return rc;
}

static void fdisk_dev_remove_parts(fdisk_dev_t *dev)
{
	fdisk_part_t *part;

	part = fdisk_part_first(dev);
	while (part != NULL) {
		fdisk_part_remove(part);
		part = fdisk_part_first(dev);
	}
}

errno_t fdisk_dev_open(fdisk_t *fdisk, service_id_t sid, fdisk_dev_t **rdev)
{
	fdisk_dev_t *dev = NULL;
	service_id_t *psids = NULL;
	size_t nparts, i;
	errno_t rc;

	dev = calloc(1, sizeof(fdisk_dev_t));
	if (dev == NULL)
		return ENOMEM;

	dev->fdisk = fdisk;
	dev->sid = sid;
	list_initialize(&dev->parts);
	list_initialize(&dev->pri_idx);
	list_initialize(&dev->pri_ba);
	list_initialize(&dev->log_ba);

	rc = fdisk_update_dev_info(dev);
	if (rc != EOK) {
		rc = EIO;
		goto error;
	}

	rc = vbd_label_get_parts(fdisk->vbd, sid, &psids, &nparts);
	if (rc != EOK) {
		rc = EIO;
		goto error;
	}

	for (i = 0; i < nparts; i++) {
		rc = fdisk_part_add(dev, psids[i], NULL);
		if (rc != EOK) {
			goto error;
		}
	}

	free(psids);
	*rdev = dev;
	return EOK;
error:
	fdisk_dev_close(dev);
	return rc;
}

void fdisk_dev_close(fdisk_dev_t *dev)
{
	if (dev == NULL)
		return;

	fdisk_dev_remove_parts(dev);
	free(dev);
}

/** Erase contents of unlabeled disk. */
errno_t fdisk_dev_erase(fdisk_dev_t *dev)
{
	fdisk_part_t *part;
	errno_t rc;

	if (dev->dinfo.ltype != lt_none)
		return EINVAL;

	part = fdisk_part_first(dev);
	assert(part != NULL);
	if (part->pcnt == vpc_empty)
		return EINVAL;

	rc = vol_part_empty(dev->fdisk->vol, part->svc_id);
	if (rc != EOK) {
		return rc;
	}

	part->pcnt = vpc_empty;
	return EOK;
}

void fdisk_dev_get_flags(fdisk_dev_t *dev, fdisk_dev_flags_t *rflags)
{
	fdisk_dev_flags_t flags;
	fdisk_part_t *part;

	flags = 0;

	/* fdf_can_create_label */
	if (dev->dinfo.ltype == lt_none) {
		part = fdisk_part_first(dev);
		assert(part != NULL);
		if (part->pcnt == vpc_empty)
			flags |= fdf_can_create_label;
		else
			flags |= fdf_can_erase_dev;
	} else {
		flags |= fdf_can_delete_label;
	}

	*rflags = flags;
}

errno_t fdisk_dev_get_svcname(fdisk_dev_t *dev, char **rname)
{
	char *name;
	errno_t rc;

	rc = loc_service_get_name(dev->sid, &name);
	if (rc != EOK)
		return rc;

	*rname = name;
	return EOK;
}

errno_t fdisk_dev_capacity(fdisk_dev_t *dev, capa_spec_t *capa)
{
	capa_from_blocks(dev->dinfo.nblocks, dev->dinfo.block_size, capa);
	return EOK;
}

errno_t fdisk_label_get_info(fdisk_dev_t *dev, fdisk_label_info_t *info)
{
	vbd_disk_info_t vinfo;
	uint64_t b0, nb;
	uint64_t hdrb;
	errno_t rc;

	rc = vbd_disk_info(dev->fdisk->vbd, dev->sid, &vinfo);
	if (rc != EOK) {
		rc = EIO;
		goto error;
	}

	info->ltype = vinfo.ltype;
	info->flags = vinfo.flags;

	if ((info->flags & lf_can_create_pri) != 0 ||
	    (info->flags & lf_can_create_ext) != 0) {
		/* Verify there is enough space to create partition */

		rc = fdisk_part_get_max_free_range(dev, spc_pri, &b0, &nb);
		if (rc != EOK)
			info->flags &= ~(lf_can_create_pri | lf_can_create_ext);
	}

	if ((info->flags & lf_can_create_log) != 0) {
		/* Verify there is enough space to create logical partition */
		hdrb = max(1, dev->align);
		rc = fdisk_part_get_max_free_range(dev, spc_log, &b0, &nb);
		if (rc != EOK || nb <= hdrb)
			info->flags &= ~lf_can_create_log;
	}

	return EOK;
error:
	return rc;
}

errno_t fdisk_label_create(fdisk_dev_t *dev, label_type_t ltype)
{
	fdisk_part_t *part;
	errno_t rc;

	/* Disk must not contain a label. */
	if (dev->dinfo.ltype != lt_none)
		return EEXIST;

	/* Dummy partition spanning entire disk must be considered empty */
	part = fdisk_part_first(dev);
	assert(part != NULL);
	if (part->pcnt != vpc_empty)
		return EEXIST;

	/* Remove dummy partition */
	fdisk_dev_remove_parts(dev);

	rc = vbd_label_create(dev->fdisk->vbd, dev->sid, ltype);
	if (rc != EOK) {
		/* Re-add dummy partition */
		(void) fdisk_dev_add_parts(dev);
		return rc;
	}

	rc = fdisk_update_dev_info(dev);
	if (rc != EOK)
		return rc;

	return EOK;
}

errno_t fdisk_label_destroy(fdisk_dev_t *dev)
{
	fdisk_part_t *part;
	fdisk_dev_flags_t dflags;
	errno_t rc;

	part = fdisk_part_first(dev);
	while (part != NULL) {
		rc = fdisk_part_destroy(part);
		if (rc != EOK)
			return EIO;
		part = fdisk_part_first(dev);
	}

	rc = vbd_label_delete(dev->fdisk->vbd, dev->sid);
	if (rc != EOK)
		return EIO;

	rc = fdisk_dev_add_parts(dev);
	if (rc != EOK)
		return rc;

	/* Make sure device is considered empty */
	fdisk_dev_get_flags(dev, &dflags);
	if ((dflags & fdf_can_erase_dev) != 0) {
		rc = fdisk_dev_erase(dev);
		if (rc != EOK)
			return rc;
	}

	return EOK;
}

fdisk_part_t *fdisk_part_first(fdisk_dev_t *dev)
{
	link_t *link;

	link = list_first(&dev->parts);
	if (link == NULL)
		return NULL;

	return list_get_instance(link, fdisk_part_t, lparts);
}

fdisk_part_t *fdisk_part_next(fdisk_part_t *part)
{
	link_t *link;

	link = list_next(&part->lparts, &part->dev->parts);
	if (link == NULL)
		return NULL;

	return list_get_instance(link, fdisk_part_t, lparts);
}

errno_t fdisk_part_get_info(fdisk_part_t *part, fdisk_part_info_t *info)
{
	info->capacity = part->capacity;
	info->pcnt = part->pcnt;
	info->fstype = part->fstype;
	info->pkind = part->pkind;
	info->label = part->label;
	info->svc_id = part->svc_id;
	return EOK;
}

/** Get size of largest free block. */
errno_t fdisk_part_get_max_avail(fdisk_dev_t *dev, fdisk_spc_t spc,
    capa_spec_t *capa)
{
	errno_t rc;
	uint64_t b0;
	uint64_t nb;
	aoff64_t hdrb;

	rc = fdisk_part_get_max_free_range(dev, spc, &b0, &nb);
	if (rc != EOK)
		return rc;

	/* For logical partitions we need to subtract header size */
	if (spc == spc_log) {
		hdrb = max(1, dev->align);
		if (nb <= hdrb)
			return ENOSPC;
		nb -= hdrb;
	}

	capa_from_blocks(nb, dev->dinfo.block_size, capa);
	return EOK;
}

/** Get total free space capacity. */
errno_t fdisk_part_get_tot_avail(fdisk_dev_t *dev, fdisk_spc_t spc,
    capa_spec_t *capa)
{
	fdisk_free_range_t fr;
	uint64_t hdrb;
	uint64_t b0;
	uint64_t nb;
	uint64_t totb;

	if (spc == spc_log)
		hdrb = max(1, dev->align);
	else
		hdrb = 0;

	totb = 0;
	fdisk_free_range_first(dev, spc, &fr);
	do {
		if (fdisk_free_range_get(&fr, &b0, &nb)) {
			if (nb > hdrb)
				totb += nb - hdrb;
		}
	} while (fdisk_free_range_next(&fr));

	capa_from_blocks(totb, dev->dinfo.block_size, capa);
	return EOK;
}

/** Create partition.
 *
 * Create new partition based on a specification.
 *
 * @param dev Fdisk device
 * @param pspec Partition specification
 * @param rpart Place to store pointer to new partition
 *
 * @return EOK on success or error code
 */
errno_t fdisk_part_create(fdisk_dev_t *dev, fdisk_part_spec_t *pspec,
    fdisk_part_t **rpart)
{
	fdisk_part_t *part = NULL;
	vbd_part_spec_t vpspec;
	vbd_part_id_t partid = 0;
	vol_part_info_t vpinfo;
	const char *label;
	const char *mountp;
	errno_t rc;

	label = pspec->label != NULL ? pspec->label : "";
	mountp = pspec->mountp != NULL ? pspec->mountp : "";

	rc = fdisk_part_spec_prepare(dev, pspec, &vpspec);
	if (rc != EOK) {
		rc = EIO;
		goto error;
	}

	rc = vbd_part_create(dev->fdisk->vbd, dev->sid, &vpspec, &partid);
	if (rc != EOK) {
		rc = EIO;
		goto error;
	}

	rc = fdisk_part_add(dev, partid, &part);
	if (rc != EOK) {
		rc = EIO;
		goto error;
	}

	if (part->svc_id != 0) {
		rc = vol_part_mkfs(dev->fdisk->vol, part->svc_id, pspec->fstype,
		    label, mountp);
		if (rc != EOK && rc != ENOTSUP) {
			rc = EIO;
			goto error;
		}

		/* Get the real label value */
		rc = vol_part_info(dev->fdisk->vol, part->svc_id, &vpinfo);
		if (rc != EOK) {
			rc = EIO;
			goto error;
		}

		part->pcnt = vpinfo.pcnt;
		part->fstype = vpinfo.fstype;
		part->label = str_dup(vpinfo.label);

		if (part->label == NULL) {
			rc = EIO;
			goto error;
		}
	}

	if (rpart != NULL)
		*rpart = part;
	return EOK;
error:
	/* Try rolling back */
	if (part != NULL)
		fdisk_part_remove(part);
	if (partid != 0)
		(void) vbd_part_delete(dev->fdisk->vbd, partid);
	return rc;
}

/** Destroy partition.
 *
 * @param part Partition
 * @return EOK on success or error code
 */
errno_t fdisk_part_destroy(fdisk_part_t *part)
{
	errno_t rc;

	rc = vol_part_eject(part->dev->fdisk->vol, part->svc_id);
	if (rc != EOK)
		return EIO;

	rc = vbd_part_delete(part->dev->fdisk->vbd, part->part_id);
	if (rc != EOK)
		return EIO;

	fdisk_part_remove(part);
	return EOK;
}

/** Set partition mount point.
 *
 * @param part Fdisk partition
 * @param mountp Mount point
 *
 * @return EOK on success or error code
 */
errno_t fdisk_part_set_mountp(fdisk_part_t *part, const char *mountp)
{
	return vol_part_set_mountp(part->dev->fdisk->vol,
	    part->svc_id, mountp);
}

void fdisk_pspec_init(fdisk_part_spec_t *pspec)
{
	memset(pspec, 0, sizeof(fdisk_part_spec_t));
}

/** Get free partition index. */
static errno_t fdisk_part_get_free_idx(fdisk_dev_t *dev, int *rindex)
{
	link_t *link;
	fdisk_part_t *part;
	int nidx;

	link = list_first(&dev->pri_idx);
	nidx = 1;
	while (link != NULL) {
		part = list_get_instance(link, fdisk_part_t, lpri_idx);
		if (part->index > nidx)
			break;
		nidx = part->index + 1;
		link = list_next(link, &dev->pri_idx);
	}

	if (nidx > 4 /* XXXX actual number of slots */) {
		return ELIMIT;
	}

	*rindex = nidx;
	return EOK;
}

/** Get free range of blocks.
 *
 * Get free range of blocks of at least the specified size (first fit).
 */
static errno_t fdisk_part_get_free_range(fdisk_dev_t *dev, aoff64_t nblocks,
    fdisk_spc_t spc, aoff64_t *rblock0, aoff64_t *rnblocks)
{
	fdisk_free_range_t fr;
	uint64_t b0;
	uint64_t nb;

	fdisk_free_range_first(dev, spc, &fr);
	do {
		if (fdisk_free_range_get(&fr, &b0, &nb)) {
			if (nb >= nblocks) {
				*rblock0 = b0;
				*rnblocks = nb;
				return EOK;
			}
		}
	} while (fdisk_free_range_next(&fr));

	/* No conforming free range found */
	return ENOSPC;
}

/** Get largest free range of blocks.
 *
 * Get free range of blocks of the maximum size.
 */
static errno_t fdisk_part_get_max_free_range(fdisk_dev_t *dev, fdisk_spc_t spc,
    aoff64_t *rblock0, aoff64_t *rnblocks)
{
	fdisk_free_range_t fr;
	uint64_t b0;
	uint64_t nb;
	uint64_t best_b0;
	uint64_t best_nb;

	best_b0 = best_nb = 0;
	fdisk_free_range_first(dev, spc, &fr);
	do {
		if (fdisk_free_range_get(&fr, &b0, &nb)) {
			if (nb > best_nb) {
				best_b0 = b0;
				best_nb = nb;
			}
		}
	} while (fdisk_free_range_next(&fr));

	if (best_nb == 0)
		return ENOSPC;

	*rblock0 = best_b0;
	*rnblocks = best_nb;
	return EOK;
}

/** Prepare new partition specification for VBD. */
static errno_t fdisk_part_spec_prepare(fdisk_dev_t *dev, fdisk_part_spec_t *pspec,
    vbd_part_spec_t *vpspec)
{
	aoff64_t nom_blocks;
	aoff64_t min_blocks;
	aoff64_t max_blocks;
	aoff64_t act_blocks;
	aoff64_t fblock0;
	aoff64_t fnblocks;
	aoff64_t hdrb;
	label_pcnt_t pcnt;
	fdisk_spc_t spc;
	int index;
	errno_t rc;

	rc = capa_to_blocks(&pspec->capacity, cv_nom, dev->dinfo.block_size,
	    &nom_blocks);
	if (rc != EOK)
		return rc;

	rc = capa_to_blocks(&pspec->capacity, cv_min, dev->dinfo.block_size,
	    &min_blocks);
	if (rc != EOK)
		return rc;

	rc = capa_to_blocks(&pspec->capacity, cv_max, dev->dinfo.block_size,
	    &max_blocks);
	if (rc != EOK)
		return rc;

	nom_blocks = fdisk_ba_align_up(dev, nom_blocks);
	min_blocks = fdisk_ba_align_up(dev, min_blocks);
	max_blocks = fdisk_ba_align_up(dev, max_blocks);

	pcnt = LPC_LIMIT;

	switch (pspec->fstype) {
	case fs_exfat:
		pcnt = lpc_exfat;
		break;
	case fs_fat:
		pcnt = lpc_fat32; /* XXX Detect FAT12/16 vs FAT32 */
		break;
	case fs_minix:
		pcnt = lpc_minix;
		break;
	case fs_ext4:
		pcnt = lpc_ext4;
		break;
	case fs_cdfs:
		return EINVAL; /* You cannot create an ISO partition */
	}

	if (pcnt == LPC_LIMIT)
		return EINVAL;

	if (pspec->pkind == lpk_logical) {
		hdrb = max(1, dev->align);
		spc = spc_log;
	} else {
		hdrb = 0;
		spc = spc_pri;
	}

	rc = fdisk_part_get_free_range(dev, hdrb + nom_blocks, spc,
	    &fblock0, &fnblocks);

	if (rc == EOK) {
		/*
		 * If the size of the free range would still give the same capacity
		 * when rounded, allocate entire range. Otherwise allocate exactly
		 * what we were asked for.
		 */
		if (fnblocks <= max_blocks) {
			act_blocks = fnblocks;
		} else {
			act_blocks = hdrb + nom_blocks;
		}
	} else {
		assert(rc == ENOSPC);

		/*
		 * There is no free range that can contain exactly the requested
		 * capacity. Try to allocate at least such number of blocks
		 * that would still fullfill the request within the limits
		 * of the precision with witch the capacity was specified
		 * (i.e. when rounded up).
		 */
		rc = fdisk_part_get_free_range(dev, hdrb + min_blocks, spc,
		    &fblock0, &fnblocks);
		if (rc != EOK)
			return rc;

		assert(fnblocks < hdrb + nom_blocks);
		act_blocks = fnblocks;
	}

	if (pspec->pkind != lpk_logical) {
		rc = fdisk_part_get_free_idx(dev, &index);
		if (rc != EOK)
			return EIO;
	} else {
		index = 0;
	}

	memset(vpspec, 0, sizeof(vbd_part_spec_t));
	vpspec->index = index;
	vpspec->hdr_blocks = hdrb;
	vpspec->block0 = fblock0 + hdrb;
	vpspec->nblocks = act_blocks - hdrb;
	vpspec->pkind = pspec->pkind;

	if (pspec->pkind != lpk_extended) {
		rc = vbd_suggest_ptype(dev->fdisk->vbd, dev->sid, pcnt,
		    &vpspec->ptype);
		if (rc != EOK)
			return EIO;
	}

	return EOK;
}

static errno_t fdisk_update_dev_info(fdisk_dev_t *dev)
{
	errno_t rc;
	size_t align_bytes;
	uint64_t avail_cap;

	rc = vbd_disk_info(dev->fdisk->vbd, dev->sid, &dev->dinfo);
	if (rc != EOK)
		return EIO;

	/* Capacity available for partition in bytes */
	avail_cap = dev->dinfo.anblocks * dev->dinfo.block_size;

	/* Determine optimum alignment */
	align_bytes = 1024 * 1024; /* 1 MiB */
	while (align_bytes > 1 && avail_cap / align_bytes < 256) {
		align_bytes = align_bytes / 16;
	}

	dev->align = align_bytes / dev->dinfo.block_size;
	if (dev->align < 1)
		dev->align = 1;
	return EOK;
}

static uint64_t fdisk_ba_align_up(fdisk_dev_t *dev, uint64_t ba)
{
	return ((ba + dev->align - 1) / dev->align) * dev->align;
}

static uint64_t fdisk_ba_align_down(fdisk_dev_t *dev, uint64_t ba)
{
	return ba - (ba % dev->align);
}

static void fdisk_free_range_first(fdisk_dev_t *dev, fdisk_spc_t spc,
    fdisk_free_range_t *fr)
{
	link_t *link;

	fr->dev = dev;
	fr->spc = spc;

	if (fr->spc == spc_pri) {
		/* Primary partitions */
		fr->b0 = fdisk_ba_align_up(fr->dev, fr->dev->dinfo.ablock0);

		link = list_first(&dev->pri_ba);
		if (link != NULL)
			fr->npart = list_get_instance(link, fdisk_part_t, lpri_ba);
		else
			fr->npart = NULL;
	} else { /* fr->spc == spc_log */
		/* Logical partitions */
		fr->b0 = fdisk_ba_align_up(fr->dev, fr->dev->ext_part->block0);

		link = list_first(&dev->log_ba);
		if (link != NULL)
			fr->npart = list_get_instance(link, fdisk_part_t, llog_ba);
		else
			fr->npart = NULL;
	}
}

static bool fdisk_free_range_next(fdisk_free_range_t *fr)
{
	link_t *link;

	if (fr->npart == NULL)
		return false;

	fr->b0 = fdisk_ba_align_up(fr->dev, fr->npart->block0 +
	    fr->npart->nblocks);

	if (fr->spc == spc_pri) {
		/* Primary partitions */
		link = list_next(&fr->npart->lpri_ba, &fr->dev->pri_ba);
		if (link != NULL)
			fr->npart = list_get_instance(link, fdisk_part_t, lpri_ba);
		else
			fr->npart = NULL;
	} else { /* fr->spc == spc_log */
		/* Logical partitions */
		link = list_next(&fr->npart->llog_ba, &fr->dev->log_ba);
		if (link != NULL)
			fr->npart = list_get_instance(link, fdisk_part_t, llog_ba);
		else
			fr->npart = NULL;
	}

	return true;
}

static bool fdisk_free_range_get(fdisk_free_range_t *fr,
    aoff64_t *b0, aoff64_t *nb)
{
	aoff64_t b1;

	if (fr->npart != NULL) {
		b1 = fdisk_ba_align_down(fr->dev, fr->npart->block0);
	} else {
		if (fr->spc == spc_pri) {
			b1 = fdisk_ba_align_down(fr->dev,
			    fr->dev->dinfo.ablock0 + fr->dev->dinfo.anblocks);
		} else { /* fr->spc == spc_log */
			b1 = fdisk_ba_align_down(fr->dev,
			    fr->dev->ext_part->block0 + fr->dev->ext_part->nblocks);
		}
	}

	if (b1 < fr->b0)
		return false;

	*b0 = fr->b0;
	*nb = b1 - fr->b0;

	return true;
}

/** Get volume label support. */
errno_t fdisk_get_vollabel_support(fdisk_dev_t *dev, vol_fstype_t fstype,
    vol_label_supp_t *vlsupp)
{
	return vol_part_get_lsupp(dev->fdisk->vol, fstype, vlsupp);
}

/** @}
 */
