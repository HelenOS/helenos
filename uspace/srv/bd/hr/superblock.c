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
	size_t i, data_offset;
	uint64_t data_blkno;
	hr_metadata_t *metadata;
	uuid_t uuid;

	metadata = calloc(1, HR_META_SIZE * vol->bsize);
	if (metadata == NULL)
		return ENOMEM;

	if (vol->nblocks <= HR_META_OFF + HR_META_SIZE) {
		log_msg(LOG_DEFAULT, LVL_ERROR,
		    "not enough blocks");
		rc = EINVAL;
		goto error;
	}

	data_offset = HR_META_OFF + HR_META_SIZE;
	if (vol->level == hr_l_1) {
		data_blkno = vol->nblocks - data_offset;
	} else if (vol->level == hr_l_0) {
		data_blkno = vol->nblocks - (data_offset * vol->dev_no);
	} else {
		log_msg(LOG_DEFAULT, LVL_ERROR,
		    "level %d not implemented yet", vol->level);
		return EINVAL;
	}

	metadata->magic = host2uint64_t_le(HR_MAGIC);
	metadata->extent_no = host2uint32_t_le(vol->dev_no);
	metadata->level = host2uint32_t_le(vol->level);
	metadata->nblocks = host2uint64_t_le(vol->nblocks);
	metadata->data_blkno = host2uint64_t_le(data_blkno);
	metadata->data_offset = host2uint32_t_le(data_offset);
	for (i = 0; i < vol->dev_no; i++) {
		metadata->index = host2uint32_t_le(i);

		rc = uuid_generate(&uuid);
		if (rc != EOK)
			goto error;
		uuid_encode(&uuid, metadata->uuid);

		str_cpy(metadata->devname, 32, vol->devname);

		rc = block_write_direct(vol->devs[i], HR_META_OFF, HR_META_SIZE,
		    metadata);
		if (rc != EOK)
			goto error;
	}

	/* fill in new members */
	vol->data_offset = data_offset;
	vol->data_blkno = data_blkno;

error:
	free(metadata);
	return rc;
}

errno_t hr_get_vol_from_meta(hr_config_t *cfg, hr_volume_t *new_volume)
{
	log_msg(LOG_DEFAULT, LVL_NOTE, "hr_get_vol_from_meta()");

	errno_t rc;
	hr_metadata_t *metadata;

	metadata = calloc(1, HR_META_SIZE * new_volume->bsize);
	if (metadata == NULL)
		return ENOMEM;

	/* for now assume metadata are in sync across extents */
	rc = read_metadata(cfg->devs[0], metadata);
	if (rc != EOK)
		goto end;

	/* TODO: sort new_volume->devs according to metadata extent index */

	if (uint64_t_le2host(metadata->magic) != HR_MAGIC) {
		printf("invalid magic\n");
		rc = EINVAL;
		goto end;
	}

	new_volume->level = uint32_t_le2host(metadata->level);
	new_volume->dev_no = uint32_t_le2host(metadata->extent_no);
	new_volume->nblocks = uint64_t_le2host(metadata->nblocks);
	new_volume->data_blkno = uint64_t_le2host(metadata->data_blkno);
	new_volume->data_offset = uint32_t_le2host(metadata->data_offset);

	if (str_cmp(metadata->devname, new_volume->devname) != 0) {
		log_msg(LOG_DEFAULT, LVL_NOTE,
		    "devname on metadata (%s) and config (%s) differ, using config",
		    metadata->devname, new_volume->devname);
	}
end:
	free(metadata);
	return EOK;
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
