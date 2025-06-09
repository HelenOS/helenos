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

#include "../../../util.h"
#include "../../../var.h"

#include "g_mirror.h"

static void *meta_gmirror_alloc_struct(void);
static errno_t meta_gmirror_init_vol2meta(const hr_volume_t *, void *);
static errno_t meta_gmirror_init_meta2vol(const list_t *, hr_volume_t *);
static void meta_gmirror_encode(void *, void *);
static errno_t meta_gmirror_decode(const void *, void *);
static errno_t meta_gmirror_get_block(service_id_t, void **);
static errno_t meta_gmirror_write_block(service_id_t, const void *);
static bool meta_gmirror_has_valid_magic(const void *);
static bool meta_gmirror_compare_uuids(const void *, const void *);
static void meta_gmirror_inc_counter(void *);
static errno_t meta_gmirror_save(hr_volume_t *, bool);
static const char *meta_gmirror_get_devname(const void *);
static hr_level_t meta_gmirror_get_level(const void *);
static uint64_t meta_gmirror_get_data_offset(void);
static size_t meta_gmirror_get_size(void);
static uint8_t meta_gmirror_get_flags(void);
static hr_metadata_type_t meta_gmirror_get_type(void);
static void meta_gmirror_dump(const void *);

hr_superblock_ops_t metadata_gmirror_ops = {
	.alloc_struct = meta_gmirror_alloc_struct,
	.init_vol2meta = meta_gmirror_init_vol2meta,
	.init_meta2vol = meta_gmirror_init_meta2vol,
	.encode = meta_gmirror_encode,
	.decode = meta_gmirror_decode,
	.get_block = meta_gmirror_get_block,
	.write_block = meta_gmirror_write_block,
	.has_valid_magic = meta_gmirror_has_valid_magic,
	.compare_uuids = meta_gmirror_compare_uuids,
	.inc_counter = meta_gmirror_inc_counter,
	.save = meta_gmirror_save,
	.get_devname = meta_gmirror_get_devname,
	.get_level = meta_gmirror_get_level,
	.get_data_offset = meta_gmirror_get_data_offset,
	.get_size = meta_gmirror_get_size,
	.get_flags = meta_gmirror_get_flags,
	.get_type = meta_gmirror_get_type,
	.dump = meta_gmirror_dump
};

static void *meta_gmirror_alloc_struct(void)
{
	return calloc(1, sizeof(struct g_mirror_metadata));
}

static errno_t meta_gmirror_init_vol2meta(const hr_volume_t *vol, void *md_v)
{
	(void)vol;
	(void)md_v;

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

		if (iter_meta->md_genid >= max_counter_val) {
			max_counter_val = iter_meta->md_genid;
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

	memcpy(vol->in_mem_md, main_meta, sizeof(struct g_mirror_metadata));

	uint8_t index = 0;
	list_foreach(*list, link, struct dev_list_member, iter) {
		struct g_mirror_metadata *iter_meta = iter->md;

		vol->extents[index].svc_id = iter->svc_id;
		iter->fini = false;

		/* for now no md_sync_offset handling for saved REBUILD */
		if (iter_meta->md_genid == max_counter_val)
			vol->extents[index].state = HR_EXT_ONLINE;
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

	rc = block_read_direct(dev, blkno - 1, 1, block);
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

	rc = block_write_direct(dev, blkno - 1, 1, block);

	return rc;
}

static bool meta_gmirror_has_valid_magic(const void *md_v)
{
	HR_DEBUG("%s()", __func__);

	const struct g_mirror_metadata *md = md_v;

	if (str_lcmp(md->md_magic, G_MIRROR_MAGIC, 16) != 0)
		return false;

	return true;
}

static bool meta_gmirror_compare_uuids(const void *m1_v, const void *m2_v)
{
	const struct g_mirror_metadata *m1 = m1_v;
	const struct g_mirror_metadata *m2 = m2_v;
	if (m1->md_mid == m2->md_mid)
		return true;

	return false;
}

static void meta_gmirror_inc_counter(void *md_v)
{
	struct g_mirror_metadata *md = md_v;

	/* XXX: probably md_genid and not md_syncid is incremented */
	md->md_genid++;
}

static errno_t meta_gmirror_save(hr_volume_t *vol, bool with_state_callback)
{
	HR_DEBUG("%s()", __func__);

	(void)vol;
	(void)with_state_callback;

	/*
	 * cannot support right now, because would need to store the
	 * metadata for all disks, because of hardcoded provider names and
	 * most importantly, disk unique ids
	 */

	/* silent */
	return EOK;
	/* return ENOTSUP; */
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

/** @}
 */
