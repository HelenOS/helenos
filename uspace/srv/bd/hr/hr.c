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
 * @file hr.c
 * @brief HelenRAID server methods.
 */

#include <adt/list.h>
#include <async.h>
#include <bd_srv.h>
#include <errno.h>
#include <hr.h>
#include <io/log.h>
#include <ipc/hr.h>
#include <ipc/services.h>
#include <loc.h>
#include <task.h>
#include <stdio.h>
#include <stdlib.h>
#include <str.h>
#include <str_error.h>
#include <block.h>

#include "util.h"
#include "var.h"

static void hr_assemble_srv(ipc_call_t *);
static void hr_auto_assemble_srv(ipc_call_t *);
static void hr_stop_srv(ipc_call_t *);
static void hr_stop_all_srv(ipc_call_t *);
static void hr_add_hotspare_srv(ipc_call_t *);
static void hr_get_vol_states_srv(ipc_call_t *);
static void hr_ctl_conn(ipc_call_t *);
static void hr_client_conn(ipc_call_t *, void *);

loc_srv_t *hr_srv;
list_t hr_volumes;
fibril_rwlock_t hr_volumes_lock;

static service_id_t ctl_sid;

/** Volume creation (server).
 *
 * Creates HelenRAID volume from parameters and
 * devices specified in hr_config_t.
 *
 * @param icall hr_config_t
 */
static void hr_create_srv(ipc_call_t *icall)
{
	HR_DEBUG("%s()", __func__);

	errno_t rc;
	size_t i, size;
	hr_config_t *cfg;
	hr_volume_t *vol;
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
	 * for creation of a new volume, abort
	 */
	for (i = 0; i < cfg->dev_no; i++) {
		if (cfg->devs[i] == 0) {
			/*
			 * XXX: own error codes, no need to log this...
			 * its user error not service error
			 */
			HR_ERROR("missing device provided for volume "
			    "creation, aborting");
			free(cfg);
			async_answer_0(icall, EINVAL);
			return;
		}
	}

	hr_metadata_type_t meta_type;
	if (cfg->vol_flags & HR_VOL_FLAG_NOOP_META)
		meta_type = HR_METADATA_NOOP;
	else
		meta_type = HR_METADATA_NATIVE;

	rc = hr_create_vol_struct(&vol, cfg->level, cfg->devname, meta_type,
	    cfg->vol_flags);
	if (rc != EOK) {
		free(cfg);
		async_answer_0(icall, rc);
		return;
	}

	rc = hr_init_extents_from_cfg(vol, cfg);
	if (rc != EOK)
		goto error;

	vol->hr_ops.init(vol);
	if (rc != EOK)
		goto error;

	rc = vol->meta_ops->init_vol2meta(vol);
	if (rc != EOK)
		goto error;

	rc = vol->hr_ops.create(vol);
	if (rc != EOK)
		goto error;

	vol->meta_ops->save(vol, WITH_STATE_CALLBACK);

	rc = hr_register_volume(vol);
	if (rc != EOK)
		goto error;

	fibril_rwlock_write_lock(&hr_volumes_lock);
	list_append(&vol->lvolumes, &hr_volumes);
	fibril_rwlock_write_unlock(&hr_volumes_lock);

	HR_NOTE("created volume \"%s\" (%" PRIun ")\n", vol->devname,
	    vol->svc_id);

	free(cfg);
	async_answer_0(icall, rc);
	return;
error:
	free(cfg);
	hr_destroy_vol_struct(vol);
	async_answer_0(icall, rc);
}

/** Manual volume assembly (server).
 *
 * Tries to assemble a volume from devices in hr_config_t and
 * sends the number of successful volumes assembled back to the
 * client.
 *
 * @param icall hr_config_t
 */
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

/** Automatic volume assembly (server).
 *
 * Tries to assemble a volume from devices in disk location
 * category and sends the number of successful volumes assembled
 * back to client.
 */
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

/** Volume deactivation (server).
 *
 * Deactivates/detaches specified volume.
 */
static void hr_stop_srv(ipc_call_t *icall)
{
	HR_DEBUG("%s()", __func__);

	errno_t rc = EOK;
	service_id_t svc_id;

	svc_id = ipc_get_arg1(icall);

	rc = hr_remove_volume(svc_id);

	async_answer_0(icall, rc);
}

/** Automatic volume deactivation (server).
 *
 * Tries to deactivate/detach all volumes.
 */
static void hr_stop_all_srv(ipc_call_t *icall)
{
	HR_DEBUG("%s()", __func__);

	service_id_t *vol_svcs = NULL;
	errno_t rc = EOK;
	size_t i, vol_cnt;

	rc = hr_get_volume_svcs(&vol_cnt, &vol_svcs);
	if (rc != EOK)
		goto fail;

	for (i = 0; i < vol_cnt; i++)
		(void)hr_remove_volume(vol_svcs[i]);

fail:
	if (vol_svcs != NULL)
		free(vol_svcs);
	async_answer_0(icall, rc);
}

/** Simulate volume extent failure (server).
 *
 * Changes the specified extent's state to FAULTY.
 * Other extents' metadata are marked as dirty, therefore
 * it effectively invalides the specified extent as well
 * for further uses.
 */
static void hr_fail_extent_srv(ipc_call_t *icall)
{
	HR_DEBUG("%s()", __func__);

	service_id_t svc_id;
	size_t extent_idx_to_fail;
	hr_volume_t *vol;

	svc_id = (service_id_t)ipc_get_arg1(icall);
	extent_idx_to_fail = (size_t)ipc_get_arg2(icall);

	vol = hr_get_volume(svc_id);
	if (vol == NULL) {
		async_answer_0(icall, ENOENT);
		return;
	}

	fibril_rwlock_read_lock(&vol->extents_lock);
	fibril_rwlock_write_lock(&vol->states_lock);

	hr_extent_t *ext = &vol->extents[extent_idx_to_fail];

	switch (ext->state) {
	case HR_EXT_NONE:
	case HR_EXT_MISSING:
	case HR_EXT_FAILED:
		fibril_rwlock_write_unlock(&vol->states_lock);
		fibril_rwlock_read_unlock(&vol->extents_lock);
		async_answer_0(icall, EINVAL);
		return;
	default:
		hr_update_ext_state(vol, extent_idx_to_fail, HR_EXT_FAILED);
		(void)vol->meta_ops->erase_block(ext->svc_id);
		hr_mark_vol_state_dirty(vol);
	}

	fibril_rwlock_write_unlock(&vol->states_lock);
	fibril_rwlock_read_unlock(&vol->extents_lock);

	vol->hr_ops.vol_state_eval(vol);

	async_answer_0(icall, EOK);
}

/** Add hotspare to volume (server).
 *
 * Adds hotspare to a volume.
 */
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

	if (vol->level == HR_LVL_0) {
		HR_NOTE("hotspare not supported on RAID level = %s\n",
		    hr_get_level_str(vol->level));
		async_answer_0(icall, ENOTSUP);
		return;
	}

	if (!(vol->meta_ops->get_flags() & HR_METADATA_HOTSPARE_SUPPORT)) {
		HR_NOTE("hotspare not supported on metadata type = %s\n",
		    hr_get_metadata_type_str(vol->meta_ops->get_type()));
		async_answer_0(icall, ENOTSUP);
		return;
	}

	rc = hr_util_add_hotspare(vol, hotspare);

	vol->hr_ops.vol_state_eval(vol);

	async_answer_0(icall, rc);
}

/** Send volume states.
 *
 * Sends the client pairs of (volume service_id, state).
 */
static void hr_get_vol_states_srv(ipc_call_t *icall)
{
	HR_DEBUG("%s()", __func__);

	errno_t rc;
	size_t vol_cnt = 0;
	hr_pair_vol_state_t pair;
	ipc_call_t call;
	size_t size;

	fibril_rwlock_read_lock(&hr_volumes_lock);

	vol_cnt = list_count(&hr_volumes);

	if (!async_data_read_receive(&call, &size)) {
		rc = EREFUSED;
		goto error;
	}

	if (size != sizeof(vol_cnt)) {
		rc = EINVAL;
		goto error;
	}

	rc = async_data_read_finalize(&call, &vol_cnt, size);
	if (rc != EOK)
		goto error;

	list_foreach(hr_volumes, lvolumes, hr_volume_t, vol) {
		pair.svc_id = vol->svc_id;
		pair.state = vol->state;

		if (!async_data_read_receive(&call, &size)) {
			rc = EREFUSED;
			goto error;
		}

		if (size != sizeof(pair)) {
			rc = EINVAL;
			goto error;
		}

		rc = async_data_read_finalize(&call, &pair, size);
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

/** Send volume info.
 *
 * Sends the client volume info.
 */
static void hr_get_vol_info_srv(ipc_call_t *icall)
{
	HR_DEBUG("%s()", __func__);

	errno_t rc;
	size_t size;
	ipc_call_t call;
	service_id_t svc_id;
	hr_vol_info_t info;
	hr_volume_t *vol;

	if (!async_data_write_receive(&call, &size)) {
		rc = EREFUSED;
		goto error;
	}

	if (size != sizeof(service_id_t)) {
		rc = EINVAL;
		goto error;
	}

	rc = async_data_write_finalize(&call, &svc_id, size);
	if (rc != EOK)
		goto error;

	vol = hr_get_volume(svc_id);
	if (vol == NULL) {
		rc = ENOENT;
		goto error;
	}

	memcpy(info.extents, vol->extents,
	    sizeof(hr_extent_t) * HR_MAX_EXTENTS);
	memcpy(info.hotspares, vol->hotspares,
	    sizeof(hr_extent_t) * HR_MAX_HOTSPARES);
	info.svc_id = vol->svc_id;
	info.extent_no = vol->extent_no;
	info.hotspare_no = vol->hotspare_no;
	info.level = vol->level;
	info.data_blkno = vol->data_blkno;
	info.rebuild_blk = vol->rebuild_blk;
	info.strip_size = vol->strip_size;
	info.bsize = vol->bsize;
	info.state = vol->state;
	info.layout = vol->layout;
	info.meta_type = vol->meta_ops->get_type();
	memcpy(info.devname, vol->devname, HR_DEVNAME_LEN);
	info.vflags = vol->vflags;

	if (!async_data_read_receive(&call, &size)) {
		rc = EREFUSED;
		goto error;
	}

	if (size != sizeof(info)) {
		rc = EINVAL;
		goto error;
	}

	rc = async_data_read_finalize(&call, &info, size);
	if (rc != EOK)
		goto error;

	async_answer_0(icall, EOK);
	return;
error:
	async_answer_0(&call, rc);
	async_answer_0(icall, rc);
}

/**  HelenRAID server control IPC methods crossroad.
 */
static void hr_ctl_conn(ipc_call_t *icall)
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
		case HR_STOP_ALL:
			hr_stop_all_srv(&call);
			break;
		case HR_FAIL_EXTENT:
			hr_fail_extent_srv(&call);
			break;
		case HR_ADD_HOTSPARE:
			hr_add_hotspare_srv(&call);
			break;
		case HR_GET_VOL_STATES:
			hr_get_vol_states_srv(&call);
			break;
		case HR_GET_VOL_INFO:
			hr_get_vol_info_srv(&call);
			break;
		default:
			async_answer_0(&call, EINVAL);
		}
	}
}

/**  HelenRAID server IPC method crossroad.
 *
 * Distinguishes between control IPC and block device
 * IPC calls.
 */
static void hr_client_conn(ipc_call_t *icall, void *arg)
{
	HR_DEBUG("%s()", __func__);

	hr_volume_t *vol;

	service_id_t svc_id = ipc_get_arg2(icall);

	if (svc_id == ctl_sid) {
		hr_ctl_conn(icall);
	} else {
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

	rc = loc_service_register(hr_srv, SERVICE_NAME_HR, fallback_port_id,
	    &ctl_sid);
	if (rc != EOK) {
		HR_ERROR("failed registering service: %s", str_error(rc));
		return EEXIST;
	}

	printf("%s: Trying automatic assembly.\n", NAME);
	size_t assembled = 0;
	(void)hr_util_try_assemble(NULL, &assembled);
	printf("%s: Assembled %zu volume(s).\n", NAME, assembled);

	printf("%s: Accepting connections.\n", NAME);
	task_retval(0);
	async_manager();

	return 0;
}

/** @}
 */
