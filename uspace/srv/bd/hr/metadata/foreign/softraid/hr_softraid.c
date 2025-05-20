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

#include "softraidvar.h"

static void *meta_softraid_alloc_struct(void);
static errno_t meta_softraid_init_vol2meta(const hr_volume_t *, void *);
static errno_t meta_softraid_init_meta2vol(const list_t *, hr_volume_t *);
static void meta_softraid_encode(void *, void *);
static errno_t meta_softraid_decode(const void *, void *);
static errno_t meta_softraid_get_block(service_id_t, void **);
static errno_t meta_softraid_write_block(service_id_t, const void *);
static bool meta_softraid_has_valid_magic(const void *);
static bool meta_softraid_compare_uuids(const void *, const void *);
static void meta_softraid_inc_counter(void *);
static errno_t meta_softraid_save(hr_volume_t *, bool);
static const char *meta_softraid_get_devname(const void *);
static hr_level_t meta_softraid_get_level(const void *);
static uint64_t meta_softraid_get_data_offset(void);
static size_t meta_softraid_get_size(void);
static uint8_t meta_softraid_get_flags(void);
static hr_metadata_type_t meta_softraid_get_type(void);
static void meta_softraid_dump(const void *);

hr_superblock_ops_t metadata_softraid_ops = {
	.alloc_struct = meta_softraid_alloc_struct,
	.init_vol2meta = meta_softraid_init_vol2meta,
	.init_meta2vol = meta_softraid_init_meta2vol,
	.encode = meta_softraid_encode,
	.decode = meta_softraid_decode,
	.get_block = meta_softraid_get_block,
	.write_block = meta_softraid_write_block,
	.has_valid_magic = meta_softraid_has_valid_magic,
	.compare_uuids = meta_softraid_compare_uuids,
	.inc_counter = meta_softraid_inc_counter,
	.save = meta_softraid_save,
	.get_devname = meta_softraid_get_devname,
	.get_level = meta_softraid_get_level,
	.get_data_offset = meta_softraid_get_data_offset,
	.get_size = meta_softraid_get_size,
	.get_flags = meta_softraid_get_flags,
	.get_type = meta_softraid_get_type,
	.dump = meta_softraid_dump
};

static void *meta_softraid_alloc_struct(void)
{
	return calloc(1, SR_META_SIZE * DEV_BSIZE);
}

static errno_t meta_softraid_init_vol2meta(const hr_volume_t *vol, void *md_v)
{
	(void)vol;
	(void)md_v;
	return ENOTSUP;
}

static errno_t meta_softraid_init_meta2vol(const list_t *list, hr_volume_t *vol)
{
	HR_DEBUG("%s()", __func__);

	errno_t rc = EOK;

	struct sr_metadata *main_meta = NULL;
	uint64_t max_counter_val = 0;

	list_foreach(*list, link, struct dev_list_member, iter) {
		struct sr_metadata *iter_meta = iter->md;

		if (iter_meta->ssd_ondisk >= max_counter_val) {
			max_counter_val = iter_meta->ssd_ondisk;
			main_meta = iter_meta;
		}
	}

	assert(main_meta != NULL);

	vol->bsize = main_meta->ssdi.ssd_secsize;

	vol->data_blkno = main_meta->ssdi.ssd_size;

	/* get coerced size from some (first) chunk metadata */
	struct sr_meta_chunk *mc = (struct sr_meta_chunk *)(main_meta + 1);
	vol->truncated_blkno = mc->scmi.scm_coerced_size;

	vol->data_offset = main_meta->ssd_data_blkno;

	if (main_meta->ssdi.ssd_chunk_no > HR_MAX_EXTENTS) {
		HR_DEBUG("Assembled volume has %u extents (max = %u)",
		    (unsigned)main_meta->ssdi.ssd_chunk_no,
		    HR_MAX_EXTENTS);
		rc = EINVAL;
		goto error;
	}

	vol->extent_no = main_meta->ssdi.ssd_chunk_no;

	if (main_meta->ssdi.ssd_level == 5)
		vol->layout = HR_RLQ_RAID5_NR;
	else
		vol->layout = HR_RLQ_NONE;

	vol->strip_size = main_meta->ssdi.ssd_strip_size;

	memcpy(vol->in_mem_md, main_meta, SR_META_SIZE * DEV_BSIZE);

	list_foreach(*list, link, struct dev_list_member, iter) {
		struct sr_metadata *iter_meta = iter->md;

		uint8_t index = iter_meta->ssdi.ssd_chunk_id;

		vol->extents[index].svc_id = iter->svc_id;
		iter->fini = false;

		/* for now no ssd_rebuild handling for saved REBUILD */
		if (iter_meta->ssd_ondisk == max_counter_val)
			vol->extents[index].state = HR_EXT_ONLINE;
		else
			vol->extents[index].state = HR_EXT_INVALID;
	}

	for (size_t i = 0; i < vol->extent_no; i++) {
		if (vol->extents[i].state == HR_EXT_NONE)
			vol->extents[i].state = HR_EXT_MISSING;
	}

error:
	return rc;
}

static void meta_softraid_encode(void *md_v, void *block)
{
	HR_DEBUG("%s()", __func__);

	(void)md_v;
	(void)block;
}

static errno_t meta_softraid_decode(const void *block, void *md_v)
{
	HR_DEBUG("%s()", __func__);

	errno_t rc = EOK;
	struct sr_metadata *md = md_v;
	uint8_t md5_hash[16];

	struct sr_metadata *scratch_md = meta_softraid_alloc_struct();
	if (scratch_md == NULL)
		return ENOMEM;

	memcpy(scratch_md, block, meta_softraid_get_size() * 512);

	md->ssdi.ssd_magic =
	    uint64_t_le2host(scratch_md->ssdi.ssd_magic);
	if (md->ssdi.ssd_magic != SR_MAGIC) {
		rc = EINVAL;
		goto error;
	}
	md->ssdi.ssd_version = uint32_t_le2host(scratch_md->ssdi.ssd_version);
	if (md->ssdi.ssd_version != SR_META_VERSION) {
		HR_DEBUG("unsupported metadata version\n");
		rc = EINVAL;
		goto error;
	}
	md->ssdi.ssd_vol_flags =
	    uint32_t_le2host(scratch_md->ssdi.ssd_vol_flags);
	memcpy(&md->ssdi.ssd_uuid, &scratch_md->ssdi.ssd_uuid, SR_UUID_MAX);

	md->ssdi.ssd_chunk_no =
	    uint32_t_le2host(scratch_md->ssdi.ssd_chunk_no);
	md->ssdi.ssd_chunk_id =
	    uint32_t_le2host(scratch_md->ssdi.ssd_chunk_id);

	md->ssdi.ssd_opt_no = uint32_t_le2host(scratch_md->ssdi.ssd_opt_no);
	if (md->ssdi.ssd_opt_no > 0) {
		HR_DEBUG("unsupported optional metadata\n");
		rc = EINVAL;
		goto error;
	}
	md->ssdi.ssd_secsize = uint32_t_le2host(scratch_md->ssdi.ssd_secsize);
	if (md->ssdi.ssd_secsize != DEV_BSIZE) {
		HR_DEBUG("unsupported sector size\n");
		rc = EINVAL;
		goto error;
	}

	md->ssdi.ssd_volid = uint32_t_le2host(scratch_md->ssdi.ssd_volid);
	md->ssdi.ssd_level = uint32_t_le2host(scratch_md->ssdi.ssd_level);
	md->ssdi.ssd_size = int64_t_le2host(scratch_md->ssdi.ssd_size);
	memcpy(md->ssdi.ssd_vendor, scratch_md->ssdi.ssd_vendor, 8);
	memcpy(md->ssdi.ssd_product, scratch_md->ssdi.ssd_product, 16);
	memcpy(md->ssdi.ssd_revision, scratch_md->ssdi.ssd_revision, 4);
	md->ssdi.ssd_strip_size =
	    uint32_t_le2host(scratch_md->ssdi.ssd_strip_size);

	rc = create_hash((const uint8_t *)&scratch_md->ssdi,
	    sizeof(struct sr_meta_invariant), md5_hash, HASH_MD5);
	assert(rc == EOK);
	if (memcmp(md5_hash, scratch_md->ssd_checksum, 16) != 0) {
		HR_DEBUG("ssd_checksum invalid\n");
		rc = EINVAL;
		goto error;
	}

	memcpy(md->ssd_checksum, scratch_md->ssd_checksum, MD5_DIGEST_LENGTH);

	memcpy(md->ssd_devname, scratch_md->ssd_devname, 32);
	md->ssd_meta_flags = uint32_t_le2host(scratch_md->ssd_meta_flags);
	md->ssd_data_blkno = uint32_t_le2host(scratch_md->ssd_data_blkno);
	md->ssd_ondisk = uint64_t_le2host(scratch_md->ssd_ondisk);
	md->ssd_rebuild = int64_t_le2host(scratch_md->ssd_rebuild);

	struct sr_meta_chunk *scratch_mc =
	    (struct sr_meta_chunk *)(scratch_md + 1);
	struct sr_meta_chunk *mc = (struct sr_meta_chunk *)(md + 1);
	for (size_t i = 0; i < md->ssdi.ssd_chunk_no; i++, mc++, scratch_mc++) {
		mc->scmi.scm_volid =
		    uint32_t_le2host(scratch_mc->scmi.scm_volid);
		mc->scmi.scm_chunk_id =
		    uint32_t_le2host(scratch_mc->scmi.scm_chunk_id);
		memcpy(mc->scmi.scm_devname, scratch_mc->scmi.scm_devname, 32);
		mc->scmi.scm_size = int64_t_le2host(scratch_mc->scmi.scm_size);
		mc->scmi.scm_coerced_size =
		    int64_t_le2host(scratch_mc->scmi.scm_coerced_size);
		memcpy(&mc->scmi.scm_uuid, &scratch_mc->scmi.scm_uuid,
		    SR_UUID_MAX);

		memcpy(mc->scm_checksum, scratch_mc->scm_checksum,
		    MD5_DIGEST_LENGTH);
		mc->scm_status = uint32_t_le2host(scratch_mc->scm_status);

		/*
		 * This commented piece of code found a bug in
		 * OpenBSD softraid chunk metadata initialization,
		 * fix has been proposed [1], if it is fixed, feel free
		 * to uncomment, although it will work only on new
		 * volumes.
		 *
		 * [1]: https://marc.info/?l=openbsd-tech&m=174535579711235&w=2
		 */
		/*
		 * rc = create_hash((const uint8_t *)&scratch_mc->scmi,
		 *     sizeof(struct sr_meta_chunk_invariant), md5_hash, HASH_MD5);
		 * assert(rc == EOK);
		 * if (memcmp(md5_hash, mc->scm_checksum, 16) != 0) {
		 * 	HR_DEBUG("chunk %zu, scm_checksum invalid\n", i);
		 * 	rc = EINVAL;
		 * 	goto error;
		 * }
		 */
	}

	struct sr_meta_opt_hdr *scratch_om =
	    (struct sr_meta_opt_hdr *)((u_int8_t *)(scratch_md + 1) +
	    sizeof(struct sr_meta_chunk) * md->ssdi.ssd_chunk_no);
	struct sr_meta_opt_hdr *om =
	    (struct sr_meta_opt_hdr *)((u_int8_t *)(md + 1) +
	    sizeof(struct sr_meta_chunk) * md->ssdi.ssd_chunk_no);
	for (size_t i = 0; i < md->ssdi.ssd_opt_no; i++) {
		om->som_type = uint32_t_le2host(scratch_om->som_type);
		om->som_length = uint32_t_le2host(scratch_om->som_length);
		memcpy(om->som_checksum, scratch_om->som_checksum,
		    MD5_DIGEST_LENGTH);

		/*
		 * No need to do checksum, we don't support optional headers.
		 * Despite this, still load it the headers.
		 */

		om = (struct sr_meta_opt_hdr *)((void *)om +
		    om->som_length);
		scratch_om = (struct sr_meta_opt_hdr *)((void *)scratch_om +
		    om->som_length);
	}

error:
	free(scratch_md);

	return rc;
}

static errno_t meta_softraid_get_block(service_id_t dev, void **rblock)
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

	if (bsize != DEV_BSIZE)
		return EINVAL;

	rc = block_get_nblocks(dev, &blkno);
	if (rc != EOK)
		return rc;

	if (blkno < SR_META_OFFSET + SR_META_SIZE)
		return EINVAL;

	block = malloc(bsize * SR_META_SIZE);
	if (block == NULL)
		return ENOMEM;

	rc = block_read_direct(dev, SR_META_OFFSET, SR_META_SIZE, block);
	if (rc != EOK) {
		free(block);
		return rc;
	}

	*rblock = block;
	return EOK;
}

static errno_t meta_softraid_write_block(service_id_t dev, const void *block)
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

	if (blkno < SR_META_OFFSET + SR_META_SIZE)
		return EINVAL;

	rc = block_write_direct(dev, SR_META_OFFSET, SR_META_SIZE, block);

	return rc;
}

static bool meta_softraid_has_valid_magic(const void *md_v)
{
	HR_DEBUG("%s()", __func__);

	const struct sr_metadata *md = md_v;

	if (md->ssdi.ssd_magic != SR_MAGIC)
		return false;

	return true;
}

static bool meta_softraid_compare_uuids(const void *m1_v, const void *m2_v)
{
	const struct sr_metadata *m1 = m1_v;
	const struct sr_metadata *m2 = m2_v;
	if (memcmp(&m1->ssdi.ssd_uuid, &m2->ssdi.ssd_uuid,
	    SR_UUID_MAX) == 0)
		return true;

	return false;
}

static void meta_softraid_inc_counter(void *md_v)
{
	struct sr_metadata *md = md_v;

	md->ssd_ondisk++;
}

static errno_t meta_softraid_save(hr_volume_t *vol, bool with_state_callback)
{
	HR_DEBUG("%s()", __func__);

	/* silent */
	return EOK;
}

static const char *meta_softraid_get_devname(const void *md_v)
{
	const struct sr_metadata *md = md_v;

	return md->ssd_devname;
}

static hr_level_t meta_softraid_get_level(const void *md_v)
{
	const struct sr_metadata *md = md_v;

	switch (md->ssdi.ssd_level) {
	case 0:
		return HR_LVL_0;
	case 1:
		return HR_LVL_1;
	case 5:
		return HR_LVL_5;
	default:
		return HR_LVL_UNKNOWN;
	}
}

static uint64_t meta_softraid_get_data_offset(void)
{
	return SR_DATA_OFFSET;
}

static size_t meta_softraid_get_size(void)
{
	return SR_META_SIZE;
}

static uint8_t meta_softraid_get_flags(void)
{
	uint8_t flags = 0;

	return flags;
}

static hr_metadata_type_t meta_softraid_get_type(void)
{
	return HR_METADATA_SOFTRAID;
}

static void meta_softraid_dump(const void *md_v)
{
	HR_DEBUG("%s()", __func__);

	const struct sr_metadata *md = md_v;

	sr_meta_print(md);
}

/** @}
 */
