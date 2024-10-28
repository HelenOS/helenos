/*
 * Copyright (c) 2024 Miroslav Cimerman
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

errno_t hr_write_meta_to_vol(hr_volume_t *vol)
{
	log_msg(LOG_DEFAULT, LVL_NOTE, "hr_write_meta_to_vol()");

	errno_t rc;
	size_t i;
	size_t meta_blkno; /* blocks needed to write metadata */
	hr_metadata_t *metadata;
	uuid_t uuid;

	metadata = calloc(1, HR_META_SIZE * vol->bsize);
	if (metadata == NULL)
		return ENOMEM;

	meta_blkno = (HR_META_OFF + HR_META_SIZE);
	if (vol->level != HR_LVL_1)
		meta_blkno *= vol->dev_no;

	if (vol->nblocks < meta_blkno) {
		log_msg(LOG_DEFAULT, LVL_ERROR,
		    "not enough blocks to write metadata");
		rc = EINVAL;
		goto error;
	} else if (vol->nblocks == meta_blkno) {
		log_msg(LOG_DEFAULT, LVL_ERROR,
		    "there would be zero data blocks after writing metadata, aborting");
		rc = EINVAL;
		goto error;
	}

	metadata->magic = host2uint64_t_le(HR_MAGIC);
	metadata->extent_no = host2uint32_t_le(vol->dev_no);
	metadata->level = host2uint32_t_le(vol->level);
	metadata->nblocks = host2uint64_t_le(vol->nblocks);
	metadata->data_blkno = host2uint64_t_le(vol->data_blkno);
	metadata->data_offset = host2uint32_t_le(vol->data_offset);
	metadata->strip_size = host2uint32_t_le(vol->strip_size);
	for (i = 0; i < vol->dev_no; i++) {
		metadata->index = host2uint32_t_le(i);

		rc = uuid_generate(&uuid);
		if (rc != EOK)
			goto error;
		uuid_encode(&uuid, metadata->uuid);

		str_cpy(metadata->devname, HR_DEVNAME_LEN, vol->devname);

		rc = block_write_direct(vol->extents[i].svc_id, HR_META_OFF,
		    HR_META_SIZE, metadata);
		if (rc != EOK)
			goto error;

		/* rndgen */
		fibril_usleep(1000);
	}
error:
	free(metadata);
	return rc;
}

static errno_t validate_meta(hr_metadata_t *md)
{
	if (uint64_t_le2host(md->magic) != HR_MAGIC) {
		printf("invalid magic\n");
		return EINVAL;
	}
	return EOK;
}


errno_t hr_fill_vol_from_meta(hr_volume_t *vol)
{
	log_msg(LOG_DEFAULT, LVL_NOTE, "hr_get_vol_from_meta()");

	errno_t rc;
	hr_metadata_t *metadata;

	metadata = calloc(1, HR_META_SIZE * vol->bsize);
	if (metadata == NULL)
		return ENOMEM;

	service_id_t cfg_svc_id_order[HR_MAXDEVS] = { 0 };
	for (size_t i = 0; i < vol->dev_no; i++)
		cfg_svc_id_order[i] = vol->extents[i].svc_id;


	uint32_t md_order[HR_MAXDEVS] = { 0 };
	for (size_t i = 0; i < vol->dev_no; i++) {
		rc = read_metadata(cfg_svc_id_order[i], metadata);
		if (rc != EOK)
			goto end;
		rc = validate_meta(metadata);
		if (rc != EOK)
			goto end;
		md_order[i] = uint32_t_le2host(metadata->index);
	}

	for (size_t i = 0; i < vol->dev_no; i++)
		for (size_t j = 0; j < vol->dev_no; j++)
			if (i == md_order[j])
				vol->extents[i].svc_id = cfg_svc_id_order[j];

	/*
	 * still assume metadata are in sync across extents
	 */

	if (vol->dev_no != uint32_t_le2host(metadata->extent_no)) {
		log_msg(LOG_DEFAULT, LVL_ERROR,
		    "number of divices in array differ: specified %zu, metadata states %u",
		    vol->dev_no, uint32_t_le2host(metadata->extent_no));
		rc = EINVAL;
		goto end;
	}

	vol->level = uint32_t_le2host(metadata->level);
	vol->nblocks = uint64_t_le2host(metadata->nblocks);
	vol->data_blkno = uint64_t_le2host(metadata->data_blkno);
	vol->data_offset = uint32_t_le2host(metadata->data_offset);
	vol->strip_size = uint32_t_le2host(metadata->strip_size);

	if (str_cmp(metadata->devname, vol->devname) != 0) {
		log_msg(LOG_DEFAULT, LVL_NOTE,
		    "devname on metadata (%s) and config (%s) differ, using config",
		    metadata->devname, vol->devname);
	}
end:
	free(metadata);
	return rc;
}

static errno_t read_metadata(service_id_t dev, hr_metadata_t *metadata)
{
	errno_t rc;
	size_t bsize;
	uint64_t nblocks;

	rc = block_get_bsize(dev, &bsize);
	if (rc != EOK)
		return rc;

	rc = block_get_nblocks(dev, &nblocks);
	if (rc != EOK)
		return rc;

	if (nblocks < (HR_META_SIZE + HR_META_OFF) * bsize)
		return EINVAL;

	rc = block_read_direct(dev, HR_META_OFF, HR_META_SIZE, metadata);
	if (rc != EOK)
		return rc;

	return EOK;
}

/** @}
 */
