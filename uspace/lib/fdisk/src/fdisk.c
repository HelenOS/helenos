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

/** @addtogroup libfdisk
 * @{
 */
/**
 * @file Disk management library.
 */

#include <adt/list.h>
#include <block.h>
#include <errno.h>
#include <fdisk.h>
#include <loc.h>
#include <mem.h>
#include <stdio.h>
#include <stdlib.h>
#include <str.h>
#include <vbd.h>
#include <vol.h>

static const char *cu_str[] = {
	[cu_byte] = "B",
	[cu_kbyte] = "kB",
	[cu_mbyte] = "MB",
	[cu_gbyte] = "GB",
	[cu_tbyte] = "TB",
	[cu_pbyte] = "PB",
	[cu_ebyte] = "EB",
	[cu_zbyte] = "ZB",
	[cu_ybyte] = "YB"
};

static int fdisk_dev_add_parts(fdisk_dev_t *);
static void fdisk_dev_remove_parts(fdisk_dev_t *);
static int fdisk_part_spec_prepare(fdisk_dev_t *, fdisk_part_spec_t *,
    vbd_part_spec_t *);
static void fdisk_pri_part_insert_lists(fdisk_dev_t *, fdisk_part_t *);
static void fdisk_log_part_insert_lists(fdisk_dev_t *, fdisk_part_t *);
static int fdisk_update_dev_info(fdisk_dev_t *);
static uint64_t fdisk_ba_align_up(fdisk_dev_t *, uint64_t);
static uint64_t fdisk_ba_align_down(fdisk_dev_t *, uint64_t);

static void fdisk_dev_info_delete(fdisk_dev_info_t *info)
{
	if (info == NULL)
		return;

	if (info->blk_inited)
		block_fini(info->svcid);

	free(info->svcname);
	free(info);
}

int fdisk_create(fdisk_t **rfdisk)
{
	fdisk_t *fdisk = NULL;
	int rc;

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

int fdisk_dev_list_get(fdisk_t *fdisk, fdisk_dev_list_t **rdevlist)
{
	fdisk_dev_list_t *devlist = NULL;
	fdisk_dev_info_t *info;
	service_id_t *svcs = NULL;
	size_t count, i;
	int rc;

	devlist = calloc(1, sizeof(fdisk_dev_list_t));
	if (devlist == NULL)
		return ENOMEM;

	list_initialize(&devlist->devinfos);

	printf("vbd_get_disks()\n");
	rc = vbd_get_disks(fdisk->vbd, &svcs, &count);
	printf(" -> %d\n", rc);
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

int fdisk_dev_info_get_svcname(fdisk_dev_info_t *info, char **rname)
{
	char *name;
	int rc;

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

int fdisk_dev_info_capacity(fdisk_dev_info_t *info, fdisk_cap_t *cap)
{
	size_t bsize;
	aoff64_t nblocks;
	int rc;

	if (!info->blk_inited) {
		rc = block_init(EXCHANGE_SERIALIZE, info->svcid, 2048);
		if (rc != EOK)
			return rc;

		info->blk_inited = true;
	}

	rc = block_get_bsize(info->svcid, &bsize);
	if (rc != EOK)
		return EIO;

	rc = block_get_nblocks(info->svcid, &nblocks);
	if (rc != EOK)
		return EIO;

	cap->value = bsize * nblocks;
	cap->cunit = cu_byte;

	return EOK;
}

/** Add partition to our inventory. */
static int fdisk_part_add(fdisk_dev_t *dev, vbd_part_id_t partid,
    fdisk_part_t **rpart)
{
	fdisk_part_t *part;
	vbd_part_info_t pinfo;
	vol_part_info_t vpinfo;
	int rc;

	part = calloc(1, sizeof(fdisk_part_t));
	if (part == NULL)
		return ENOMEM;

	printf("vbd_part_get_info(%zu)\n", partid);
	rc = vbd_part_get_info(dev->fdisk->vbd, partid, &pinfo);
	if (rc != EOK) {
		rc = EIO;
		goto error;
	}

	printf("vol_part_add(%zu)...\n", pinfo.svc_id);
	/*
	 * Normally vol service discovers the partition asynchronously.
	 * Here we need to make sure the partition is already known to it.
	 */
	rc = vol_part_add(dev->fdisk->vol, pinfo.svc_id);
	printf("vol_part_add->rc = %d\n", rc);
	if (rc != EOK && rc != EEXIST) {
		rc = EIO;
		goto error;
	}

	printf("vol_part_info(%zu)\n", pinfo.svc_id);
	rc = vol_part_info(dev->fdisk->vol, pinfo.svc_id, &vpinfo);
	printf("vol_part_info->rc = %d\n", rc);
	if (rc != EOK) {
		rc = EIO;
		goto error;
	}

	part->dev = dev;
	part->index = pinfo.index;
	part->block0 = pinfo.block0;
	part->nblocks = pinfo.nblocks;
	part->pkind = pinfo.pkind;
	part->svc_id = pinfo.svc_id;
	part->pcnt = vpinfo.pcnt;
	part->fstype = vpinfo.fstype;

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

	part->capacity.cunit = cu_byte;
	part->capacity.value = part->nblocks * dev->dinfo.block_size;
	part->part_id = partid;

	if (rpart != NULL)
		*rpart = part;
	return EOK;
error:
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

static int fdisk_dev_add_parts(fdisk_dev_t *dev)
{
	service_id_t *psids = NULL;
	size_t nparts, i;
	int rc;

	printf("get label info\n");
	rc = fdisk_update_dev_info(dev);
	if (rc != EOK) {
		printf("failed\n");
		rc = EIO;
		goto error;
	}

	printf("block size: %zu\n", dev->dinfo.block_size);
	printf("get partitions\n");
	rc = vbd_label_get_parts(dev->fdisk->vbd, dev->sid, &psids, &nparts);
	if (rc != EOK) {
		printf("failed\n");
		rc = EIO;
		goto error;
	}
	printf("OK\n");

	printf("found %zu partitions.\n", nparts);
	for (i = 0; i < nparts; i++) {
		printf("add partition sid=%zu\n", psids[i]);
		rc = fdisk_part_add(dev, psids[i], NULL);
		if (rc != EOK) {
			printf("failed\n");
			goto error;
		}
		printf("OK\n");
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

int fdisk_dev_open(fdisk_t *fdisk, service_id_t sid, fdisk_dev_t **rdev)
{
	vbd_disk_info_t vinfo;
	fdisk_dev_t *dev = NULL;
	service_id_t *psids = NULL;
	size_t nparts, i;
	int rc;

	dev = calloc(1, sizeof(fdisk_dev_t));
	if (dev == NULL)
		return ENOMEM;

	dev->fdisk = fdisk;
	dev->sid = sid;
	list_initialize(&dev->parts);
	list_initialize(&dev->pri_idx);
	list_initialize(&dev->pri_ba);
	list_initialize(&dev->log_ba);

	rc = vbd_disk_info(fdisk->vbd, sid, &vinfo);
	if (rc != EOK) {
		rc = EIO;
		goto error;
	}

	printf("get label info\n");
	rc = fdisk_update_dev_info(dev);
	if (rc != EOK) {
		printf("failed\n");
		rc = EIO;
		goto error;
	}

	printf("block size: %zu\n", dev->dinfo.block_size);
	printf("get partitions\n");
	rc = vbd_label_get_parts(fdisk->vbd, sid, &psids, &nparts);
	if (rc != EOK) {
		printf("failed\n");
		rc = EIO;
		goto error;
	}
	printf("OK\n");

	printf("found %zu partitions.\n", nparts);
	for (i = 0; i < nparts; i++) {
		printf("add partition sid=%zu\n", psids[i]);
		rc = fdisk_part_add(dev, psids[i], NULL);
		if (rc != EOK) {
			printf("failed\n");
			goto error;
		}
		printf("OK\n");
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
int fdisk_dev_erase(fdisk_dev_t *dev)
{
	fdisk_part_t *part;
	int rc;

	printf("fdisk_dev_erase.. check ltype\n");
	if (dev->dinfo.ltype != lt_none)
		return EINVAL;

	printf("fdisk_dev_erase.. get first part\n");
	part = fdisk_part_first(dev);
	assert(part != NULL);
	printf("fdisk_dev_erase.. check part\n");
	if (part->pcnt == vpc_empty)
		return EINVAL;

	printf("fdisk_dev_erase.. check part\n");
	rc = vol_part_empty(dev->fdisk->vol, part->svc_id);
	if (rc != EOK) {
		printf("vol_part_empty -> %d\n", rc);
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

int fdisk_dev_get_svcname(fdisk_dev_t *dev, char **rname)
{
	char *name;
	int rc;

	rc = loc_service_get_name(dev->sid, &name);
	if (rc != EOK)
		return rc;

	*rname = name;
	return EOK;
}

int fdisk_dev_capacity(fdisk_dev_t *dev, fdisk_cap_t *cap)
{
	size_t bsize;
	aoff64_t nblocks;
	int rc;

	rc = block_init(EXCHANGE_SERIALIZE, dev->sid, 2048);
	if (rc != EOK)
		return rc;

	rc = block_get_bsize(dev->sid, &bsize);
	if (rc != EOK)
		return EIO;

	rc = block_get_nblocks(dev->sid, &nblocks);
	if (rc != EOK)
		return EIO;

	block_fini(dev->sid);

	cap->value = bsize * nblocks;
	cap->cunit = cu_byte;

	return EOK;
}

int fdisk_label_get_info(fdisk_dev_t *dev, fdisk_label_info_t *info)
{
	vbd_disk_info_t vinfo;
	int rc;

	rc = vbd_disk_info(dev->fdisk->vbd, dev->sid, &vinfo);
	if (rc != EOK) {
		rc = EIO;
		goto error;
	}

	info->ltype = vinfo.ltype;
	info->flags = vinfo.flags;
	return EOK;
error:
	return rc;
}

int fdisk_label_create(fdisk_dev_t *dev, label_type_t ltype)
{
	fdisk_part_t *part;
	int rc;

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
	if (rc != EOK)
		return rc;

	rc = fdisk_update_dev_info(dev);
	if (rc != EOK)
		return rc;

	return EOK;
}

int fdisk_label_destroy(fdisk_dev_t *dev)
{
	fdisk_part_t *part;
	fdisk_dev_flags_t dflags;
	int rc;

	printf("fdisk_label_destroy: begin\n");

	part = fdisk_part_first(dev);
	while (part != NULL) {
		rc = fdisk_part_destroy(part);
		if (rc != EOK)
			return EIO;
		part = fdisk_part_first(dev);
	}

	printf("fdisk_label_destroy: vbd_label_delete\n");

	rc = vbd_label_delete(dev->fdisk->vbd, dev->sid);
	if (rc != EOK)
		return EIO;

	printf("fdisk_label_destroy: add parts\n");
	rc = fdisk_dev_add_parts(dev);
	if (rc != EOK)
		return rc;

	printf("fdisk_label_destroy: erase dev\n");
	/* Make sure device is considered empty */
	fdisk_dev_get_flags(dev, &dflags);
	if ((dflags & fdf_can_erase_dev) != 0) {
		rc = fdisk_dev_erase(dev);
		if (rc != EOK)
			return rc;
	}

	printf("fdisk_label_destroy: done\n");
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

int fdisk_part_get_info(fdisk_part_t *part, fdisk_part_info_t *info)
{
	info->capacity = part->capacity;
	info->pcnt = part->pcnt;
	info->fstype = part->fstype;
	info->pkind = part->pkind;
	return EOK;
}

int fdisk_part_get_max_avail(fdisk_dev_t *dev, fdisk_cap_t *cap)
{
	return EOK;
}

int fdisk_part_create(fdisk_dev_t *dev, fdisk_part_spec_t *pspec,
    fdisk_part_t **rpart)
{
	fdisk_part_t *part;
	vbd_part_spec_t vpspec;
	vbd_part_id_t partid;
	int rc;

	printf("fdisk_part_create()\n");

	rc = fdisk_part_spec_prepare(dev, pspec, &vpspec);
	if (rc != EOK)
		return EIO;

	printf("fdisk_part_create() - call vbd_part_create\n");
	rc = vbd_part_create(dev->fdisk->vbd, dev->sid, &vpspec, &partid);
	if (rc != EOK)
		return EIO;

	printf("fdisk_part_create() - call fdisk_part_add\n");
	rc = fdisk_part_add(dev, partid, &part);
	if (rc != EOK) {
		/* Try rolling back */
		(void) vbd_part_delete(dev->fdisk->vbd, partid);
		return EIO;
	}

	rc = vol_part_mkfs(dev->fdisk->vol, part->svc_id, pspec->fstype);
	if (rc != EOK && rc != ENOTSUP) {
		printf("mkfs failed\n");
		fdisk_part_remove(part);
		(void) vbd_part_delete(dev->fdisk->vbd, partid);
		return EIO;
	}

	printf("fdisk_part_create() - done\n");
	part->pcnt = vpc_fs;
	part->fstype = pspec->fstype;
	part->capacity = pspec->capacity;

	if (rpart != NULL)
		*rpart = part;
	return EOK;
}

int fdisk_part_destroy(fdisk_part_t *part)
{
	int rc;

	rc = vbd_part_delete(part->dev->fdisk->vbd, part->part_id);
	if (rc != EOK)
		return EIO;

	fdisk_part_remove(part);
	return EOK;
}

void fdisk_pspec_init(fdisk_part_spec_t *pspec)
{
	memset(pspec, 0, sizeof(fdisk_part_spec_t));
}

int fdisk_cap_format(fdisk_cap_t *cap, char **rstr)
{
	int rc;
	const char *sunit;

	sunit = NULL;

	if (cap->cunit < 0 || cap->cunit >= CU_LIMIT)
		assert(false);

	sunit = cu_str[cap->cunit];
	rc = asprintf(rstr, "%" PRIu64 " %s", cap->value, sunit);
	if (rc < 0)
		return ENOMEM;

	return EOK;
}

int fdisk_cap_parse(const char *str, fdisk_cap_t *cap)
{
	char *eptr;
	char *p;
	unsigned long val;
	int i;

	val = strtoul(str, &eptr, 10);

	while (*eptr == ' ')
		++eptr;

	if (*eptr == '\0') {
		cap->cunit = cu_byte;
	} else {
		for (i = 0; i < CU_LIMIT; i++) {
			if (str_lcasecmp(eptr, cu_str[i],
			    str_length(cu_str[i])) == 0) {
				p = eptr + str_size(cu_str[i]);
				while (*p == ' ')
					++p;
				if (*p == '\0')
					goto found;
			}
		}

		return EINVAL;
found:
		cap->cunit = i;
	}

	cap->value = val;
	return EOK;
}

int fdisk_ltype_format(label_type_t ltype, char **rstr)
{
	const char *sltype;
	char *s;

	sltype = NULL;
	switch (ltype) {
	case lt_none:
		sltype = "None";
		break;
	case lt_mbr:
		sltype = "MBR";
		break;
	case lt_gpt:
		sltype = "GPT";
		break;
	}

	s = str_dup(sltype);
	if (s == NULL)
		return ENOMEM;

	*rstr = s;
	return EOK;
}

int fdisk_fstype_format(vol_fstype_t fstype, char **rstr)
{
	const char *sfstype;
	char *s;

	sfstype = NULL;
	switch (fstype) {
	case fs_exfat:
		sfstype = "ExFAT";
		break;
	case fs_fat:
		sfstype = "FAT";
		break;
	case fs_minix:
		sfstype = "MINIX";
		break;
	case fs_ext4:
		sfstype = "Ext4";
		break;
	}

	s = str_dup(sfstype);
	if (s == NULL)
		return ENOMEM;

	*rstr = s;
	return EOK;
}

int fdisk_pkind_format(label_pkind_t pkind, char **rstr)
{
	const char *spkind;
	char *s;

	spkind = NULL;
	switch (pkind) {
	case lpk_primary:
		spkind = "Primary";
		break;
	case lpk_extended:
		spkind = "Extended";
		break;
	case lpk_logical:
		spkind = "Logical";
		break;
	}

	s = str_dup(spkind);
	if (s == NULL)
		return ENOMEM;

	*rstr = s;
	return EOK;
}

/** Get free partition index. */
static int fdisk_part_get_free_idx(fdisk_dev_t *dev, int *rindex)
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

	if (nidx > 4 /* XXXX actual number of slots*/) {
		return ELIMIT;
	}

	*rindex = nidx;
	return EOK;
}

/** Get free range of blocks.
 *
 * Get free range of blocks of at least the specified size (first fit).
 */
static int fdisk_part_get_free_range(fdisk_dev_t *dev, aoff64_t nblocks,
    aoff64_t *rblock0, aoff64_t *rnblocks)
{
	link_t *link;
	fdisk_part_t *part;
	uint64_t avail;
	uint64_t pb0;
	uint64_t nba;

	printf("fdisk_part_get_free_range: align=%" PRIu64 "\n",
	    dev->align);

	link = list_first(&dev->pri_ba);
	nba = fdisk_ba_align_up(dev, dev->dinfo.ablock0);
	while (link != NULL) {
		part = list_get_instance(link, fdisk_part_t, lpri_ba);
		pb0 = fdisk_ba_align_down(dev, part->block0);
		if (pb0 >= nba && pb0 - nba >= nblocks)
			break;
		nba = fdisk_ba_align_up(dev, part->block0 + part->nblocks);
		link = list_next(link, &dev->pri_ba);
	}

	if (link != NULL) {
		printf("nba=%" PRIu64 " pb0=%" PRIu64 "\n",
		    nba, pb0);
		/* Free range before a partition */
		avail = pb0 - nba;
	} else {
		/* Free range at the end */
		pb0 = fdisk_ba_align_down(dev, dev->dinfo.ablock0 +
		    dev->dinfo.anblocks);
		if (pb0 < nba)
			return ELIMIT;
		avail = pb0 - nba;
		printf("nba=%" PRIu64 " avail=%" PRIu64 "\n",
		    nba, avail);

	}

	/* Verify that the range is large enough */
	if (avail < nblocks)
		return ELIMIT;

	*rblock0 = nba;
	*rnblocks = avail;
	return EOK;
}

/** Get free range of blocks in extended partition.
 *
 * Get free range of blocks in extended partition that can accomodate
 * a partition of at least the specified size plus the header (EBR + padding).
 * Returns the header size in blocks, the start and length of the partition.
 */
static int fdisk_part_get_log_free_range(fdisk_dev_t *dev, aoff64_t nblocks,
    aoff64_t *rhdrb, aoff64_t *rblock0, aoff64_t *rnblocks)
{
	link_t *link;
	fdisk_part_t *part;
	uint64_t avail;
	uint64_t hdrb;
	uint64_t pb0;
	uint64_t nba;

	printf("fdisk_part_get_log_free_range\n");
	/* Number of header blocks */
	hdrb = max(1, dev->align);

	link = list_first(&dev->log_ba);
	nba = fdisk_ba_align_up(dev, dev->ext_part->block0);
	while (link != NULL) {
		part = list_get_instance(link, fdisk_part_t, llog_ba);
		pb0 = fdisk_ba_align_down(dev, part->block0);
		if (pb0 >= nba && pb0 - nba >= nblocks)
			break;
		nba = fdisk_ba_align_up(dev, part->block0 + part->nblocks);
		link = list_next(link, &dev->log_ba);
	}

	if (link != NULL) {
		/* Free range before a partition */
		avail = pb0 - nba;
	} else {
		/* Free range at the end */
		pb0 = fdisk_ba_align_down(dev, dev->ext_part->block0 +
		    dev->ext_part->nblocks);
		if (pb0 < nba) {
			printf("not enough space\n");
			return ELIMIT;
		}

		avail = pb0 - nba;

		printf("nba=%" PRIu64 " pb0=%" PRIu64" avail=%" PRIu64 "\n",
		    nba, pb0, avail);
	}
	/* Verify that the range is large enough */
	if (avail < hdrb + nblocks) {
		printf("not enough space\n");
		return ELIMIT;
	}

	*rhdrb = hdrb;
	*rblock0 = nba + hdrb;
	*rnblocks = avail;
	printf("hdrb=%" PRIu64 " block0=%" PRIu64" avail=%" PRIu64 "\n",
	    hdrb, nba + hdrb, avail);
	return EOK;
}

/** Prepare new partition specification for VBD. */
static int fdisk_part_spec_prepare(fdisk_dev_t *dev, fdisk_part_spec_t *pspec,
    vbd_part_spec_t *vpspec)
{
	uint64_t cbytes;
	aoff64_t req_blocks;
	aoff64_t fhdr;
	aoff64_t fblock0;
	aoff64_t fnblocks;
	uint64_t block_size;
	label_pcnt_t pcnt;
	unsigned i;
	int index;
	int rc;

	printf("fdisk_part_spec_prepare() - dev=%p pspec=%p vpspec=%p\n", dev, pspec,
	    vpspec);
	printf("fdisk_part_spec_prepare() - block size\n");
	block_size = dev->dinfo.block_size;
	printf("fdisk_part_spec_prepare() - cbytes\n");
	cbytes = pspec->capacity.value;
	printf("fdisk_part_spec_prepare() - cunit\n");
	for (i = 0; i < pspec->capacity.cunit; i++)
		cbytes = cbytes * 1000;

	printf("fdisk_part_spec_prepare() - req_blocks block_size=%zu\n",
	    block_size);
	req_blocks = (cbytes + block_size - 1) / block_size;
	req_blocks = fdisk_ba_align_up(dev, req_blocks);

	pcnt = -1;

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
	}

	if (pcnt < 0)
		return EINVAL;

	printf("fdisk_part_spec_prepare() - switch\n");
	switch (pspec->pkind) {
	case lpk_primary:
	case lpk_extended:
		printf("fdisk_part_spec_prepare() - pri/ext\n");
		rc = fdisk_part_get_free_idx(dev, &index);
		if (rc != EOK)
			return EIO;

		printf("fdisk_part_spec_prepare() - get free range\n");
		rc = fdisk_part_get_free_range(dev, req_blocks, &fblock0, &fnblocks);
		if (rc != EOK)
			return EIO;

		printf("fdisk_part_spec_prepare() - memset\n");
		memset(vpspec, 0, sizeof(vbd_part_spec_t));
		vpspec->index = index;
		vpspec->block0 = fblock0;
		vpspec->nblocks = req_blocks;
		vpspec->pkind = pspec->pkind;
		break;
	case lpk_logical:
		printf("fdisk_part_spec_prepare() - log\n");
		rc = fdisk_part_get_log_free_range(dev, req_blocks, &fhdr,
		    &fblock0, &fnblocks);
		if (rc != EOK)
			return EIO;

		memset(vpspec, 0, sizeof(vbd_part_spec_t));
		vpspec->hdr_blocks = fhdr;
		vpspec->block0 = fblock0;
		vpspec->nblocks = req_blocks;
		vpspec->pkind = lpk_logical;
		vpspec->ptype.fmt = lptf_num;
		vpspec->ptype.t.num = 42;
		break;
	}

	if (pspec->pkind != lpk_extended) {
		rc = vbd_suggest_ptype(dev->fdisk->vbd, dev->sid, pcnt,
		    &vpspec->ptype);
		if (rc != EOK)
			return EIO;
	}

	return EOK;
}

static int fdisk_update_dev_info(fdisk_dev_t *dev)
{
	int rc;
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

/** @}
 */
