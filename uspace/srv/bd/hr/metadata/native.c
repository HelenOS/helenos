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

#include "../util.h"
#include "../var.h"

#include "native.h"

static void		*meta_native_alloc_struct(void);
static errno_t		 meta_native_init_vol2meta(const hr_volume_t *, void *);
static errno_t		 meta_native_init_meta2vol(const list_t *,
    hr_volume_t *);
static void		 meta_native_encode(void *, void *);
static errno_t		 meta_native_decode(const void *, void *);
static errno_t		 meta_native_get_block(service_id_t, void **);
static errno_t		 meta_native_write_block(service_id_t, const void *);
static bool		 meta_native_has_valid_magic(const void *);
static bool		 meta_native_compare_uuids(const void *, const void *);
static void		 meta_native_inc_counter(void *);
static errno_t		 meta_native_save(hr_volume_t *, bool);
static const char	*meta_native_get_devname(const void *);
static hr_level_t	 meta_native_get_level(const void *);
static uint64_t		 meta_native_get_data_offset(void);
static size_t		 meta_native_get_size(void);
static uint8_t		 meta_native_get_flags(void);
static hr_metadata_type_t meta_native_get_type(void);
static void		 meta_native_dump(const void *);

hr_superblock_ops_t metadata_native_ops = {
	.alloc_struct		= meta_native_alloc_struct,
	.init_vol2meta		= meta_native_init_vol2meta,
	.init_meta2vol		= meta_native_init_meta2vol,
	.encode			= meta_native_encode,
	.decode			= meta_native_decode,
	.get_block		= meta_native_get_block,
	.write_block		= meta_native_write_block,
	.has_valid_magic	= meta_native_has_valid_magic,
	.compare_uuids		= meta_native_compare_uuids,
	.inc_counter		= meta_native_inc_counter,
	.save			= meta_native_save,
	.get_devname		= meta_native_get_devname,
	.get_level		= meta_native_get_level,
	.get_data_offset	= meta_native_get_data_offset,
	.get_size		= meta_native_get_size,
	.get_flags		= meta_native_get_flags,
	.get_type		= meta_native_get_type,
	.dump			= meta_native_dump
};

static void *meta_native_alloc_struct(void)
{
	return calloc(1, sizeof(hr_metadata_t));
}

static errno_t meta_native_init_vol2meta(const hr_volume_t *vol, void *md_v)
{
	HR_DEBUG("%s()", __func__);

	hr_metadata_t *md = md_v;

	str_cpy(md->magic, HR_NATIVE_MAGIC_SIZE, HR_NATIVE_MAGIC_STR);

	md->version = HR_NATIVE_METADATA_VERSION;

	md->counter = 0;

	uuid_t uuid;
	/* rndgen */
	fibril_usleep(1000);
	errno_t rc = uuid_generate(&uuid);
	if (rc != EOK)
		return rc;

	/* XXX: for now we just copy byte by byte as "encoding" */
	memcpy(md->uuid, &uuid, HR_NATIVE_UUID_LEN);
	/* uuid_encode(&uuid, metadata->uuid); */

	md->data_blkno = vol->data_blkno;
	md->truncated_blkno = vol->truncated_blkno;
	md->data_offset = vol->data_offset;
	md->extent_no = vol->extent_no;
	md->level = vol->level;
	md->layout = vol->layout;
	md->strip_size = vol->strip_size;
	md->bsize = vol->bsize;
	memcpy(md->devname, vol->devname, HR_DEVNAME_LEN);

	return EOK;
}

static errno_t meta_native_init_meta2vol(const list_t *list, hr_volume_t *vol)
{
	HR_DEBUG("%s()", __func__);

	hr_metadata_t *main_meta = NULL;
	uint64_t max_counter_val = 0;

	list_foreach(*list, link, struct dev_list_member, iter) {
		hr_metadata_t *iter_meta = (hr_metadata_t *)iter->md;
		if (iter_meta->counter >= max_counter_val) {
			max_counter_val = iter_meta->counter;
			main_meta = iter_meta;
		}
	}

	assert(main_meta != NULL);

	vol->data_blkno = main_meta->data_blkno;
	vol->truncated_blkno = main_meta->truncated_blkno;
	vol->data_offset = main_meta->data_offset;
	vol->extent_no = main_meta->extent_no;
	/* vol->level = main_meta->level; */ /* already set */
	vol->layout = main_meta->layout;
	vol->strip_size = main_meta->strip_size;
	vol->bsize = main_meta->bsize;
	/* already set */
	/* memcpy(vol->devname, main_meta->devname, HR_DEVNAME_LEN); */
	memcpy(vol->in_mem_md, main_meta, sizeof(hr_metadata_t));

	list_foreach(*list, link, struct dev_list_member, iter) {
		hr_metadata_t *iter_meta = (hr_metadata_t *)iter->md;

		vol->extents[iter_meta->index].svc_id = iter->svc_id;
		iter->fini = false;

		if (iter_meta->counter == max_counter_val)
			vol->extents[iter_meta->index].state = HR_EXT_ONLINE;
		else
			vol->extents[iter_meta->index].state = HR_EXT_INVALID;
	}

	for (size_t i = 0; i < vol->extent_no; i++) {
		if (vol->extents[i].state == HR_EXT_NONE)
			vol->extents[i].state = HR_EXT_MISSING;
	}

	return EOK;
}

static void meta_native_encode(void *md_v, void *block)
{
	HR_DEBUG("%s()", __func__);

	const hr_metadata_t *metadata = md_v;

	/*
	 * Use scratch metadata for easier encoding without the need
	 * for manualy specifying offsets.
	 */
	hr_metadata_t scratch_md;

	memcpy(scratch_md.magic, metadata->magic, HR_NATIVE_MAGIC_SIZE);
	memcpy(scratch_md.uuid, metadata->uuid, HR_NATIVE_UUID_LEN);
	/* uuid_decode((uint8_t *)scratch_md.uuid, (uuid_t *)metadata->uuid); */

	scratch_md.data_blkno = host2uint64_t_le(metadata->data_blkno);
	scratch_md.truncated_blkno = host2uint64_t_le(
	    metadata->truncated_blkno);
	scratch_md.data_offset = host2uint64_t_le(metadata->data_offset);
	scratch_md.counter = host2uint64_t_le(metadata->counter);
	scratch_md.version = host2uint32_t_le(metadata->version);
	scratch_md.extent_no = host2uint32_t_le(metadata->extent_no);
	scratch_md.index = host2uint32_t_le(metadata->index);
	scratch_md.level = host2uint32_t_le(metadata->level);
	scratch_md.layout = host2uint32_t_le(metadata->layout);
	scratch_md.strip_size = host2uint32_t_le(metadata->strip_size);
	scratch_md.bsize = host2uint32_t_le(metadata->bsize);
	memcpy(scratch_md.devname, metadata->devname, HR_DEVNAME_LEN);

	memcpy(block, &scratch_md, sizeof(hr_metadata_t));
}

static errno_t meta_native_decode(const void *block, void *md_v)
{
	HR_DEBUG("%s()", __func__);

	hr_metadata_t *metadata = md_v;

	/*
	 * Use scratch metadata for easier decoding without the need
	 * for manualy specifying offsets.
	 */
	hr_metadata_t scratch_md;
	memcpy(&scratch_md, block, sizeof(hr_metadata_t));

	memcpy(metadata->magic, scratch_md.magic, HR_NATIVE_MAGIC_SIZE);
	memcpy(metadata->uuid, scratch_md.uuid, HR_NATIVE_UUID_LEN);
	/* uuid_decode((uint8_t *)scratch_md.uuid, (uuid_t *)metadata->uuid); */

	metadata->data_blkno = uint64_t_le2host(scratch_md.data_blkno);
	metadata->truncated_blkno = uint64_t_le2host(
	    scratch_md.truncated_blkno);
	metadata->data_offset = uint64_t_le2host(scratch_md.data_offset);
	metadata->counter = uint64_t_le2host(scratch_md.counter);
	metadata->version = uint32_t_le2host(scratch_md.version);
	metadata->extent_no = uint32_t_le2host(scratch_md.extent_no);
	metadata->index = uint32_t_le2host(scratch_md.index);
	metadata->level = uint32_t_le2host(scratch_md.level);
	metadata->layout = uint32_t_le2host(scratch_md.layout);
	metadata->strip_size = uint32_t_le2host(scratch_md.strip_size);
	metadata->bsize = uint32_t_le2host(scratch_md.bsize);
	memcpy(metadata->devname, scratch_md.devname, HR_DEVNAME_LEN);

	return EOK;
}

static errno_t meta_native_get_block(service_id_t dev, void **rblock)
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

	if (bsize < sizeof(hr_metadata_t))
		return EINVAL;

	rc = block_get_nblocks(dev, &blkno);
	if (rc != EOK)
		return rc;

	if (blkno < HR_NATIVE_META_SIZE)
		return EINVAL;

	block = malloc(bsize);
	if (block == NULL)
		return ENOMEM;

	rc = block_read_direct(dev, blkno - 1, HR_NATIVE_META_SIZE, block);
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

static errno_t meta_native_write_block(service_id_t dev, const void *block)
{
	HR_DEBUG("%s()", __func__);

	errno_t rc;
	uint64_t blkno;
	size_t bsize;

	rc = block_get_bsize(dev, &bsize);
	if (rc != EOK)
		return rc;

	if (bsize < sizeof(hr_metadata_t))
		return EINVAL;

	rc = block_get_nblocks(dev, &blkno);
	if (rc != EOK)
		return rc;

	if (blkno < HR_NATIVE_META_SIZE)
		return EINVAL;

	rc = block_write_direct(dev, blkno - 1, HR_NATIVE_META_SIZE, block);

	return rc;
}

static bool meta_native_has_valid_magic(const void *md_v)
{
	HR_DEBUG("%s()", __func__);

	const hr_metadata_t *md = md_v;

	if (str_lcmp(md->magic, HR_NATIVE_MAGIC_STR, HR_NATIVE_MAGIC_SIZE) != 0)
		return false;

	return true;
}

static bool meta_native_compare_uuids(const void *m1p, const void *m2p)
{
	const hr_metadata_t *m1 = m1p;
	const hr_metadata_t *m2 = m2p;
	if (memcmp(m1->uuid, m2->uuid, HR_NATIVE_UUID_LEN) == 0)
		return true;

	return false;
}

static void meta_native_inc_counter(void *md_v)
{
	hr_metadata_t *md = md_v;

	md->counter++;
}

/*
 * XXX: finish this fcn documentation
 *
 * Returns ENOMEM else EOK
 */
static errno_t meta_native_save(hr_volume_t *vol, bool with_state_callback)
{
	HR_DEBUG("%s()", __func__);

	errno_t rc = EOK;

	void *md_block = calloc(1, vol->bsize);
	if (md_block == NULL)
		return ENOMEM;

	hr_metadata_t *md = (hr_metadata_t *)vol->in_mem_md;

	fibril_rwlock_read_lock(&vol->extents_lock);

	fibril_mutex_lock(&vol->md_lock);

	for (size_t i = 0; i < vol->extent_no; i++) {
		hr_extent_t *ext = &vol->extents[i];

		fibril_rwlock_read_lock(&vol->states_lock);

		/* TODO: special case for REBUILD */
		if (ext->state != HR_EXT_ONLINE)
			continue;

		fibril_rwlock_read_unlock(&vol->states_lock);

		md->index = i;
		meta_native_encode(md, md_block);
		rc = meta_native_write_block(ext->svc_id, md_block);
		if (with_state_callback && rc != EOK)
			vol->state_callback(vol, i, rc);
	}

	fibril_mutex_unlock(&vol->md_lock);

	fibril_rwlock_read_unlock(&vol->extents_lock);

	if (with_state_callback)
		vol->hr_ops.state_event(vol);

	free(md_block);
	return EOK;
}

static const char *meta_native_get_devname(const void *md_v)
{
	const hr_metadata_t *md = md_v;

	return md->devname;
}

static hr_level_t meta_native_get_level(const void *md_v)
{
	const hr_metadata_t *md = md_v;

	return md->level;
}

static uint64_t meta_native_get_data_offset(void)
{
	return HR_NATIVE_DATA_OFF;
}

static size_t meta_native_get_size(void)
{
	return HR_NATIVE_META_SIZE;
}

static uint8_t meta_native_get_flags(void)
{
	uint8_t flags = 0;

	flags |= HR_METADATA_HOTSPARE_SUPPORT;

	return flags;
}

static hr_metadata_type_t meta_native_get_type(void)
{
	return HR_METADATA_NATIVE;
}

static void meta_native_dump(const void *md_v)
{
	HR_DEBUG("%s()", __func__);

	const hr_metadata_t *metadata = md_v;

	printf("\tmagic: %s\n", metadata->magic);
	printf("\tUUID: ");
	for (size_t i = 0; i < HR_NATIVE_UUID_LEN; ++i) {
		printf("%.2X", metadata->uuid[i]);
		if (i + 1 < HR_NATIVE_UUID_LEN)
			printf(" ");
	}
	printf("\n");
	printf("\tdata_blkno: %" PRIu64 "\n", metadata->data_blkno);
	printf("\ttruncated_blkno: %" PRIu64 "\n", metadata->truncated_blkno);
	printf("\tdata_offset: %" PRIu64 "\n", metadata->data_offset);
	printf("\tcounter: %" PRIu64 "\n", metadata->counter);
	printf("\tversion: %" PRIu32 "\n", metadata->version);
	printf("\textent_no: %" PRIu32 "\n", metadata->extent_no);
	printf("\tindex: %" PRIu32 "\n", metadata->index);
	printf("\tlevel: %" PRIu32 "\n", metadata->level);
	printf("\tlayout: %" PRIu32 "\n", metadata->layout);
	printf("\tstrip_size: %" PRIu32 "\n", metadata->strip_size);
	printf("\tbsize: %" PRIu32 "\n", metadata->bsize);
	printf("\tdevname: %s\n", metadata->devname);
}

/** @}
 */
