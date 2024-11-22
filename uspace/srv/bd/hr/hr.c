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
#include <errno.h>
#include <hr.h>
#include <io/log.h>
#include <inttypes.h>
#include <ipc/hr.h>
#include <ipc/services.h>
#include <loc.h>
#include <task.h>
#include <stdio.h>
#include <stdlib.h>
#include <str.h>
#include <str_error.h>

#include "superblock.h"
#include "util.h"
#include "var.h"

loc_srv_t *hr_srv;

static fibril_mutex_t hr_volumes_lock;
static list_t hr_volumes;

static service_id_t ctl_sid;

static hr_volume_t *hr_get_volume(service_id_t svc_id)
{
	HR_DEBUG("hr_get_volume(): (%" PRIun ")\n", svc_id);

	fibril_mutex_lock(&hr_volumes_lock);
	list_foreach(hr_volumes, lvolumes, hr_volume_t, vol) {
		if (vol->svc_id == svc_id) {
			fibril_mutex_unlock(&hr_volumes_lock);
			return vol;
		}
	}

	fibril_mutex_unlock(&hr_volumes_lock);
	return NULL;
}

static errno_t hr_remove_volume(service_id_t svc_id)
{
	HR_DEBUG("hr_remove_volume(): (%" PRIun ")\n", svc_id);

	fibril_mutex_lock(&hr_volumes_lock);
	list_foreach(hr_volumes, lvolumes, hr_volume_t, vol) {
		if (vol->svc_id == svc_id) {
			hr_fini_devs(vol);
			list_remove(&vol->lvolumes);
			free(vol);
			fibril_mutex_unlock(&hr_volumes_lock);
			return EOK;
		}
	}

	fibril_mutex_unlock(&hr_volumes_lock);
	return ENOENT;
}

static void hr_create_srv(ipc_call_t *icall, bool assemble)
{
	HR_DEBUG("hr_create_srv()\n");

	errno_t rc;
	size_t i, size;
	hr_config_t *cfg;
	hr_volume_t *new_volume;
	ipc_call_t call;

	if (!async_data_write_receive(&call, &size)) {
		async_answer_0(&call, EREFUSED);
		async_answer_0(icall, EREFUSED);
		return;
	}

	if (size != sizeof(hr_config_t)) {
		async_answer_0(&call, EINVAL);
		async_answer_0(icall, EINVAL);
		return;
	}

	cfg = calloc(1, sizeof(hr_config_t));
	if (cfg == NULL) {
		async_answer_0(&call, ENOMEM);
		async_answer_0(icall, ENOMEM);
		return;
	}

	rc = async_data_write_finalize(&call, cfg, size);
	if (rc != EOK) {
		free(cfg);
		async_answer_0(&call, rc);
		async_answer_0(icall, rc);
		return;
	}

	/*
	 * If there was a missing device provided
	 * for creation of a new array, abort
	 */
	if (!assemble) {
		for (i = 0; i < cfg->dev_no; i++) {
			if (cfg->devs[i] == 0) {
				HR_ERROR("missing device provided for array "
				    "creation, aborting");
				free(cfg);
				async_answer_0(icall, EINVAL);
				return;
			}
		}
	}

	new_volume = calloc(1, sizeof(hr_volume_t));
	if (new_volume == NULL) {
		free(cfg);
		async_answer_0(icall, ENOMEM);
		return;
	}

	str_cpy(new_volume->devname, HR_DEVNAME_LEN, cfg->devname);
	for (i = 0; i < cfg->dev_no; i++)
		new_volume->extents[i].svc_id = cfg->devs[i];
	new_volume->level = cfg->level;
	new_volume->dev_no = cfg->dev_no;

	if (assemble) {
		if (cfg->level != HR_LVL_UNKNOWN)
			HR_WARN("level manually set when assembling, ingoring");
		new_volume->level = HR_LVL_UNKNOWN;
	}

	rc = hr_init_devs(new_volume);
	if (rc != EOK) {
		free(cfg);
		free(new_volume);
		async_answer_0(icall, rc);
		return;
	}

	if (assemble) {
		/* just bsize needed for reading metadata later */
		rc = hr_check_devs(new_volume, NULL, &new_volume->bsize);
		if (rc != EOK)
			goto error;

		rc = hr_fill_vol_from_meta(new_volume);
		if (rc != EOK)
			goto error;
	}

	switch (new_volume->level) {
	case HR_LVL_1:
		new_volume->hr_ops.create = hr_raid1_create;
		new_volume->hr_ops.init = hr_raid1_init;
		new_volume->hr_ops.status_event = hr_raid1_status_event;
		new_volume->hr_ops.add_hotspare = hr_raid1_add_hotspare;
		break;
	case HR_LVL_0:
		new_volume->hr_ops.create = hr_raid0_create;
		new_volume->hr_ops.init = hr_raid0_init;
		new_volume->hr_ops.status_event = hr_raid0_status_event;
		break;
	case HR_LVL_4:
		new_volume->hr_ops.create = hr_raid4_create;
		new_volume->hr_ops.init = hr_raid4_init;
		new_volume->hr_ops.status_event = hr_raid4_status_event;
		new_volume->hr_ops.add_hotspare = hr_raid4_add_hotspare;
		break;
	case HR_LVL_5:
		new_volume->hr_ops.create = hr_raid5_create;
		new_volume->hr_ops.init = hr_raid5_init;
		new_volume->hr_ops.status_event = hr_raid5_status_event;
		break;
	default:
		HR_ERROR("unkown level: %d, aborting\n", new_volume->level);
		rc = EINVAL;
		goto error;
	}

	if (!assemble) {
		new_volume->hr_ops.init(new_volume);
		if (rc != EOK)
			goto error;

		rc = hr_write_meta_to_vol(new_volume);
		if (rc != EOK)
			goto error;
	}

	fibril_mutex_initialize(&new_volume->lock);

	rc = new_volume->hr_ops.create(new_volume);
	if (rc != EOK)
		goto error;

	fibril_mutex_lock(&hr_volumes_lock);
	list_append(&new_volume->lvolumes, &hr_volumes);
	fibril_mutex_unlock(&hr_volumes_lock);

	if (assemble) {
		HR_DEBUG("assembled volume \"%s\" (%" PRIun ")\n",
		    new_volume->devname, new_volume->svc_id);
	} else {
		HR_DEBUG("created volume \"%s\" (%" PRIun ")\n",
		    new_volume->devname, new_volume->svc_id);
	}

	free(cfg);
	async_answer_0(icall, rc);
	return;
error:
	free(cfg);
	hr_fini_devs(new_volume);
	free(new_volume);
	async_answer_0(icall, rc);
}

static void hr_stop_srv(ipc_call_t *icall)
{
	HR_DEBUG("hr_stop_srv()\n");

	errno_t rc = EOK;
	service_id_t svc_id;
	long fail_extent;
	hr_volume_t *vol;

	svc_id = ipc_get_arg1(icall);
	fail_extent = (long) ipc_get_arg2(icall);

	vol = hr_get_volume(svc_id);
	if (vol == NULL) {
		async_answer_0(icall, ENOENT);
		return;
	}

	if (fail_extent == -1) {
		rc = hr_remove_volume(svc_id);
		if (rc != EOK) {
			async_answer_0(icall, rc);
			return;
		}
		rc = loc_service_unregister(hr_srv, svc_id);
	} else {
		/* fibril safe for now */
		fibril_mutex_lock(&vol->lock);
		hr_update_ext_status(vol, fail_extent, HR_EXT_FAILED);
		fibril_mutex_unlock(&vol->lock);

		vol->hr_ops.status_event(vol);
	}
	async_answer_0(icall, rc);
}

static void hr_add_hotspare_srv(ipc_call_t *icall)
{
	HR_DEBUG("hr_add_hotspare()\n");

	errno_t rc = EOK;
	service_id_t vol_svc_id;
	service_id_t hotspare;
	hr_volume_t *vol;

	vol_svc_id = ipc_get_arg1(icall);
	hotspare = ipc_get_arg2(icall);

	vol = hr_get_volume(vol_svc_id);
	if (vol == NULL) {
		async_answer_0(icall, ENOENT);
		return;
	}

	if (vol->hr_ops.add_hotspare == NULL) {
		HR_DEBUG("hr_add_hotspare(): not supported on RAID level %d\n",
		    vol->level);
		async_answer_0(icall, ENOTSUP);
		return;
	}

	rc = vol->hr_ops.add_hotspare(vol, hotspare);

	async_answer_0(icall, rc);
}

static void hr_print_status_srv(ipc_call_t *icall)
{
	HR_DEBUG("hr_status_srv()\n");

	errno_t rc;
	size_t vol_cnt = 0;
	hr_vol_info_t info;
	ipc_call_t call;
	size_t size;

	fibril_mutex_lock(&hr_volumes_lock);

	vol_cnt = list_count(&hr_volumes);

	if (!async_data_read_receive(&call, &size)) {
		rc = EREFUSED;
		goto error;
	}

	if (size != sizeof(size_t)) {
		rc = EINVAL;
		goto error;
	}

	rc = async_data_read_finalize(&call, &vol_cnt, size);
	if (rc != EOK)
		goto error;

	list_foreach(hr_volumes, lvolumes, hr_volume_t, vol) {
		memcpy(info.extents, vol->extents,
		    sizeof(hr_extent_t) * HR_MAX_EXTENTS);
		memcpy(info.hotspares, vol->hotspares,
		    sizeof(hr_extent_t) * HR_MAX_HOTSPARES);
		info.svc_id = vol->svc_id;
		info.extent_no = vol->dev_no;
		info.hotspare_no = vol->hotspare_no;
		info.level = vol->level;
		/* print usable number of blocks */
		info.nblocks = vol->data_blkno;
		info.strip_size = vol->strip_size;
		info.bsize = vol->bsize;
		info.status = vol->status;

		if (!async_data_read_receive(&call, &size)) {
			rc = EREFUSED;
			goto error;
		}

		if (size != sizeof(hr_vol_info_t)) {
			rc = EINVAL;
			goto error;
		}

		rc = async_data_read_finalize(&call, &info, size);
		if (rc != EOK)
			goto error;
	}

	fibril_mutex_unlock(&hr_volumes_lock);
	async_answer_0(icall, EOK);
	return;
error:
	fibril_mutex_unlock(&hr_volumes_lock);
	async_answer_0(&call, rc);
	async_answer_0(icall, rc);
}

static void hr_ctl_conn(ipc_call_t *icall, void *arg)
{
	HR_DEBUG("hr_ctl_conn()\n");

	async_accept_0(icall);

	while (true) {
		ipc_call_t call;
		async_get_call(&call);
		sysarg_t method = ipc_get_imethod(&call);

		if (!method) {
			async_answer_0(&call, EOK);
			return;
		}

		switch (method) {
		case HR_CREATE:
			hr_create_srv(&call, false);
			break;
		case HR_ASSEMBLE:
			hr_create_srv(&call, true);
			break;
		case HR_STOP:
			hr_stop_srv(&call);
			break;
		case HR_ADD_HOTSPARE:
			hr_add_hotspare_srv(&call);
			break;
		case HR_STATUS:
			hr_print_status_srv(&call);
			break;
		default:
			async_answer_0(&call, EINVAL);
		}
	}
}

static void hr_client_conn(ipc_call_t *icall, void *arg)
{
	HR_DEBUG("hr_client_conn()\n");

	hr_volume_t *vol;

	service_id_t svc_id = ipc_get_arg2(icall);

	if (svc_id == ctl_sid) {
		hr_ctl_conn(icall, arg);
	} else {
		HR_DEBUG("bd_conn()\n");
		vol = hr_get_volume(svc_id);
		if (vol == NULL)
			async_answer_0(icall, EINVAL);
		bd_conn(icall, &vol->hr_bds);
	}
}

int main(int argc, char **argv)
{
	errno_t rc;

	printf("%s: HelenRAID server\n", NAME);

	rc = log_init(NAME);
	if (rc != EOK) {
		printf("%s: failed to initialize logging\n", NAME);
		return 1;
	}

	fibril_mutex_initialize(&hr_volumes_lock);
	list_initialize(&hr_volumes);

	async_set_fallback_port_handler(hr_client_conn, NULL);

	rc = loc_server_register(NAME, &hr_srv);
	if (rc != EOK) {
		HR_ERROR("failed registering server: %s", str_error(rc));
		return EEXIST;
	}

	rc = loc_service_register(hr_srv, SERVICE_NAME_HR, &ctl_sid);
	if (rc != EOK) {
		HR_ERROR("failed registering service: %s", str_error(rc));
		return EEXIST;
	}

	printf("%s: accepting connections\n", NAME);
	task_retval(0);
	async_manager();

	return 0;
}

/** @}
 */
