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

#include "util.h"
#include "var.h"

extern fibril_mutex_t big_lock;
extern loc_srv_t *hr_srv;

static errno_t hr_raid1_bd_open(bd_srvs_t *, bd_srv_t *);
static errno_t hr_raid1_bd_close(bd_srv_t *);
static errno_t hr_raid1_bd_read_blocks(bd_srv_t *, aoff64_t, size_t, void *,
    size_t);
static errno_t hr_raid1_bd_sync_cache(bd_srv_t *, aoff64_t, size_t);
static errno_t hr_raid1_bd_write_blocks(bd_srv_t *, aoff64_t, size_t,
    const void *, size_t);
static errno_t hr_raid1_bd_get_block_size(bd_srv_t *, size_t *);
static errno_t hr_raid1_bd_get_num_blocks(bd_srv_t *, aoff64_t *);

static bd_ops_t hr_raid1_bd_ops = {
	.open = hr_raid1_bd_open,
	.close = hr_raid1_bd_close,
	.sync_cache = hr_raid1_bd_sync_cache,
	.read_blocks = hr_raid1_bd_read_blocks,
	.write_blocks = hr_raid1_bd_write_blocks,
	.get_block_size = hr_raid1_bd_get_block_size,
	.get_num_blocks = hr_raid1_bd_get_num_blocks
};

static errno_t hr_raid1_bd_open(bd_srvs_t *bds, bd_srv_t *bd)
{
	log_msg(LOG_DEFAULT, LVL_NOTE, "hr_bd_open()");
	return EOK;
}

static errno_t hr_raid1_bd_close(bd_srv_t *bd)
{
	log_msg(LOG_DEFAULT, LVL_NOTE, "hr_bd_close()");
	return EOK;
}

static errno_t hr_raid1_bd_sync_cache(bd_srv_t *bd, aoff64_t ba, size_t cnt)
{
	hr_volume_t *vol = bd->srvs->sarg;

	errno_t rc;
	size_t i;

	rc = hr_check_ba_range(vol, cnt, ba);
	if (rc != EOK)
		return rc;

	hr_add_ba_offset(vol, &ba);

	fibril_mutex_lock(&big_lock);

	for (i = 0; i < vol->dev_no; i++) {
		rc = block_sync_cache(vol->extents[i].svc_id, ba, cnt);
		if (rc != EOK)
			break;
	}

	fibril_mutex_unlock(&big_lock);
	return rc;
}

static errno_t hr_raid1_bd_read_blocks(bd_srv_t *bd, aoff64_t ba, size_t cnt,
    void *buf, size_t size)
{
	hr_volume_t *vol = bd->srvs->sarg;

	errno_t rc;
	size_t i;

	rc = hr_check_ba_range(vol, cnt, ba);
	if (rc != EOK)
		return rc;

	hr_add_ba_offset(vol, &ba);

	fibril_mutex_lock(&big_lock);

	for (i = 0; i < vol->dev_no; i++) {
		rc = block_read_direct(vol->extents[i].svc_id, ba, cnt, buf);
		if (rc != EOK)
			break;
	}

	fibril_mutex_unlock(&big_lock);
	return rc;
}

static errno_t hr_raid1_bd_write_blocks(bd_srv_t *bd, aoff64_t ba, size_t cnt,
    const void *data, size_t size)
{
	hr_volume_t *vol = bd->srvs->sarg;

	errno_t rc;
	size_t i;

	rc = hr_check_ba_range(vol, cnt, ba);
	if (rc != EOK)
		return rc;

	hr_add_ba_offset(vol, &ba);

	fibril_mutex_lock(&big_lock);

	for (i = 0; i < vol->dev_no; i++) {
		rc = block_write_direct(vol->extents[i].svc_id, ba, cnt, data);
		if (rc != EOK)
			break;
	}

	fibril_mutex_unlock(&big_lock);
	return rc;
}

static errno_t hr_raid1_bd_get_block_size(bd_srv_t *bd, size_t *rsize)
{
	hr_volume_t *vol = bd->srvs->sarg;

	*rsize = vol->bsize;
	return EOK;
}

static errno_t hr_raid1_bd_get_num_blocks(bd_srv_t *bd, aoff64_t *rnb)
{
	hr_volume_t *vol = bd->srvs->sarg;

	*rnb = vol->data_blkno;
	return EOK;
}

errno_t hr_raid1_create(hr_volume_t *new_volume)
{
	errno_t rc;

	assert(new_volume->level == hr_l_1);

	if (new_volume->dev_no < 2) {
		log_msg(LOG_DEFAULT, LVL_ERROR,
		    "RAID 1 array needs at least 2 devices");
		return EINVAL;
	}

	bd_srvs_init(&new_volume->hr_bds);
	new_volume->hr_bds.ops = &hr_raid1_bd_ops;
	new_volume->hr_bds.sarg = new_volume;

	rc = hr_register_volume(new_volume);
	if (rc != EOK)
		return rc;

	return EOK;
}

/** @}
 */
