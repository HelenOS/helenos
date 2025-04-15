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
#include <inttypes.h>
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

errno_t hr_metadata_init(hr_volume_t *vol, hr_metadata_t *md)
{
	HR_DEBUG("%s()", __func__);

	str_cpy(md->magic, HR_MAGIC_SIZE, HR_MAGIC_STR);

	md->version = HR_METADATA_VERSION;
	vol->metadata_version = md->version;

	md->counter = 0;

	uuid_t uuid;
	/* rndgen */
	fibril_usleep(1000);
	errno_t rc = uuid_generate(&uuid);
	if (rc != EOK)
		return rc;

	/* XXX: for now we just copy byte by byte as "encoding" */
	memcpy(md->uuid, &uuid, HR_UUID_LEN);
	/* uuid_encode(&uuid, metadata->uuid); */

	md->nblocks = vol->nblocks;
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

/*
 * XXX: finish this fcn documentation
 *
 * Returns ENOMEM else EOK
 */
errno_t hr_metadata_save(hr_volume_t *vol, bool with_state_callback)
{
	HR_DEBUG("%s()", __func__);

	errno_t rc = EOK;

	void *md_block = calloc(1, vol->bsize);
	if (md_block == NULL)
		return ENOMEM;

	fibril_rwlock_read_lock(&vol->extents_lock);

	fibril_mutex_lock(&vol->md_lock);

	for (size_t i = 0; i < vol->extent_no; i++) {
		hr_extent_t *ext = &vol->extents[i];

		fibril_rwlock_read_lock(&vol->states_lock);

		/* TODO: special case for REBUILD */
		if (ext->status != HR_EXT_ONLINE)
			continue;

		fibril_rwlock_read_unlock(&vol->states_lock);

		vol->in_mem_md->index = i;
		hr_encode_metadata_to_block(vol->in_mem_md, md_block);
		rc = hr_write_metadata_block(ext->svc_id, md_block);
		if (with_state_callback && rc != EOK)
			vol->state_callback(vol, i, rc);
	}

	fibril_mutex_unlock(&vol->md_lock);

	fibril_rwlock_read_unlock(&vol->extents_lock);

	if (with_state_callback)
		vol->hr_ops.status_event(vol);

	free(md_block);
	return EOK;
}

bool hr_valid_md_magic(hr_metadata_t *md)
{
	HR_DEBUG("%s()", __func__);

	if (str_lcmp(md->magic, HR_MAGIC_STR, HR_MAGIC_SIZE) != 0)
		return false;

	return true;
}

errno_t hr_write_metadata_block(service_id_t dev, const void *block)
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

	if (blkno < HR_META_SIZE)
		return EINVAL;

	rc = block_write_direct(dev, blkno - 1, HR_META_SIZE, block);

	return rc;
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

	if (blkno < HR_META_SIZE)
		return EINVAL;

	block = malloc(bsize);
	if (block == NULL)
		return ENOMEM;

	rc = block_read_direct(dev, blkno - 1, HR_META_SIZE, block);
	/*
	 * XXX: here maybe call vol status event or the state callback...
	 *
	 * but need to pass vol pointer
	 */
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

void hr_encode_metadata_to_block(hr_metadata_t *metadata, void *block)
{
	HR_DEBUG("%s()", __func__);

	/*
	 * Use scratch metadata for easier encoding without the need
	 * for manualy specifying offsets.
	 */
	hr_metadata_t scratch_md;

	memcpy(scratch_md.magic, metadata->magic, HR_MAGIC_SIZE);
	memcpy(scratch_md.uuid, metadata->uuid, HR_UUID_LEN);
	/* uuid_decode((uint8_t *)scratch_md.uuid, (uuid_t *)metadata->uuid); */

	scratch_md.nblocks = host2uint64_t_le(metadata->nblocks);
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

void hr_decode_metadata_from_block(const void *block, hr_metadata_t *metadata)
{
	HR_DEBUG("%s()", __func__);

	/*
	 * Use scratch metadata for easier decoding without the need
	 * for manualy specifying offsets.
	 */
	hr_metadata_t scratch_md;
	memcpy(&scratch_md, block, sizeof(hr_metadata_t));

	memcpy(metadata->magic, scratch_md.magic, HR_MAGIC_SIZE);
	memcpy(metadata->uuid, scratch_md.uuid, HR_UUID_LEN);
	/* uuid_decode((uint8_t *)scratch_md.uuid, (uuid_t *)metadata->uuid); */

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
	metadata->bsize = uint32_t_le2host(scratch_md.bsize);
	memcpy(metadata->devname, scratch_md.devname, HR_DEVNAME_LEN);
}

void hr_metadata_dump(hr_metadata_t *metadata)
{
	HR_DEBUG("%s()", __func__);

	printf("\tmagic: %s\n", metadata->magic);
	printf("\tUUID: ");
	for (size_t i = 0; i < HR_UUID_LEN; ++i) {
		printf("%.2X", metadata->uuid[i]);
		if (i + 1 < HR_UUID_LEN)
			printf(" ");
	}
	printf("\n");
	printf("\tnblocks: %" PRIu64 "\n", metadata->nblocks);
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
