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
#include <types/uuid.h>

#include "../../../io.h"
#include "../../../util.h"
#include "../../../var.h"

#include "g_mirror.h"

/* not exposed */
static void *meta_gmirror_alloc_struct(void);
static void meta_gmirror_encode(void *, void *);
static errno_t meta_gmirror_decode(const void *, void *);
static errno_t meta_gmirror_get_block(service_id_t, void **);
static errno_t meta_gmirror_write_block(service_id_t, const void *);

static errno_t meta_gmirror_probe(service_id_t, void **);
static errno_t meta_gmirror_init_vol2meta(hr_volume_t *);
static errno_t meta_gmirror_init_meta2vol(const list_t *, hr_volume_t *);
static errno_t meta_gmirror_erase_block(service_id_t);
static bool meta_gmirror_compare_uuids(const void *, const void *);
static void meta_gmirror_inc_counter(hr_volume_t *);
static errno_t meta_gmirror_save(hr_volume_t *, bool);
static errno_t meta_gmirror_save_ext(hr_volume_t *, size_t, bool);
static const char *meta_gmirror_get_devname(const void *);
static hr_level_t meta_gmirror_get_level(const void *);
static uint64_t meta_gmirror_get_data_offset(void);
static size_t meta_gmirror_get_size(void);
static uint8_t meta_gmirror_get_flags(void);
static hr_metadata_type_t meta_gmirror_get_type(void);
static void meta_gmirror_dump(const void *);

hr_superblock_ops_t metadata_gmirror_ops = {
	.probe = meta_gmirror_probe,
	.init_vol2meta = meta_gmirror_init_vol2meta,
	.init_meta2vol = meta_gmirror_init_meta2vol,
	.erase_block = meta_gmirror_erase_block,
	.compare_uuids = meta_gmirror_compare_uuids,
	.inc_counter = meta_gmirror_inc_counter,
	.save = meta_gmirror_save,
	.save_ext = meta_gmirror_save_ext,
	.get_devname = meta_gmirror_get_devname,
	.get_level = meta_gmirror_get_level,
	.get_data_offset = meta_gmirror_get_data_offset,
	.get_size = meta_gmirror_get_size,
	.get_flags = meta_gmirror_get_flags,
	.get_type = meta_gmirror_get_type,
	.dump = meta_gmirror_dump
};

static errno_t meta_gmirror_probe(service_id_t svc_id, void **rmd)
{
	errno_t rc;
	void *meta_block;

	void *metadata_struct = meta_gmirror_alloc_struct();
	if (metadata_struct == NULL)
		return ENOMEM;

	rc = meta_gmirror_get_block(svc_id, &meta_block);
	if (rc != EOK)
		goto error;

	rc = meta_gmirror_decode(meta_block, metadata_struct);

	free(meta_block);

	if (rc != EOK)
		goto error;

	*rmd = metadata_struct;
	return EOK;

error:
	free(metadata_struct);
	return rc;
}

static errno_t meta_gmirror_init_vol2meta(hr_volume_t *vol)
{
	(void)vol;

	return ENOTSUP;
}

static errno_t meta_gmirror_init_meta2vol(const list_t *list, hr_volume_t *vol)
{
	HR_DEBUG("%s()", __func__);

	errno_t rc = EOK;

	struct g_mirror_metadata *main_meta = NULL;
	uint64_t max_counter_val = 0;

	list_foreach(*list, link, struct dev_list_member, iter) {
		struct g_mirror_metadata *iter_meta = iter->md;

		if (iter_meta->md_syncid >= max_counter_val) {
			max_counter_val = iter_meta->md_syncid;
			main_meta = iter_meta;
		}
	}

	assert(main_meta != NULL);

	vol->truncated_blkno =
	    main_meta->md_mediasize / main_meta->md_sectorsize;

	vol->data_blkno = vol->truncated_blkno - 1;

	vol->data_offset = 0;

	if (main_meta->md_all > HR_MAX_EXTENTS) {
		HR_DEBUG("Assembled volume has %u extents (max = %u)",
		    (unsigned)main_meta->md_all, HR_MAX_EXTENTS);
		rc = EINVAL;
		goto error;
	}

	vol->extent_no = main_meta->md_all;

	vol->layout = HR_LAYOUT_NONE;

	vol->strip_size = 0;

	vol->bsize = main_meta->md_sectorsize;

	vol->in_mem_md =
	    calloc(vol->extent_no, sizeof(struct g_mirror_metadata));
	if (vol->in_mem_md == NULL)
		return ENOMEM;
	memcpy(vol->in_mem_md, main_meta, sizeof(struct g_mirror_metadata));

	bool rebuild_set = false;

	uint8_t index = 0;
	list_foreach(*list, link, struct dev_list_member, iter) {
		struct g_mirror_metadata *iter_meta = iter->md;

		struct g_mirror_metadata *p =
		    ((struct g_mirror_metadata *)vol->in_mem_md) + index;
		memcpy(p, iter_meta, sizeof(*p));

		vol->extents[index].svc_id = iter->svc_id;
		iter->fini = false;

		bool invalidate = false;
		bool rebuild_this_ext = false;

		if (iter_meta->md_dflags & G_MIRROR_DISK_FLAG_DIRTY)
			invalidate = true;
		if (iter_meta->md_syncid != max_counter_val)
			invalidate = true;

		if (iter_meta->md_dflags & G_MIRROR_DISK_FLAG_SYNCHRONIZING &&
		    !invalidate) {
			if (rebuild_set) {
				HR_DEBUG("only 1 rebuilt extent allowed");
				rc = EINVAL;
				goto error;
			}
			rebuild_set = true;
			rebuild_this_ext = true;
			vol->rebuild_blk = iter_meta->md_sync_offset;
		}

		if (!rebuild_this_ext && !invalidate)
			vol->extents[index].state = HR_EXT_ONLINE;
		else if (rebuild_this_ext && !invalidate)
			vol->extents[index].state = HR_EXT_REBUILD;
		else
			vol->extents[index].state = HR_EXT_INVALID;

		index++;
	}

	for (size_t i = 0; i < vol->extent_no; i++) {
		if (vol->extents[i].state == HR_EXT_NONE)
			vol->extents[i].state = HR_EXT_MISSING;
	}

error:
	return rc;
}

static errno_t meta_gmirror_erase_block(service_id_t dev)
{
	HR_DEBUG("%s()", __func__);

	(void)dev;

	return ENOTSUP;
}

static bool meta_gmirror_compare_uuids(const void *m1_v, const void *m2_v)
{
	const struct g_mirror_metadata *m1 = m1_v;
	const struct g_mirror_metadata *m2 = m2_v;
	if (m1->md_mid == m2->md_mid)
		return true;

	return false;
}

static void meta_gmirror_inc_counter(hr_volume_t *vol)
{
	fibril_mutex_lock(&vol->md_lock);

	for (size_t d = 0; d < vol->extent_no; d++) {
		struct g_mirror_metadata *md =
		    ((struct g_mirror_metadata *)vol->in_mem_md) + d;
		md->md_syncid++;
	}

	fibril_mutex_unlock(&vol->md_lock);
}

static errno_t meta_gmirror_save(hr_volume_t *vol, bool with_state_callback)
{
	HR_DEBUG("%s()", __func__);

	fibril_rwlock_read_lock(&vol->extents_lock);

	for (size_t i = 0; i < vol->extent_no; i++)
		meta_gmirror_save_ext(vol, i, with_state_callback);

	fibril_rwlock_read_unlock(&vol->extents_lock);

	return EOK;
}

static errno_t meta_gmirror_save_ext(hr_volume_t *vol, size_t ext_idx,
    bool with_state_callback)
{
	HR_DEBUG("%s()", __func__);

	assert(fibril_rwlock_is_locked(&vol->extents_lock));

	void *md_block = hr_calloc_waitok(1, vol->bsize);

	struct g_mirror_metadata *md =
	    ((struct g_mirror_metadata *)vol->in_mem_md) + ext_idx;

	hr_extent_t *ext = &vol->extents[ext_idx];

	fibril_rwlock_read_lock(&vol->states_lock);
	hr_ext_state_t s = ext->state;
	fibril_rwlock_read_unlock(&vol->states_lock);

	if (s != HR_EXT_ONLINE && s != HR_EXT_REBUILD) {
		return EINVAL;
	}

	fibril_mutex_lock(&vol->md_lock);

	if (s == HR_EXT_REBUILD) {
		md->md_sync_offset = vol->rebuild_blk;
		md->md_dflags |= G_MIRROR_DISK_FLAG_SYNCHRONIZING;
	} else {
		md->md_sync_offset = 0;
		md->md_dflags &= ~(G_MIRROR_DISK_FLAG_SYNCHRONIZING);
	}

	meta_gmirror_encode(md, md_block);
	errno_t rc = meta_gmirror_write_block(ext->svc_id, md_block);
	if (rc != EOK && with_state_callback)
		vol->hr_ops.ext_state_cb(vol, ext_idx, rc);

	fibril_mutex_unlock(&vol->md_lock);

	if (with_state_callback)
		vol->hr_ops.vol_state_eval(vol);

	free(md_block);
	return EOK;
}

static const char *meta_gmirror_get_devname(const void *md_v)
{
	const struct g_mirror_metadata *md = md_v;

	return md->md_name;
}

static hr_level_t meta_gmirror_get_level(const void *md_v)
{
	(void)md_v;

	return HR_LVL_1;
}

static uint64_t meta_gmirror_get_data_offset(void)
{
	return 0;
}

static size_t meta_gmirror_get_size(void)
{
	return 1;
}

static uint8_t meta_gmirror_get_flags(void)
{
	uint8_t flags = 0;

	return flags;
}

static hr_metadata_type_t meta_gmirror_get_type(void)
{
	return 	HR_METADATA_GEOM_MIRROR;
}

static void meta_gmirror_dump(const void *md_v)
{
	HR_DEBUG("%s()", __func__);

	mirror_metadata_dump(md_v);
}

static void *meta_gmirror_alloc_struct(void)
{
	return calloc(1, sizeof(struct g_mirror_metadata));
}
static void meta_gmirror_encode(void *md_v, void *block)
{
	HR_DEBUG("%s()", __func__);

	mirror_metadata_encode(md_v, block);
}

static errno_t meta_gmirror_decode(const void *block, void *md_v)
{
	HR_DEBUG("%s()", __func__);

	errno_t rc = mirror_metadata_decode(block, md_v);
	return rc;
}

static errno_t meta_gmirror_get_block(service_id_t dev, void **rblock)
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

	if (bsize < sizeof(struct g_mirror_metadata))
		return EINVAL;

	rc = block_get_nblocks(dev, &blkno);
	if (rc != EOK)
		return rc;

	if (blkno < 1)
		return EINVAL;

	block = malloc(bsize);
	if (block == NULL)
		return ENOMEM;

	rc = hr_read_direct(dev, blkno - 1, 1, block);
	/*
	 * XXX: here maybe call vol state event or the state callback...
	 *
	 * but need to pass vol pointer
	 */
	if (rc != EOK) {
		free(block);
		return rc;
	}

	*rblock = block;
	return EOK;
}

static errno_t meta_gmirror_write_block(service_id_t dev, const void *block)
{
	HR_DEBUG("%s()", __func__);

	errno_t rc;
	uint64_t blkno;
	size_t bsize;

	rc = block_get_bsize(dev, &bsize);
	if (rc != EOK)
		return rc;

	if (bsize < sizeof(struct g_mirror_metadata))
		return EINVAL;

	rc = block_get_nblocks(dev, &blkno);
	if (rc != EOK)
		return rc;

	if (blkno < 1)
		return EINVAL;

	rc = hr_write_direct(dev, blkno - 1, 1, block);

	return rc;
}

/** @}
 */
