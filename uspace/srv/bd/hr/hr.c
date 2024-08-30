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
#include <inttypes.h>
#include <ipc/hr.h>
#include <ipc/services.h>
#include <loc.h>
#include <task.h>
#include <stdio.h>
#include <stdlib.h>
#include <str_error.h>

#include "var.h"

loc_srv_t *hr_srv;
fibril_mutex_t big_lock; /* for now */

static fibril_mutex_t hr_volumes_lock;
static list_t hr_volumes;

static service_id_t ctl_sid;

errno_t hr_init_devs(hr_volume_t *vol)
{
	log_msg(LOG_DEFAULT, LVL_NOTE, "hr_init_devs()");

	errno_t rc;
	size_t i;

	for (i = 0; i < vol->dev_no; i++) {
		rc = block_init(vol->devs[i]);
		log_msg(LOG_DEFAULT, LVL_DEBUG,
		    "hr_init_devs(): initing (%" PRIun ")", vol->devs[i]);
		if (rc != EOK) {
			log_msg(LOG_DEFAULT, LVL_ERROR,
			    "hr_init_devs(): initing (%" PRIun ") failed, aborting",
			    vol->devs[i]);
			break;
		}
	}

	return rc;
}

void hr_fini_devs(hr_volume_t *vol)
{
	log_msg(LOG_DEFAULT, LVL_NOTE, "hr_fini_devs()");

	size_t i;

	for (i = 0; i < vol->dev_no; i++)
		block_fini(vol->devs[i]);
}

static void hr_create_srv(ipc_call_t *icall)
{
	log_msg(LOG_DEFAULT, LVL_NOTE, "hr_create_srv()");

	errno_t rc;
	size_t size;
	hr_config_t *hr_config;

	hr_config = calloc(1, sizeof(hr_config_t));
	if (hr_config == NULL) {
		async_answer_0(icall, ENOMEM);
		return;
	}

	hr_config->level = ipc_get_arg1(icall);

	rc = async_data_write_accept((void **) &hr_config->name, true, 0, 0, 0,
	    NULL);
	if (rc != EOK) {
		async_answer_0(icall, EREFUSED);
		return;
	}

	rc = async_data_write_accept((void **) &hr_config->devs, false,
	    sizeof(service_id_t), 0, 0, &size);
	if (rc != EOK) {
		async_answer_0(icall, EREFUSED);
		return;
	}

	hr_config->dev_no = size / sizeof(service_id_t);

	fibril_mutex_initialize(&big_lock);

	hr_volume_t *new_volume = calloc(1, sizeof(hr_volume_t));
	if (new_volume == NULL) {
		rc = ENOMEM;
		goto end;
	}
	new_volume->devname = hr_config->name;
	new_volume->level = hr_config->level;
	new_volume->dev_no = hr_config->dev_no;
	new_volume->devs = hr_config->devs;

	switch (hr_config->level) {
	case hr_l_1:
		new_volume->hr_ops.create = hr_raid1_create;
		break;
	default:
		log_msg(LOG_DEFAULT, LVL_NOTE,
		    "level %d not implemented yet\n", new_volume->level);
		rc = EINVAL;
		goto end;
	}

	rc = new_volume->hr_ops.create(new_volume);
	if (rc != EOK)
		goto end;

	fibril_mutex_lock(&hr_volumes_lock);
	list_append(&new_volume->lvolumes, &hr_volumes);
	fibril_mutex_unlock(&hr_volumes_lock);

	log_msg(LOG_DEFAULT, LVL_NOTE, "created volume \"%s\" (%" PRIun ")\n",
	    new_volume->devname, new_volume->svc_id);

end:
	free(hr_config);
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

	sysarg_t svc_id = ipc_get_arg2(icall);

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
