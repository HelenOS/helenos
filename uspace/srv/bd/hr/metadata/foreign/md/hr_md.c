/*
 * Copyright (c) 2025 Miroslav Cimerman
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

/** @addtogroup hr
 * @{
 */
/**
 * @file
 */

#include <adt/list.h>
#include <block.h>
#include <byteorder.h>
#include <errno.h>
#include <inttypes.h>
#include <io/log.h>
#include <loc.h>
#include <mem.h>
#include <uuid.h>
#include <stdlib.h>
#include <stdio.h>
#include <str.h>

#include "../../../io.h"
#include "../../../util.h"
#include "../../../var.h"

#include "md_p.h"

/* not exposed */
static void *meta_md_alloc_struct(void);
static void meta_md_encode(void *, void *);
static errno_t meta_md_decode(const void *, void *);
static errno_t meta_md_get_block(service_id_t, void **);
static errno_t meta_md_write_block(service_id_t, const void *);

static errno_t meta_md_probe(service_id_t, void **);
static errno_t meta_md_init_vol2meta(hr_volume_t *);
static errno_t meta_md_init_meta2vol(const list_t *, hr_volume_t *);
static errno_t meta_md_erase_block(service_id_t);
static bool meta_md_compare_uuids(const void *, const void *);
static void meta_md_inc_counter(hr_volume_t *);
static errno_t meta_md_save(hr_volume_t *, bool);
static errno_t meta_md_save_ext(hr_volume_t *, size_t, bool);
static const char *meta_md_get_devname(const void *);
static hr_level_t meta_md_get_level(const void *);
static uint64_t meta_md_get_data_offset(void);
static size_t meta_md_get_size(void);
static uint8_t meta_md_get_flags(void);
static hr_metadata_type_t meta_md_get_type(void);
static void meta_md_dump(const void *);

hr_superblock_ops_t metadata_md_ops = {
	.probe = meta_md_probe,
	.init_vol2meta = meta_md_init_vol2meta,
	.init_meta2vol = meta_md_init_meta2vol,
	.erase_block = meta_md_erase_block,
	.compare_uuids = meta_md_compare_uuids,
	.inc_counter = meta_md_inc_counter,
	.save = meta_md_save,
	.save_ext = meta_md_save_ext,
	.get_devname = meta_md_get_devname,
	.get_level = meta_md_get_level,
	.get_data_offset = meta_md_get_data_offset,
	.get_size = meta_md_get_size,
	.get_flags = meta_md_get_flags,
	.get_type = meta_md_get_type,
	.dump = meta_md_dump
};

static errno_t meta_md_probe(service_id_t svc_id, void **rmd)
{
	errno_t rc;
	void *meta_block;

	void *metadata_struct = meta_md_alloc_struct();
	if (metadata_struct == NULL)
		return ENOMEM;

	rc = meta_md_get_block(svc_id, &meta_block);
	if (rc != EOK)
		goto error;

	rc = meta_md_decode(meta_block, metadata_struct);

	free(meta_block);

	if (rc != EOK)
		goto error;

	*rmd = metadata_struct;
	return EOK;

error:
	free(metadata_struct);
	return rc;
}

static errno_t meta_md_init_vol2meta(hr_volume_t *vol)
{
	(void)vol;

	return ENOTSUP;
}

static errno_t meta_md_init_meta2vol(const list_t *list, hr_volume_t *vol)
{
	HR_DEBUG("%s()", __func__);

	errno_t rc = EOK;

	struct mdp_superblock_1 *main_meta = NULL;
	uint64_t max_events = 0;

	list_foreach(*list, link, struct dev_list_member, iter) {
		struct mdp_superblock_1 *iter_meta = iter->md;

		if (iter_meta->events >= max_events) {
			max_events = iter_meta->events;
			main_meta = iter_meta;
		}
	}

	assert(main_meta != NULL);

	vol->bsize = 512;

	vol->truncated_blkno = main_meta->size;

	vol->extent_no = main_meta->raid_disks;

	switch (vol->level) {
	case HR_LVL_0:
		vol->data_blkno = vol->truncated_blkno * vol->extent_no;
		vol->layout = HR_LAYOUT_NONE;
		break;
	case HR_LVL_1:
		vol->data_blkno = vol->truncated_blkno;
		vol->layout = HR_LAYOUT_NONE;
		break;
	case HR_LVL_4:
		vol->data_blkno = vol->truncated_blkno * (vol->extent_no - 1);
		vol->layout = HR_LAYOUT_RAID4_N;
		break;
	case HR_LVL_5:
		vol->data_blkno = vol->truncated_blkno * (vol->extent_no - 1);
		switch (main_meta->layout) {
		case ALGORITHM_LEFT_ASYMMETRIC:
			vol->layout = HR_LAYOUT_RAID5_NR;
			break;
		case ALGORITHM_RIGHT_ASYMMETRIC:
			vol->layout = HR_LAYOUT_RAID5_0R;
			break;
		case ALGORITHM_LEFT_SYMMETRIC:
			vol->layout = HR_LAYOUT_RAID5_NC;
			break;
		}
		break;
	default:
		return EINVAL;
	}

	vol->data_offset = main_meta->data_offset;

	vol->strip_size = main_meta->chunksize * 512;

	vol->in_mem_md = calloc(vol->extent_no, MD_SIZE * 512);
	if (vol->in_mem_md == NULL)
		return ENOMEM;

	size_t i = 0;
	list_foreach(*list, link, struct dev_list_member, iter) {
		struct mdp_superblock_1 *iter_meta = iter->md;

		uint8_t index = iter_meta->dev_roles[iter_meta->dev_number];

		struct mdp_superblock_1 *p = (struct mdp_superblock_1 *)
		    (((char *)vol->in_mem_md) + MD_SIZE * 512 * index);
		memcpy(p, iter_meta, MD_SIZE * 512);

		vol->extents[index].svc_id = iter->svc_id;
		iter->fini = false;

		bool invalidate = false;

		if (iter_meta->events != max_events)
			invalidate = true;

		if (iter_meta->feature_map & MD_DISK_SYNC)
			invalidate = true;

		if (!invalidate)
			vol->extents[index].state = HR_EXT_ONLINE;
		else
			vol->extents[index].state = HR_EXT_INVALID;

		i++;
		if (i == vol->extent_no)
			break;
	}

	for (size_t i = 0; i < vol->extent_no; i++) {
		if (vol->extents[i].state == HR_EXT_NONE)
			vol->extents[i].state = HR_EXT_MISSING;
	}

	return rc;
}

static errno_t meta_md_erase_block(service_id_t dev)
{
	HR_DEBUG("%s()", __func__);

	(void)dev;

	return ENOTSUP;
}

static bool meta_md_compare_uuids(const void *m1_v, const void *m2_v)
{
	const struct mdp_superblock_1 *m1 = m1_v;
	const struct mdp_superblock_1 *m2 = m2_v;
	if (memcmp(&m1->set_uuid, &m2->set_uuid, 16) == 0)
		return true;

	return false;
}

static void meta_md_inc_counter(hr_volume_t *vol)
{
	fibril_mutex_lock(&vol->md_lock);

	for (size_t d = 0; d < vol->extent_no; d++) {
		struct mdp_superblock_1 *md = (struct mdp_superblock_1 *)
		    (((uint8_t *)vol->in_mem_md) + MD_SIZE * 512 * d);
		md->events++;
	}

	fibril_mutex_unlock(&vol->md_lock);
}

static errno_t meta_md_save(hr_volume_t *vol, bool with_state_callback)
{
	HR_DEBUG("%s()", __func__);

	fibril_rwlock_read_lock(&vol->extents_lock);

	for (size_t i = 0; i < vol->extent_no; i++)
		meta_md_save_ext(vol, i, with_state_callback);

	fibril_rwlock_read_unlock(&vol->extents_lock);

	return EOK;
}

static errno_t meta_md_save_ext(hr_volume_t *vol, size_t ext_idx,
    bool with_state_callback)
{
	HR_DEBUG("%s()", __func__);

	assert(fibril_rwlock_is_locked(&vol->extents_lock));

	void *md_block = hr_calloc_waitok(1, MD_SIZE * 512);

	struct mdp_superblock_1 *md = (struct mdp_superblock_1 *)
	    (((uint8_t *)vol->in_mem_md) + MD_SIZE * 512 * ext_idx);

	hr_extent_t *ext = &vol->extents[ext_idx];

	fibril_rwlock_read_lock(&vol->states_lock);
	hr_ext_state_t s = ext->state;
	fibril_rwlock_read_unlock(&vol->states_lock);

	if (s != HR_EXT_ONLINE && s != HR_EXT_REBUILD) {
		return EINVAL;
	}

	fibril_mutex_lock(&vol->md_lock);

	if (s == HR_EXT_REBUILD) {
		md->resync_offset = vol->rebuild_blk;
		md->feature_map = MD_DISK_SYNC;
	} else {
		md->resync_offset = 0;
		md->feature_map = 0;
	}

	meta_md_encode(md, md_block);
	errno_t rc = meta_md_write_block(ext->svc_id, md_block);
	if (rc != EOK && with_state_callback)
		vol->hr_ops.ext_state_cb(vol, ext_idx, rc);

	fibril_mutex_unlock(&vol->md_lock);

	if (with_state_callback)
		vol->hr_ops.vol_state_eval(vol);

	free(md_block);

	return EOK;
}

static const char *meta_md_get_devname(const void *md_v)
{
	const struct mdp_superblock_1 *md = md_v;

	return md->set_name;
}

static hr_level_t meta_md_get_level(const void *md_v)
{
	const struct mdp_superblock_1 *md = md_v;

	switch (md->level) {
	case 0:
		return HR_LVL_0;
	case 1:
		return HR_LVL_1;
	case 4:
		return HR_LVL_4;
	case 5:
		return HR_LVL_5;
	default:
		return HR_LVL_UNKNOWN;
	}
}

static uint64_t meta_md_get_data_offset(void)
{
	return MD_DATA_OFFSET;
}

static size_t meta_md_get_size(void)
{
	return MD_SIZE;
}

static uint8_t meta_md_get_flags(void)
{
	uint8_t flags = 0;

	return flags;
}

static hr_metadata_type_t meta_md_get_type(void)
{
	return HR_METADATA_MD;
}

static void bytefield_print(const uint8_t *d, size_t s)
{
	size_t i;

	for (i = 0; i < s; i++)
		printf("%02x", d[i]);
}

static void meta_md_dump(const void *md_v)
{
	HR_DEBUG("%s()", __func__);

	const struct mdp_superblock_1 *md = md_v;

	printf("magic: 0x%" PRIx32 "\n", md->magic);

	printf("major_version: %" PRIu32 "\n", md->major_version);

	printf("feature_map: 0x%" PRIx32 "\n", md->feature_map);

	printf("set_uuid: ");
	bytefield_print(md->set_uuid, 16);
	printf("\n");

	printf("set_name: %s\n", md->set_name);

	printf("level: %" PRIi32 "\n", md->level);

	printf("layout: %" PRIi32 "\n", md->layout);

	printf("size: %" PRIu64 "\n", md->size);

	printf("chunksize: %" PRIu32 "\n", md->chunksize);

	printf("raid_disks: %" PRIu32 "\n", md->raid_disks);

	printf("data_offset: %" PRIu64 "\n", md->data_offset);

	printf("data_size: %" PRIu64 "\n", md->data_size);

	printf("super_offset: %" PRIu64 "\n", md->super_offset);

	printf("dev_number: %" PRIu32 "\n", md->dev_number);

	printf("device_uuid: ");
	bytefield_print(md->device_uuid, 16);
	printf("\n");

	printf("events: %" PRIu64 "\n", md->events);

	if (md->resync_offset == ~(0ULL))
		printf("resync_offset: 0\n");
	else
		printf("resync_offset: %" PRIu64 "\n", md->resync_offset);

	printf("max_dev: %" PRIu32 "\n", md->max_dev);

	printf("dev_roles: ");
	for (uint32_t d = 0; d < md->max_dev; d++) {
		printf("0x%" PRIx16, md->dev_roles[d]);
		if (d + 1 < md->max_dev)
			printf(", ");
	}
	printf("\n");
}

static void *meta_md_alloc_struct(void)
{
	/* 1KiB - 256 meta size + max 384 devices */
	return calloc(1, MD_SIZE * 512);
}

static void meta_md_encode(void *md_v, void *block)
{
	HR_DEBUG("%s()", __func__);

	memcpy(block, md_v, meta_md_get_size() * 512);

	struct mdp_superblock_1 *md = block;

	md->magic = host2uint32_t_le(md->magic);
	md->major_version = host2uint32_t_le(md->major_version);
	md->feature_map = host2uint32_t_le(md->feature_map);
	md->level = host2uint32_t_le(md->level);
	md->layout = host2uint32_t_le(md->layout);
	md->size = host2uint64_t_le(md->size);
	md->chunksize = host2uint32_t_le(md->chunksize);
	md->raid_disks = host2uint32_t_le(md->raid_disks);
	md->data_offset = host2uint64_t_le(md->data_offset);
	md->data_size = host2uint64_t_le(md->data_size);
	md->super_offset = host2uint64_t_le(md->super_offset);
	md->dev_number = host2uint32_t_le(md->dev_number);
	md->events = host2uint64_t_le(md->events);
	md->resync_offset = host2uint64_t_le(md->resync_offset);
	md->max_dev = host2uint32_t_le(md->max_dev);
	for (uint32_t d = 0; d < md->max_dev; d++)
		md->dev_roles[d] = host2uint16_t_le(md->dev_roles[d]);

	md->sb_csum = calc_sb_1_csum(md);
}

static errno_t meta_md_decode(const void *block, void *md_v)
{
	HR_DEBUG("%s()", __func__);

	errno_t rc = EOK;
	struct mdp_superblock_1 *md = md_v;

	/*
	 * Do in-place decoding to cpu byte order.
	 * We do it like this because:
	 * 1) we do not know what is after the 256 bytes
	 * of the struct, so we write back what was there
	 * previously,
	 * 2) we do not want to deal unused fields such
	 * as unions and so on.
	 */
	memcpy(md, block, meta_md_get_size() * 512);

	md->magic = uint32_t_le2host(md->magic);
	if (md->magic != MD_MAGIC) {
		rc = EINVAL;
		goto error;
	}

	md->major_version = uint32_t_le2host(md->major_version);
	if (md->major_version != 1) {
		HR_DEBUG("unsupported metadata version\n");
		rc = EINVAL;
		goto error;
	}

	md->feature_map = uint32_t_le2host(md->feature_map);
	/* XXX: do not even support MD_DISK_SYNC here */
	if (md->feature_map != 0x0) {
		HR_DEBUG("unsupported feature map bits\n");
		rc = EINVAL;
		goto error;
	}

	md->level = uint32_t_le2host(md->level);
	switch (md->level) {
	case 0:
	case 1:
	case 4:
	case 5:
		break;
	default:
		HR_DEBUG("unsupported level\n");
		rc = EINVAL;
		goto error;
	}

	md->layout = uint32_t_le2host(md->layout);
	if (md->level == 5) {
		switch (md->layout) {
		case ALGORITHM_LEFT_ASYMMETRIC:
		case ALGORITHM_RIGHT_ASYMMETRIC:
		case ALGORITHM_LEFT_SYMMETRIC:
			break;
		default:
			HR_DEBUG("unsupported layout\n");
			rc = EINVAL;
			goto error;
		}
	} else if (md->level == 4) {
		if (md->layout != 0) {
			HR_DEBUG("unsupported layout\n");
			rc = EINVAL;
			goto error;
		}
	}

	md->size = uint64_t_le2host(md->size);

	md->chunksize = uint32_t_le2host(md->chunksize);

	md->raid_disks = uint32_t_le2host(md->raid_disks);
	if (md->raid_disks > HR_MAX_EXTENTS) {
		rc = EINVAL;
		goto error;
	}

	md->data_offset = uint64_t_le2host(md->data_offset);

	md->data_size = uint64_t_le2host(md->data_size);
	if (md->data_size != md->size) {
		rc = EINVAL;
		goto error;
	}

	md->super_offset = uint64_t_le2host(md->super_offset);
	if (md->super_offset != MD_OFFSET) {
		rc = EINVAL;
		goto error;
	}

	md->dev_number = uint32_t_le2host(md->dev_number);

	md->events = uint64_t_le2host(md->events);

	md->resync_offset = uint64_t_le2host(md->resync_offset);
	if (md->feature_map == 0 && md->resync_offset != ~(0ULL)) {
		rc = EINVAL;
		goto error;
	}

	md->max_dev = uint32_t_le2host(md->max_dev);
	if (md->max_dev > 256 + 128) {
		rc = EINVAL;
		goto error;
	}

	for (uint32_t d = 0; d < md->max_dev; d++)
		md->dev_roles[d] = uint16_t_le2host(md->dev_roles[d]);

error:
	return rc;
}

static errno_t meta_md_get_block(service_id_t dev, void **rblock)
{
	HR_DEBUG("%s()", __func__);

	errno_t rc;
	uint64_t blkno;
	size_t bsize;
	void *block;

	if (rblock == NULL)
		return EINVAL;

	rc = block_get_bsize(dev, &bsize);
	if (rc != EOK)
		return rc;

	if (bsize != 512)
		return EINVAL;

	rc = block_get_nblocks(dev, &blkno);
	if (rc != EOK)
		return rc;

	if (blkno < MD_OFFSET + MD_SIZE)
		return EINVAL;

	block = malloc(bsize * MD_SIZE);
	if (block == NULL)
		return ENOMEM;

	rc = hr_read_direct(dev, MD_OFFSET, MD_SIZE, block);
	if (rc != EOK) {
		free(block);
		return rc;
	}

	*rblock = block;
	return EOK;
}

static errno_t meta_md_write_block(service_id_t dev, const void *block)
{
	HR_DEBUG("%s()", __func__);

	errno_t rc;
	uint64_t blkno;
	size_t bsize;

	rc = block_get_bsize(dev, &bsize);
	if (rc != EOK)
		return rc;

	if (bsize != 512)
		return EINVAL;

	rc = block_get_nblocks(dev, &blkno);
	if (rc != EOK)
		return rc;

	if (blkno < MD_OFFSET + MD_SIZE)
		return EINVAL;

	rc = hr_write_direct(dev, MD_OFFSET, MD_SIZE, block);

	return rc;
}

/** @}
 */
