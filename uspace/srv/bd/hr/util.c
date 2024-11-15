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
#include <errno.h>
#include <hr.h>
#include <io/log.h>
#include <loc.h>
#include <stdlib.h>
#include <stdio.h>
#include <str_error.h>

#include "util.h"
#include "var.h"

extern loc_srv_t *hr_srv;

errno_t hr_init_devs(hr_volume_t *vol)
{
	DPRINTF("hr_init_devs()\n");

	errno_t rc;
	size_t i;
	hr_extent_t *extent;

	for (i = 0; i < vol->dev_no; i++) {
		extent = &vol->extents[i];
		if (extent->svc_id == 0) {
			extent->status = HR_EXT_MISSING;
			continue;
		}

		DPRINTF("hr_init_devs(): block_init() on (%lu)\n",
		    extent->svc_id);
		rc = block_init(extent->svc_id);
		extent->status = HR_EXT_ONLINE;

		if (rc != EOK) {
			ERR_PRINTF("hr_init_devs(): initing (%lu) failed, "
			    "aborting\n", extent->svc_id);
			break;
		}
	}

	return rc;
}

void hr_fini_devs(hr_volume_t *vol)
{
	DPRINTF("hr_fini_devs()\n");

	size_t i;

	for (i = 0; i < vol->dev_no; i++) {
		if (vol->extents[i].status != HR_EXT_MISSING) {
			DPRINTF("hr_fini_devs(): block_fini() on (%lu)\n",
			    vol->extents[i].svc_id);
			block_fini(vol->extents[i].svc_id);
		}
	}
}

errno_t hr_register_volume(hr_volume_t *vol)
{
	DPRINTF("hr_register_volume()\n");

	errno_t rc;
	service_id_t new_id;
	category_id_t cat_id;
	char *fullname = NULL;
	char *devname = vol->devname;

	if (asprintf(&fullname, "devices/%s", devname) < 0)
		return ENOMEM;

	rc = loc_service_register(hr_srv, fullname, &new_id);
	if (rc != EOK) {
		ERR_PRINTF("unable to register device \"%s\": %s\n",
		    fullname, str_error(rc));
		goto error;
	}

	rc = loc_category_get_id("raid", &cat_id, IPC_FLAG_BLOCKING);
	if (rc != EOK) {
		ERR_PRINTF("failed resolving category \"raid\": %s\n",
		    str_error(rc));
		goto error;
	}

	rc = loc_service_add_to_cat(hr_srv, new_id, cat_id);
	if (rc != EOK) {
		ERR_PRINTF("failed adding \"%s\" to category \"raid\": %s\n",
		    fullname, str_error(rc));
		goto error;
	}

	vol->svc_id = new_id;

error:
	free(fullname);
	return rc;
}

errno_t hr_check_devs(hr_volume_t *vol, uint64_t *rblkno, size_t *rbsize)
{
	DPRINTF("hr_check_devs()\n");

	errno_t rc;
	size_t i, bsize;
	uint64_t nblocks;
	size_t last_bsize = 0;
	uint64_t last_nblocks = 0;
	uint64_t total_blocks = 0;
	hr_extent_t *extent;

	for (i = 0; i < vol->dev_no; i++) {
		extent = &vol->extents[i];
		if (extent->status == HR_EXT_MISSING)
			continue;
		rc = block_get_nblocks(extent->svc_id, &nblocks);
		if (rc != EOK)
			goto error;
		if (last_nblocks != 0 && nblocks != last_nblocks) {
			ERR_PRINTF("number of blocks differs\n");
			rc = EINVAL;
			goto error;
		}

		total_blocks += nblocks;
		last_nblocks = nblocks;
	}

	for (i = 0; i < vol->dev_no; i++) {
		extent = &vol->extents[i];
		if (extent->status == HR_EXT_MISSING)
			continue;
		rc = block_get_bsize(extent->svc_id, &bsize);
		if (rc != EOK)
			goto error;
		if (last_bsize != 0 && bsize != last_bsize) {
			ERR_PRINTF("block sizes differ\n");
			rc = EINVAL;
			goto error;
		}

		last_bsize = bsize;
	}

	if ((bsize % 512) != 0) {
		ERR_PRINTF("block size not multiple of 512\n");
		return EINVAL;
	}

	if (rblkno != NULL)
		*rblkno = total_blocks;
	if (rbsize != NULL)
		*rbsize = bsize;
error:
	return rc;
}

errno_t hr_check_ba_range(hr_volume_t *vol, size_t cnt, uint64_t ba)
{
	if (ba + cnt > vol->data_blkno)
		return ERANGE;
	return EOK;
}

void hr_add_ba_offset(hr_volume_t *vol, uint64_t *ba)
{
	*ba = *ba + vol->data_offset;
}

void hr_update_ext_status(hr_volume_t *vol, uint64_t extent, hr_ext_status_t s)
{
	log_msg(LOG_DEFAULT, LVL_WARN,
	    "vol %s, changing extent: %lu, to status: %s",
	    vol->devname, extent, hr_get_ext_status_msg(s));
	vol->extents[extent].status = s;
}

/*
 * Do a whole sync (ba = 0, cnt = 0) across all extents,
 * and update extent status. *For now*, the caller has to
 * update volume status after the syncs.
 *
 * TODO: add update_vol_status fcn ptr for each raid
 */
void hr_sync_all_extents(hr_volume_t *vol)
{
	errno_t rc;

	fibril_mutex_lock(&vol->lock);
	for (size_t i = 0; i < vol->dev_no; i++) {
		if (vol->extents[i].status != HR_EXT_ONLINE)
			continue;
		rc = block_sync_cache(vol->extents[i].svc_id, 0, 0);
		if (rc != EOK && rc != ENOTSUP) {
			if (rc == ENOENT)
				hr_update_ext_status(vol, i, HR_EXT_MISSING);
			else if (rc != EOK)
				hr_update_ext_status(vol, i, HR_EXT_FAILED);
		}
	}
	fibril_mutex_unlock(&vol->lock);
}

/** @}
 */
