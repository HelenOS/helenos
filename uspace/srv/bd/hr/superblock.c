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

#include <block.h>
#include <byteorder.h>
#include <errno.h>
#include <io/log.h>
#include <loc.h>
#include <mem.h>
#include <uuid.h>
#include <stdlib.h>
#include <stdio.h>
#include <str.h>
#include <types/uuid.h>

#include "superblock.h"
#include "util.h"
#include "var.h"

static errno_t read_metadata(service_id_t, hr_metadata_t *);
static errno_t hr_fill_meta_from_vol(hr_volume_t *, hr_metadata_t *);

errno_t hr_write_meta_to_vol(hr_volume_t *vol)
{
	HR_DEBUG("%s()", __func__);

	errno_t rc;
	size_t i;
	hr_metadata_t *metadata;
	uuid_t uuid;

	metadata = calloc(1, HR_META_SIZE * vol->bsize);
	if (metadata == NULL)
		return ENOMEM;

	rc = hr_fill_meta_from_vol(vol, metadata);
	if (rc != EOK)
		goto error;

	/* rndgen */
	fibril_usleep(1000);
	rc = uuid_generate(&uuid);
	if (rc != EOK)
		goto error;

	/* XXX: for now we just copy byte by byte as "encoding" */
	memcpy(metadata->uuid, &uuid, sizeof(HR_UUID_LEN));
	/* uuid_encode(&uuid, metadata->uuid); */

	for (i = 0; i < vol->extent_no; i++) {
		metadata->index = host2uint32_t_le(i);

		rc = block_write_direct(vol->extents[i].svc_id, HR_META_OFF,
		    HR_META_SIZE, metadata);
		if (rc != EOK)
			goto error;

	}

	memcpy(vol->in_mem_md, metadata, sizeof(hr_metadata_t));
error:
	free(metadata);
	return rc;
}

errno_t hr_write_meta_to_ext(hr_volume_t *vol, size_t ext)
{
	HR_DEBUG("%s()", __func__);

	errno_t rc;
	hr_metadata_t *metadata;
	uuid_t uuid;

	/* XXX: use scratchpad */
	metadata = calloc(1, HR_META_SIZE * vol->bsize);
	if (metadata == NULL)
		return ENOMEM;

	rc = hr_fill_meta_from_vol(vol, metadata);
	if (rc != EOK)
		goto error;

	metadata->index = host2uint32_t_le(ext);

	rc = uuid_generate(&uuid);
	if (rc != EOK)
		goto error;
	uuid_encode(&uuid, metadata->uuid);

	rc = block_write_direct(vol->extents[ext].svc_id, HR_META_OFF,
	    HR_META_SIZE, metadata);
	if (rc != EOK)
		goto error;

error:
	free(metadata);
	return rc;
}

/*
 * Fill metadata members from
 * specified volume.
 *
 * Does not fill extent index and UUID.
 */
static errno_t hr_fill_meta_from_vol(hr_volume_t *vol, hr_metadata_t *metadata)
{
	HR_DEBUG("%s()", __func__);

	size_t meta_blkno = HR_META_OFF + HR_META_SIZE;

	if (vol->level != HR_LVL_1)
		meta_blkno *= vol->extent_no;

	if (vol->nblocks < meta_blkno) {
		HR_ERROR("hr_fill_meta_from_vol(): volume \"%s\" does not "
		    " have enough space to store metada, aborting\n",
		    vol->devname);
		return EINVAL;
	} else if (vol->nblocks == meta_blkno) {
		HR_ERROR("hr_fill_meta_from_vol(): volume \"%s\" would have "
		    "zero data blocks after writing metadata, aborting\n",
		    vol->devname);
		return EINVAL;
	}

	/* XXX: use scratchpad */
	str_cpy(metadata->magic, HR_MAGIC_SIZE, HR_MAGIC_STR);
	metadata->nblocks = host2uint64_t_le(vol->nblocks);
	metadata->data_blkno = host2uint64_t_le(vol->data_blkno);
	metadata->truncated_blkno = host2uint64_t_le(vol->truncated_blkno);
	metadata->data_offset = host2uint64_t_le(vol->data_offset);
	metadata->counter = host2uint64_t_le(~(0UL)); /* XXX: unused */
	metadata->version = host2uint32_t_le(~(0U)); /* XXX: unused */
	metadata->extent_no = host2uint32_t_le(vol->extent_no);
	/* index filled separately for each extent */
	metadata->level = host2uint32_t_le(vol->level);
	metadata->layout = host2uint32_t_le(vol->layout);
	metadata->strip_size = host2uint32_t_le(vol->strip_size);
	metadata->bsize = host2uint32_t_le(vol->bsize);
	str_cpy(metadata->devname, HR_DEVNAME_LEN, vol->devname);

	return EOK;
}

bool hr_valid_md_magic(hr_metadata_t *md)
{
	if (str_lcmp(md->magic, HR_MAGIC_STR, HR_MAGIC_SIZE) != 0)
		return false;

	return true;
}

errno_t hr_fill_vol_from_meta(hr_volume_t *vol)
{
	HR_DEBUG("%s()", __func__);

	errno_t rc;
	hr_metadata_t *metadata;

	metadata = calloc(1, HR_META_SIZE * vol->bsize);
	if (metadata == NULL)
		return ENOMEM;

	service_id_t assembly_svc_id_order[HR_MAX_EXTENTS] = { 0 };
	for (size_t i = 0; i < vol->extent_no; i++)
		assembly_svc_id_order[i] = vol->extents[i].svc_id;

	size_t md_order_indices[HR_MAX_EXTENTS] = { 0 };
	for (size_t i = 0; i < vol->extent_no; i++) {
		if (assembly_svc_id_order[i] == 0) {
			/* set invalid index */
			md_order_indices[i] = vol->extent_no;
			continue;
		}
		rc = read_metadata(assembly_svc_id_order[i], metadata);
		if (rc != EOK)
			goto end;
		if (!hr_valid_md_magic(metadata))
			goto end;
		md_order_indices[i] = uint32_t_le2host(metadata->index);
	}

	for (size_t i = 0; i < vol->extent_no; i++) {
		vol->extents[i].svc_id = 0;
		vol->extents[i].status = HR_EXT_MISSING;
	}

	/* sort */
	for (size_t i = 0; i < vol->extent_no; i++) {
		for (size_t j = 0; j < vol->extent_no; j++) {
			if (i == md_order_indices[j]) {
				vol->extents[i].svc_id =
				    assembly_svc_id_order[j];
				vol->extents[i].status = HR_EXT_ONLINE;
			}
		}
	}

	/*
	 * still assume metadata are in sync across extents
	 */

	if (vol->extent_no != uint32_t_le2host(metadata->extent_no)) {
		HR_ERROR("number of divices in array differ: specified %zu, "
		    "metadata states %u",
		    vol->extent_no, uint32_t_le2host(metadata->extent_no));
		rc = EINVAL;
		goto end;
	}

	/* TODO: handle version */
	vol->level = uint32_t_le2host(metadata->level);
	vol->layout = (uint8_t)uint32_t_le2host(metadata->layout);
	vol->strip_size = uint32_t_le2host(metadata->strip_size);
	vol->nblocks = uint64_t_le2host(metadata->nblocks);
	vol->data_blkno = uint64_t_le2host(metadata->data_blkno);
	vol->data_offset = uint64_t_le2host(metadata->data_offset);
	vol->bsize = uint32_t_le2host(metadata->bsize);
	vol->counter = uint64_t_le2host(0x00); /* unused */

	if (str_cmp(metadata->devname, vol->devname) != 0) {
		HR_WARN("devname on metadata (%s) and config (%s) differ, "
		    "using config", metadata->devname, vol->devname);
	}
end:
	free(metadata);
	return rc;
}

static errno_t read_metadata(service_id_t dev, hr_metadata_t *metadata)
{
	errno_t rc;
	uint64_t nblocks;

	rc = block_get_nblocks(dev, &nblocks);
	if (rc != EOK)
		return rc;

	if (nblocks < HR_META_SIZE + HR_META_OFF)
		return EINVAL;

	rc = block_read_direct(dev, HR_META_OFF, HR_META_SIZE, metadata);
	if (rc != EOK)
		return rc;

	return EOK;
}

errno_t hr_get_metadata_block(service_id_t dev, void **rblock)
{
	HR_DEBUG("%s()", __func__);
	errno_t rc;
	uint64_t blkno;
	size_t bsize;
	void *block;

	rc = block_get_bsize(dev, &bsize);
	if (rc != EOK)
		return rc;

	if (bsize < sizeof(hr_metadata_t))
		return EINVAL;

	rc = block_get_nblocks(dev, &blkno);
	if (rc != EOK)
		return rc;

	if (blkno < HR_META_OFF + HR_META_SIZE)
		return EINVAL;

	block = malloc(bsize);
	if (block == NULL)
		return ENOMEM;

	rc = block_read_direct(dev, HR_META_OFF, HR_META_SIZE, block);
	if (rc != EOK) {
		free(block);
		return rc;
	}

	if (rblock == NULL) {
		free(block);
		return EINVAL;
	}

	*rblock = block;
	return EOK;
}

void hr_decode_metadata_from_block(void *block, hr_metadata_t *metadata)
{
	/*
	 * Use scratch metadata for easier decoding without the need
	 * for manualy specifying offsets.
	 */
	hr_metadata_t scratch_md;
	memcpy(&scratch_md, block, sizeof(hr_metadata_t));

	memcpy(metadata->magic, scratch_md.magic, HR_MAGIC_SIZE);
	memcpy(metadata->uuid, scratch_md.uuid, HR_UUID_LEN);
	metadata->nblocks = uint64_t_le2host(scratch_md.nblocks);
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
	metadata->bsize = uint64_t_le2host(scratch_md.bsize);
	memcpy(metadata->devname, scratch_md.devname, HR_DEVNAME_LEN);
}

void hr_metadata_dump(hr_metadata_t *metadata)
{
	printf("\tmagic: %s\n", metadata->magic);
	printf("\tUUID: ");
	for (size_t i = 0; i < HR_UUID_LEN; ++i) {
		printf("%.2X", metadata->uuid[i]);
		if (i + 1 < HR_UUID_LEN)
			printf(" ");
	}
	printf("\n");
	printf("\tnblocks: %lu\n", metadata->nblocks);
	printf("\tdata_blkno: %lu\n", metadata->data_blkno);
	printf("\ttruncated_blkno: %lu\n", metadata->truncated_blkno);
	printf("\tdata_offset: %lu\n", metadata->data_offset);
	printf("\tcounter: %lu\n", metadata->counter);
	printf("\tversion: %u\n", metadata->version);
	printf("\textent_no: %u\n", metadata->extent_no);
	printf("\tindex: %u\n", metadata->index);
	printf("\tlevel: %u\n", metadata->level);
	printf("\tlayout: %u\n", metadata->layout);
	printf("\tstrip_size: %u\n", metadata->strip_size);
	printf("\tdevname: %s\n", metadata->devname);
}

/** @}
 */
