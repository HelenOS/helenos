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
	fdisk_t *fdisk;
	int rc;

	fdisk = calloc(1, sizeof(fdisk_t));
	if (fdisk == NULL)
		return ENOMEM;

	rc = vol_create(&fdisk->vol);
	if (rc != EOK) {
		free(fdisk);
		return EIO;
	}

	*rfdisk = fdisk;
	return EOK;
}

void fdisk_destroy(fdisk_t *fdisk)
{
	vol_destroy(fdisk->vol);
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

	rc = vol_get_disks(fdisk->vol, &svcs, &count);
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

int fdisk_dev_open(fdisk_t *fdisk, service_id_t sid, fdisk_dev_t **rdev)
{
	fdisk_dev_t *dev;

	dev = calloc(1, sizeof(fdisk_dev_t));
	if (dev == NULL)
		return ENOMEM;

	dev->fdisk = fdisk;
	dev->sid = sid;
	list_initialize(&dev->parts);
	*rdev = dev;
	return EOK;
}

void fdisk_dev_close(fdisk_dev_t *dev)
{
	free(dev);
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
	vol_disk_info_t vinfo;
	int rc;

	rc = vol_disk_info(dev->fdisk->vol, dev->sid, &vinfo);
	if (rc != EOK) {
		rc = EIO;
		goto error;
	}

	info->dcnt = vinfo.dcnt;
	info->ltype = vinfo.ltype;
	return EOK;
error:
	return rc;
}

int fdisk_label_create(fdisk_dev_t *dev, label_type_t ltype)
{
	return vol_label_create(dev->fdisk->vol, dev->sid, ltype);
}

int fdisk_label_destroy(fdisk_dev_t *dev)
{
	fdisk_part_t *part;
	int rc;

	part = fdisk_part_first(dev);
	while (part != NULL) {
		(void) fdisk_part_destroy(part); /* XXX */
		part = fdisk_part_first(dev);
	}

	rc = vol_disk_empty(dev->fdisk->vol, dev->sid);
	if (rc != EOK)
		return EIO;

	dev->dcnt = dc_empty;
	return EOK;
}

fdisk_part_t *fdisk_part_first(fdisk_dev_t *dev)
{
	link_t *link;

	link = list_first(&dev->parts);
	if (link == NULL)
		return NULL;

	return list_get_instance(link, fdisk_part_t, ldev);
}

fdisk_part_t *fdisk_part_next(fdisk_part_t *part)
{
	link_t *link;

	link = list_next(&part->ldev, &part->dev->parts);
	if (link == NULL)
		return NULL;

	return list_get_instance(link, fdisk_part_t, ldev);
}

int fdisk_part_get_info(fdisk_part_t *part, fdisk_part_info_t *info)
{
	info->capacity = part->capacity;
	info->fstype = part->fstype;
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

	part = calloc(1, sizeof(fdisk_part_t));
	if (part == NULL)
		return ENOMEM;

	part->dev = dev;
	list_append(&part->ldev, &dev->parts);
	part->capacity = pspec->capacity;
	part->fstype = pspec->fstype;

	if (rpart != NULL)
		*rpart = part;
	return EOK;
}

int fdisk_part_destroy(fdisk_part_t *part)
{
	list_remove(&part->ldev);
	free(part);
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

int fdisk_fstype_format(fdisk_fstype_t fstype, char **rstr)
{
	const char *sfstype;
	char *s;

	sfstype = NULL;
	switch (fstype) {
	case fdfs_none:
		sfstype = "None";
		break;
	case fdfs_unknown:
		sfstype = "Unknown";
		break;
	case fdfs_exfat:
		sfstype = "ExFAT";
		break;
	case fdfs_fat:
		sfstype = "FAT";
		break;
	case fdfs_minix:
		sfstype = "MINIX";
		break;
	case fdfs_ext4:
		sfstype = "Ext4";
		break;
	}

	s = str_dup(sfstype);
	if (s == NULL)
		return ENOMEM;

	*rstr = s;
	return EOK;
}

/** @}
 */
