/*
 * Copyright (c) 2015 Jiri Svoboda
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

/** @addtogroup volsrv
 * @{
 */
/**
 * @file Volume service
 */

#include <async.h>
#include <errno.h>
#include <str_error.h>
#include <io/log.h>
#include <ipc/services.h>
#include <ipc/vol.h>
#include <loc.h>
#include <macros.h>
#include <stdio.h>
#include <stdlib.h>
#include <task.h>
#include <types/vol.h>

#include "mkfs.h"
#include "part.h"

#define NAME  "volsrv"

static void vol_client_conn(cap_call_handle_t, ipc_call_t *, void *);

static errno_t vol_init(void)
{
	errno_t rc;
	log_msg(LOG_DEFAULT, LVL_DEBUG, "vol_init()");

	rc = vol_part_init();
	if (rc != EOK)
		return rc;

	rc = vol_part_discovery_start();
	if (rc != EOK)
		return rc;

	async_set_fallback_port_handler(vol_client_conn, NULL);

	rc = loc_server_register(NAME);
	if (rc != EOK) {
		log_msg(LOG_DEFAULT, LVL_ERROR, "Failed registering server: %s.", str_error(rc));
		return EEXIST;
	}

	service_id_t sid;
	rc = loc_service_register(SERVICE_NAME_VOLSRV, &sid);
	if (rc != EOK) {
		log_msg(LOG_DEFAULT, LVL_ERROR, "Failed registering service: %s.", str_error(rc));
		return EEXIST;
	}

	return EOK;
}

static void vol_get_parts_srv(cap_call_handle_t icall_handle, ipc_call_t *icall)
{
	cap_call_handle_t chandle;
	size_t size;
	size_t act_size;
	errno_t rc;

	if (!async_data_read_receive(&chandle, &size)) {
		async_answer_0(chandle, EREFUSED);
		async_answer_0(icall_handle, EREFUSED);
		return;
	}

	service_id_t *id_buf = (service_id_t *) malloc(size);
	if (id_buf == NULL) {
		async_answer_0(chandle, ENOMEM);
		async_answer_0(icall_handle, ENOMEM);
		return;
	}

	rc = vol_part_get_ids(id_buf, size, &act_size);
	if (rc != EOK) {
		async_answer_0(chandle, rc);
		async_answer_0(icall_handle, rc);
		return;
	}

	errno_t retval = async_data_read_finalize(chandle, id_buf, size);
	free(id_buf);

	async_answer_1(icall_handle, retval, act_size);
}

static void vol_part_add_srv(cap_call_handle_t icall_handle, ipc_call_t *icall)
{
	service_id_t sid;
	errno_t rc;

	sid = IPC_GET_ARG1(*icall);

	rc = vol_part_add(sid);
	if (rc != EOK) {
		async_answer_0(icall_handle, rc);
		return;
	}

	async_answer_0(icall_handle, EOK);
}

static void vol_part_info_srv(cap_call_handle_t icall_handle, ipc_call_t *icall)
{
	service_id_t sid;
	vol_part_t *part;
	vol_part_info_t pinfo;
	errno_t rc;

	sid = IPC_GET_ARG1(*icall);
	log_msg(LOG_DEFAULT, LVL_DEBUG, "vol_part_info_srv(%zu)",
	    sid);
	rc = vol_part_find_by_id(sid, &part);
	if (rc != EOK) {
		async_answer_0(icall_handle, ENOENT);
		return;
	}

	rc = vol_part_get_info(part, &pinfo);
	if (rc != EOK) {
		async_answer_0(icall_handle, EIO);
		return;
	}

	cap_call_handle_t chandle;
	size_t size;
	if (!async_data_read_receive(&chandle, &size)) {
		async_answer_0(chandle, EREFUSED);
		async_answer_0(icall_handle, EREFUSED);
		return;
	}

	if (size != sizeof(vol_part_info_t)) {
		async_answer_0(chandle, EINVAL);
		async_answer_0(icall_handle, EINVAL);
		return;
	}

	rc = async_data_read_finalize(chandle, &pinfo,
	    min(size, sizeof(pinfo)));
	if (rc != EOK) {
		async_answer_0(chandle, rc);
		async_answer_0(icall_handle, rc);
		return;
	}

	async_answer_0(icall_handle, EOK);
}

static void vol_part_empty_srv(cap_call_handle_t icall_handle, ipc_call_t *icall)
{
	service_id_t sid;
	vol_part_t *part;
	errno_t rc;

	sid = IPC_GET_ARG1(*icall);
	log_msg(LOG_DEFAULT, LVL_DEBUG, "vol_part_empty_srv(%zu)", sid);

	rc = vol_part_find_by_id(sid, &part);
	if (rc != EOK) {
		async_answer_0(icall_handle, ENOENT);
		return;
	}

	rc = vol_part_empty_part(part);
	if (rc != EOK) {
		async_answer_0(icall_handle, EIO);
		return;
	}

	async_answer_0(icall_handle, EOK);
}

static void vol_part_get_lsupp_srv(cap_call_handle_t icall_handle, ipc_call_t *icall)
{
	vol_fstype_t fstype;
	vol_label_supp_t vlsupp;
	errno_t rc;

	fstype = IPC_GET_ARG1(*icall);
	log_msg(LOG_DEFAULT, LVL_DEBUG, "vol_part_get_lsupp_srv(%u)",
	    fstype);

	volsrv_part_get_lsupp(fstype, &vlsupp);

	cap_call_handle_t chandle;
	size_t size;
	if (!async_data_read_receive(&chandle, &size)) {
		async_answer_0(chandle, EREFUSED);
		async_answer_0(icall_handle, EREFUSED);
		return;
	}

	if (size != sizeof(vol_label_supp_t)) {
		async_answer_0(chandle, EINVAL);
		async_answer_0(icall_handle, EINVAL);
		return;
	}

	rc = async_data_read_finalize(chandle, &vlsupp,
	    min(size, sizeof(vlsupp)));
	if (rc != EOK) {
		async_answer_0(chandle, rc);
		async_answer_0(icall_handle, rc);
		return;
	}

	async_answer_0(icall_handle, EOK);
}


static void vol_part_mkfs_srv(cap_call_handle_t icall_handle, ipc_call_t *icall)
{
	service_id_t sid;
	vol_part_t *part;
	vol_fstype_t fstype;
	char *label;
	errno_t rc;

	sid = IPC_GET_ARG1(*icall);
	fstype = IPC_GET_ARG2(*icall);

	rc = async_data_write_accept((void **)&label, true, 0, VOL_LABEL_MAXLEN,
	    0, NULL);
	if (rc != EOK) {
		async_answer_0(icall_handle, rc);
		return;
	}

	printf("vol_part_mkfs_srv: label=%p\n", label);
	if (label != NULL)
		printf("vol_part_mkfs_srv: label='%s'\n", label);

	rc = vol_part_find_by_id(sid, &part);
	if (rc != EOK) {
		free(label);
		async_answer_0(icall_handle, ENOENT);
		return;
	}

	rc = vol_part_mkfs_part(part, fstype, label);
	if (rc != EOK) {
		free(label);
		async_answer_0(icall_handle, rc);
		return;
	}

	free(label);
	async_answer_0(icall_handle, EOK);
}

static void vol_client_conn(cap_call_handle_t icall_handle, ipc_call_t *icall, void *arg)
{
	log_msg(LOG_DEFAULT, LVL_DEBUG, "vol_client_conn()");

	/* Accept the connection */
	async_answer_0(icall_handle, EOK);

	while (true) {
		ipc_call_t call;
		cap_call_handle_t chandle = async_get_call(&call);
		sysarg_t method = IPC_GET_IMETHOD(call);

		if (!method) {
			/* The other side has hung up */
			async_answer_0(chandle, EOK);
			return;
		}

		switch (method) {
		case VOL_GET_PARTS:
			vol_get_parts_srv(chandle, &call);
			break;
		case VOL_PART_ADD:
			vol_part_add_srv(chandle, &call);
			break;
		case VOL_PART_INFO:
			vol_part_info_srv(chandle, &call);
			break;
		case VOL_PART_EMPTY:
			vol_part_empty_srv(chandle, &call);
			break;
		case VOL_PART_LSUPP:
			vol_part_get_lsupp_srv(chandle, &call);
			break;
		case VOL_PART_MKFS:
			vol_part_mkfs_srv(chandle, &call);
			break;
		default:
			async_answer_0(chandle, EINVAL);
		}
	}
}

int main(int argc, char *argv[])
{
	errno_t rc;

	printf("%s: Volume service\n", NAME);

	if (log_init(NAME) != EOK) {
		printf(NAME ": Failed to initialize logging.\n");
		return 1;
	}

	rc = vol_init();
	if (rc != EOK)
		return 1;

	printf(NAME ": Accepting connections.\n");
	task_retval(0);
	async_manager();

	return 0;
}

/** @}
 */
