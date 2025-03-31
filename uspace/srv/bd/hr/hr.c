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
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <str.h>
#include <str_error.h>
#include <block.h>

#include "fge.h"
#include "io.h"
#include "superblock.h"
#include "util.h"
#include "var.h"

static void hr_assemble_srv(ipc_call_t *);
static void hr_auto_assemble_srv(ipc_call_t *);
static void hr_stop_srv(ipc_call_t *);
static void hr_add_hotspare_srv(ipc_call_t *);
static void hr_print_status_srv(ipc_call_t *);
static void hr_ctl_conn(ipc_call_t *, void *);
static void hr_client_conn(ipc_call_t *, void *);

loc_srv_t *hr_srv;
list_t hr_volumes;
fibril_rwlock_t hr_volumes_lock;

static service_id_t ctl_sid;

static void hr_create_srv(ipc_call_t *icall)
{
	HR_DEBUG("%s()", __func__);

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
	for (i = 0; i < cfg->dev_no; i++) {
		if (cfg->devs[i] == 0) {
			/*
			 * XXX: own error codes, no need to log this...
			 * its user error not service error
			 */
			HR_ERROR("missing device provided for array "
			    "creation, aborting");
			free(cfg);
			async_answer_0(icall, EINVAL);
			return;
		}
	}

	rc = hr_create_vol_struct(&new_volume, cfg->level, cfg->devname);
	if (rc != EOK) {
		free(cfg);
		async_answer_0(icall, rc);
		return;
	}

	rc = hr_init_extents_from_cfg(new_volume, cfg);
	if (rc != EOK)
		goto error;

	new_volume->hr_ops.init(new_volume);
	if (rc != EOK)
		goto error;

	rc = hr_write_meta_to_vol(new_volume);
	if (rc != EOK)
		goto error;

	rc = new_volume->hr_ops.create(new_volume);
	if (rc != EOK)
		goto error;

	rc = hr_register_volume(new_volume);
	if (rc != EOK)
		goto error;

	fibril_rwlock_write_lock(&hr_volumes_lock);
	list_append(&new_volume->lvolumes, &hr_volumes);
	fibril_rwlock_write_unlock(&hr_volumes_lock);

	HR_DEBUG("created volume \"%s\" (%" PRIun ")\n", new_volume->devname,
	    new_volume->svc_id);

	free(cfg);
	async_answer_0(icall, rc);
	return;
error:
	free(cfg);
	hr_destroy_vol_struct(new_volume);
	async_answer_0(icall, rc);
}

static void hr_assemble_srv(ipc_call_t *icall)
{
	HR_DEBUG("%s()", __func__);

	errno_t rc;
	size_t size, assembled_cnt;
	hr_config_t *cfg;
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
	if (rc != EOK)
		goto error;

	if (!async_data_read_receive(&call, &size)) {
		async_answer_0(icall, EREFUSED);
		return;
	}

	if (size != sizeof(size_t)) {
		async_answer_0(icall, EINVAL);
		return;
	}

	rc = hr_util_try_assemble(cfg, &assembled_cnt);
	if (rc != EOK)
		goto error;

	rc = async_data_read_finalize(&call, &assembled_cnt, size);
	if (rc != EOK)
		goto error;

	free(cfg);
	async_answer_0(icall, EOK);
	return;
error:
	free(cfg);
	async_answer_0(&call, rc);
	async_answer_0(icall, rc);
}

static void hr_auto_assemble_srv(ipc_call_t *icall)
{
	HR_DEBUG("%s()", __func__);

	errno_t rc;
	size_t size;
	size_t assembled_cnt = 0;
	ipc_call_t call;

	if (!async_data_read_receive(&call, &size)) {
		async_answer_0(icall, EREFUSED);
		return;
	}

	if (size != sizeof(size_t)) {
		async_answer_0(&call, EINVAL);
		async_answer_0(icall, EINVAL);
		return;
	}

	rc = hr_util_try_assemble(NULL, &assembled_cnt);
	if (rc != EOK)
		goto error;

	rc = async_data_read_finalize(&call, &assembled_cnt, size);
	if (rc != EOK)
		goto error;

	async_answer_0(icall, EOK);
	return;
error:
	async_answer_0(&call, rc);
	async_answer_0(icall, rc);
}

static void hr_stop_srv(ipc_call_t *icall)
{
	HR_DEBUG("%s()", __func__);

	errno_t rc = EOK;
	service_id_t svc_id;
	long fail_extent;
	hr_volume_t *vol;

	svc_id = ipc_get_arg1(icall);
	fail_extent = (long)ipc_get_arg2(icall);

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
	} else {
		fibril_rwlock_write_lock(&vol->states_lock);
		fibril_rwlock_read_lock(&vol->extents_lock);

		/* TODO: maybe expose extent state callbacks */
		hr_update_ext_status(vol, fail_extent, HR_EXT_FAILED);
		hr_mark_vol_state_dirty(vol);

		fibril_rwlock_read_unlock(&vol->extents_lock);
		fibril_rwlock_write_unlock(&vol->states_lock);

		vol->hr_ops.status_event(vol);
	}
	async_answer_0(icall, rc);
}

static void hr_add_hotspare_srv(ipc_call_t *icall)
{
	HR_DEBUG("%s()", __func__);

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
	HR_DEBUG("%s()", __func__);

	errno_t rc;
	size_t vol_cnt = 0;
	hr_vol_info_t info;
	ipc_call_t call;
	size_t size;

	fibril_rwlock_read_lock(&hr_volumes_lock);

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
		info.extent_no = vol->extent_no;
		info.hotspare_no = vol->hotspare_no;
		info.level = vol->level;
		/* print usable number of blocks */
		info.nblocks = vol->data_blkno;
		info.strip_size = vol->strip_size;
		info.bsize = vol->bsize;
		info.status = vol->status;
		info.layout = vol->layout;

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

	fibril_rwlock_read_unlock(&hr_volumes_lock);
	async_answer_0(icall, EOK);
	return;
error:
	fibril_rwlock_read_unlock(&hr_volumes_lock);
	async_answer_0(&call, rc);
	async_answer_0(icall, rc);
}

static void hr_ctl_conn(ipc_call_t *icall, void *arg)
{
	HR_DEBUG("%s()", __func__);

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
			hr_create_srv(&call);
			break;
		case HR_ASSEMBLE:
			hr_assemble_srv(&call);
			break;
		case HR_AUTO_ASSEMBLE:
			hr_auto_assemble_srv(&call);
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
	HR_DEBUG("%s()", __func__);

	hr_volume_t *vol;

	service_id_t svc_id = ipc_get_arg2(icall);

	if (svc_id == ctl_sid) {
		hr_ctl_conn(icall, arg);
	} else {
		HR_DEBUG("bd_conn()\n");
		vol = hr_get_volume(svc_id);
		if (vol == NULL)
			async_answer_0(icall, ENOENT);
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

	fibril_rwlock_initialize(&hr_volumes_lock);
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
