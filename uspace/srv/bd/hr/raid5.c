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

#include <abi/ipc/ipc.h>
#include <bd_srv.h>
#include <block.h>
#include <errno.h>
#include <hr.h>
#include <io/log.h>
#include <ipc/hr.h>
#include <ipc/services.h>
#include <loc.h>
#include <task.h>
#include <stdio.h>
#include <stdlib.h>
#include <str_error.h>

#include "superblock.h"
#include "util.h"
#include "var.h"

extern loc_srv_t *hr_srv;

static errno_t hr_raid5_bd_open(bd_srvs_t *, bd_srv_t *);
static errno_t hr_raid5_bd_close(bd_srv_t *);
static errno_t hr_raid5_bd_read_blocks(bd_srv_t *, aoff64_t, size_t, void *,
    size_t);
static errno_t hr_raid5_bd_sync_cache(bd_srv_t *, aoff64_t, size_t);
static errno_t hr_raid5_bd_write_blocks(bd_srv_t *, aoff64_t, size_t,
    const void *, size_t);
static errno_t hr_raid5_bd_get_block_size(bd_srv_t *, size_t *);
static errno_t hr_raid5_bd_get_num_blocks(bd_srv_t *, aoff64_t *);

static bd_ops_t hr_raid5_bd_ops = {
	.open = hr_raid5_bd_open,
	.close = hr_raid5_bd_close,
	.sync_cache = hr_raid5_bd_sync_cache,
	.read_blocks = hr_raid5_bd_read_blocks,
	.write_blocks = hr_raid5_bd_write_blocks,
	.get_block_size = hr_raid5_bd_get_block_size,
	.get_num_blocks = hr_raid5_bd_get_num_blocks
};

static void xor(void *dst, const void *src, size_t size)
{
	size_t i;
	uint64_t *d = dst;
	const uint64_t *s = src;

	for (i = 0; i < size / sizeof(uint64_t); ++i)
		*d++ ^= *s++;
}

static errno_t write_parity(hr_volume_t *vol, uint64_t p_extent,
    uint64_t extent, uint64_t block, const void *data)
{
	errno_t rc;
	size_t i;
	void *xorbuf;
	void *buf;

	xorbuf = calloc(1, vol->bsize);
	if (xorbuf == NULL)
		return ENOMEM;

	buf = malloc(vol->bsize);
	if (buf == NULL) {
		free(xorbuf);
		return ENOMEM;
	}

	for (i = 0; i < vol->dev_no; i++) {
		if (i == p_extent)
			continue;

		if (i == extent) {
			xor(xorbuf, data, vol->bsize);
		} else {
			rc = block_read_direct(vol->extents[i].svc_id, block, 1, buf);
			if (rc != EOK)
				goto end;
			xor(xorbuf, buf, vol->bsize);
		}
	}

	rc = block_write_direct(vol->extents[p_extent].svc_id, block, 1, xorbuf);

end:
	free(xorbuf);
	free(buf);
	return rc;
}

static void raid5_geometry(uint64_t x, hr_volume_t *vol, size_t *extent,
    uint64_t *phys_block, uint64_t *p_extent)
{
	uint64_t N = vol->dev_no; /* extents */
	uint64_t L = vol->strip_size / vol->bsize; /* size of strip in blocks */

	uint64_t p = ((x / L) / (N - 1)) % N;

	uint64_t i; /* extent */
	if (((x / L) % (N - 1)) < p)
		i = (x / L) % (N - 1);
	else
		i = ((x / L) % (N - 1)) + 1;

	uint64_t j = (x / L) / (N - 1); /* stripe */
	uint64_t k = x % L; /* strip offset */

	*extent = i;
	*phys_block = j * L + k;
	if (p_extent != NULL)
		*p_extent = p;
}

static errno_t hr_raid5_bd_open(bd_srvs_t *bds, bd_srv_t *bd)
{
	log_msg(LOG_DEFAULT, LVL_NOTE, "hr_bd_open()");
	return EOK;
}

static errno_t hr_raid5_bd_close(bd_srv_t *bd)
{
	log_msg(LOG_DEFAULT, LVL_NOTE, "hr_bd_close()");
	return EOK;
}

static errno_t hr_raid5_bd_sync_cache(bd_srv_t *bd, aoff64_t ba, size_t cnt)
{
	hr_volume_t *vol = bd->srvs->sarg;
	errno_t rc;
	uint64_t phys_block;
	size_t extent;

	rc = hr_check_ba_range(vol, cnt, ba);
	if (rc != EOK)
		return rc;

	fibril_mutex_lock(&vol->lock);

	size_t left = cnt;
	while (left != 0) {
		raid5_geometry(ba, vol, &extent, &phys_block, NULL);
		hr_add_ba_offset(vol, &phys_block);
		rc = block_sync_cache(vol->extents[extent].svc_id, phys_block, 1);
		if (rc != EOK)
			break;
		left--;
		ba++;
	}

	fibril_mutex_unlock(&vol->lock);
	return rc;
}

static errno_t hr_raid5_bd_read_blocks(bd_srv_t *bd, aoff64_t ba, size_t cnt,
    void *buf, size_t size)
{
	hr_volume_t *vol = bd->srvs->sarg;
	errno_t rc;
	uint64_t phys_block;
	size_t extent;

	if (size < cnt * vol->bsize)
		return EINVAL;

	rc = hr_check_ba_range(vol, cnt, ba);
	if (rc != EOK)
		return rc;

	fibril_mutex_lock(&vol->lock);

	size_t left = cnt;
	while (left != 0) {
		raid5_geometry(ba, vol, &extent, &phys_block, NULL);
		hr_add_ba_offset(vol, &phys_block);
		rc = block_read_direct(vol->extents[extent].svc_id, phys_block, 1, buf);
		buf = buf + vol->bsize;
		if (rc != EOK)
			break;
		left--;
		ba++;
	}

	fibril_mutex_unlock(&vol->lock);
	return rc;
}

static errno_t hr_raid5_bd_write_blocks(bd_srv_t *bd, aoff64_t ba, size_t cnt,
    const void *data, size_t size)
{
	hr_volume_t *vol = bd->srvs->sarg;
	errno_t rc;
	uint64_t phys_block, p_extent;
	size_t extent;

	if (size < cnt * vol->bsize)
		return EINVAL;

	rc = hr_check_ba_range(vol, cnt, ba);
	if (rc != EOK)
		return rc;

	fibril_mutex_lock(&vol->lock);

	size_t left = cnt;
	while (left != 0) {
		raid5_geometry(ba, vol, &extent, &phys_block, &p_extent);
		hr_add_ba_offset(vol, &phys_block);
		rc = block_write_direct(vol->extents[extent].svc_id, phys_block, 1, data);
		if (rc != EOK)
			break;
		rc = write_parity(vol, p_extent, extent, phys_block, data);
		if (rc != EOK)
			break;
		data = data + vol->bsize;
		left--;
		ba++;
	}

	fibril_mutex_unlock(&vol->lock);
	return rc;
}

static errno_t hr_raid5_bd_get_block_size(bd_srv_t *bd, size_t *rsize)
{
	hr_volume_t *vol = bd->srvs->sarg;

	*rsize = vol->bsize;
	return EOK;
}

static errno_t hr_raid5_bd_get_num_blocks(bd_srv_t *bd, aoff64_t *rnb)
{
	hr_volume_t *vol = bd->srvs->sarg;

	*rnb = vol->data_blkno;
	return EOK;
}

errno_t hr_raid5_create(hr_volume_t *new_volume)
{
	errno_t rc;

	assert(new_volume->level == hr_l_5);

	if (new_volume->dev_no < 3) {
		log_msg(LOG_DEFAULT, LVL_ERROR,
		    "RAID 5 array needs at least 3 devices");
		return EINVAL;
	}

	bd_srvs_init(&new_volume->hr_bds);
	new_volume->hr_bds.ops = &hr_raid5_bd_ops;
	new_volume->hr_bds.sarg = new_volume;

	rc = hr_register_volume(new_volume);

	return rc;
}

errno_t hr_raid5_init(hr_volume_t *vol)
{
	errno_t rc;
	size_t bsize;
	uint64_t total_blkno;

	assert(vol->level == hr_l_5);

	rc = hr_check_devs(vol, &total_blkno, &bsize);
	if (rc != EOK)
		return rc;

	vol->nblocks = total_blkno;
	vol->bsize = bsize;
	vol->data_offset = HR_DATA_OFF;
	vol->data_blkno = vol->nblocks - (vol->data_offset * vol->dev_no) -
	    (vol->nblocks / vol->dev_no);
	vol->strip_size = HR_STRIP_SIZE;

	return EOK;
}

/** @}
 */
