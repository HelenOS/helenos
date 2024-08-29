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

#include <async.h>
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

static errno_t hr_raid1_bd_sync_cache(bd_srv_t *bd, aoff64_t ba, size_t size)
{
	fibril_mutex_lock(&big_lock);
	hr_volume_t *vol = bd->srvs->sarg;

	errno_t rc;
	size_t i;

	for (i = 0; i < vol->dev_no; i++) {
		rc = block_sync_cache(vol->devs[i], ba, size);
		if (rc != EOK)
			break;
	}

	fibril_mutex_unlock(&big_lock);
	return rc;
}

static errno_t hr_raid1_bd_read_blocks(bd_srv_t *bd, aoff64_t ba, size_t cnt,
    void *buf, size_t size)
{
	fibril_mutex_lock(&big_lock);
	hr_volume_t *vol = bd->srvs->sarg;

	errno_t rc;
	size_t i;

	for (i = 0; i < vol->dev_no; i++) {
		rc = block_read_direct(vol->devs[i], ba, cnt, buf);
		if (rc != EOK)
			break;
	}

	fibril_mutex_unlock(&big_lock);
	return rc;
}

static errno_t hr_raid1_bd_write_blocks(bd_srv_t *bd, aoff64_t ba, size_t cnt,
    const void *data, size_t size)
{
	fibril_mutex_lock(&big_lock);
	hr_volume_t *vol = bd->srvs->sarg;

	errno_t rc;
	size_t i;

	for (i = 0; i < vol->dev_no; i++) {
		rc = block_write_direct(vol->devs[i], ba, cnt, data);
		if (rc != EOK)
			break;
	}

	fibril_mutex_unlock(&big_lock);
	return rc;
}

static errno_t hr_raid1_bd_get_block_size(bd_srv_t *bd, size_t *rsize)
{
	hr_volume_t *vol = bd->srvs->sarg;

	return block_get_bsize(vol->devs[0], rsize);
}

static errno_t hr_raid1_bd_get_num_blocks(bd_srv_t *bd, aoff64_t *rnb)
{
	hr_volume_t *vol = bd->srvs->sarg;

	return block_get_nblocks(vol->devs[0], rnb);
}

errno_t hr_raid1_create(hr_volume_t *new_volume)
{
	assert(new_volume->level == hr_l_1);

	errno_t rc;
	service_id_t new_id;
	category_id_t cat_id;

	rc = hr_init_devs(new_volume);
	if (rc != EOK)
		goto end;

	bd_srvs_init(&new_volume->hr_bds);
	new_volume->hr_bds.ops = &hr_raid1_bd_ops;
	new_volume->hr_bds.sarg = new_volume;

	rc = loc_service_register(hr_srv, new_volume->devname, &new_id);
	if (rc != EOK) {
		log_msg(LOG_DEFAULT, LVL_ERROR,
		    "unable to register device \"%s\": %s\n",
		    new_volume->devname, str_error(rc));

		goto error;
	}

	rc = loc_category_get_id("raid", &cat_id, IPC_FLAG_BLOCKING);
	if (rc != EOK) {
		log_msg(LOG_DEFAULT, LVL_ERROR,
		    "failed resolving category \"raid\": %s\n", str_error(rc));
		goto error;
	}

	rc = loc_service_add_to_cat(hr_srv, new_id, cat_id);
	if (rc != EOK) {
		log_msg(LOG_DEFAULT, LVL_ERROR,
		    "failed adding \"%s\" to category \"raid\": %s\n",
		    new_volume->devname, str_error(rc));
		goto error;
	}

	new_volume->svc_id = new_id;

	return EOK;
error:
	hr_fini_devs(new_volume);
end:
	return rc;
}

/** @}
 */
