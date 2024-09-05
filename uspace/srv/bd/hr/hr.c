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

#include "var.h"

loc_srv_t *hr_srv;
fibril_mutex_t big_lock; /* for now */

static fibril_mutex_t hr_volumes_lock;
static list_t hr_volumes;

static service_id_t ctl_sid;

static void hr_create_srv(ipc_call_t *icall)
{
	log_msg(LOG_DEFAULT, LVL_NOTE, "hr_create_srv()");

	errno_t rc;
	size_t size;
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
		async_answer_0(&call, rc);
		async_answer_0(icall, rc);
	}

	new_volume = calloc(1, sizeof(hr_volume_t));
	if (new_volume == NULL) {
		rc = ENOMEM;
		goto end;
	}

	str_cpy(new_volume->devname, 32, cfg->devname);
	memcpy(new_volume->devs, cfg->devs, sizeof(service_id_t) * HR_MAXDEVS);
	new_volume->level = cfg->level;
	new_volume->dev_no = cfg->dev_no;

	switch (new_volume->level) {
	case hr_l_1:
		new_volume->hr_ops.create = hr_raid1_create;
		break;
	case hr_l_0:
		new_volume->hr_ops.create = hr_raid0_create;
		break;
	default:
		log_msg(LOG_DEFAULT, LVL_NOTE,
		    "level %d not implemented yet\n", new_volume->level);
		rc = EINVAL;
		goto end;
	}

	rc = new_volume->hr_ops.create(new_volume);
	if (rc != EOK) {
		goto end;
	}

	fibril_mutex_lock(&hr_volumes_lock);
	list_append(&new_volume->lvolumes, &hr_volumes);
	fibril_mutex_unlock(&hr_volumes_lock);

	log_msg(LOG_DEFAULT, LVL_NOTE, "created volume \"%s\" (%" PRIun ")\n",
	    new_volume->devname, new_volume->svc_id);

end:
	free(cfg);
	async_answer_0(icall, rc);
}

static void hr_print_status_srv(ipc_call_t *icall)
{
	log_msg(LOG_DEFAULT, LVL_NOTE, "hr_status_srv()");

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

	list_foreach(hr_volumes, lvolumes, hr_volume_t, volume) {
		memcpy(info.extents, volume->devs,
		    sizeof(service_id_t) * HR_MAXDEVS);
		info.svc_id = volume->svc_id;
		info.extent_no = volume->dev_no;
		info.level = volume->level;
		info.nblocks = volume->nblocks;
		info.bsize = volume->bsize;

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
	log_msg(LOG_DEFAULT, LVL_NOTE, "hr_ctl_conn()");

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
		case HR_STATUS:
			hr_print_status_srv(&call);
			break;
		default:
			async_answer_0(&call, EINVAL);
		}
	}
}

static hr_volume_t *hr_get_volume(service_id_t svc_id)
{
	log_msg(LOG_DEFAULT, LVL_DEBUG, "hr_get_volume(): (%" PRIun ")",
	    svc_id);

	fibril_mutex_lock(&hr_volumes_lock);
	list_foreach(hr_volumes, lvolumes, hr_volume_t, volume) {
		if (volume->svc_id == svc_id) {
			fibril_mutex_unlock(&hr_volumes_lock);
			return volume;
		}
	}

	fibril_mutex_unlock(&hr_volumes_lock);
	return NULL;
}

static void hr_client_conn(ipc_call_t *icall, void *arg)
{
	log_msg(LOG_DEFAULT, LVL_NOTE, "hr_client_conn()");

	hr_volume_t *vol;

	service_id_t svc_id = ipc_get_arg2(icall);

	if (svc_id == ctl_sid) {
		hr_ctl_conn(icall, arg);
	} else {
		log_msg(LOG_DEFAULT, LVL_NOTE, "bd_conn()");
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

	fibril_mutex_initialize(&big_lock);

	fibril_mutex_initialize(&hr_volumes_lock);
	list_initialize(&hr_volumes);

	async_set_fallback_port_handler(hr_client_conn, NULL);

	rc = loc_server_register(NAME, &hr_srv);
	if (rc != EOK) {
		log_msg(LOG_DEFAULT, LVL_ERROR,
		    "failed registering server: %s", str_error(rc));
		return EEXIST;
	}

	rc = loc_service_register(hr_srv, SERVICE_NAME_HR, &ctl_sid);
	if (rc != EOK) {
		log_msg(LOG_DEFAULT, LVL_ERROR,
		    "failed registering service: %s", str_error(rc));
		return EEXIST;
	}

	printf("%s: accepting connections\n", NAME);
	task_retval(0);
	async_manager();

	return 0;
}

/** @}
 */
