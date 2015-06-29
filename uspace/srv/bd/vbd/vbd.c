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

/** @addtogroup vbd
 * @{
 */
/**
 * @file
 */

#include <async.h>
#include <errno.h>
#include <io/log.h>
#include <ipc/services.h>
#include <ipc/vbd.h>
#include <loc.h>
#include <stdio.h>
#include <stdlib.h>
#include <task.h>

#include "disk.h"
#include "types/vbd.h"

#define NAME  "vbd"

static void vbds_client_conn(ipc_callid_t, ipc_call_t *, void *);

static service_id_t ctl_sid;

static int vbds_init(void)
{
	int rc;
	log_msg(LOG_DEFAULT, LVL_NOTE, "vbds_init()");

	vbds_disks_init();

	async_set_client_connection(vbds_client_conn);

	rc = loc_server_register(NAME);
	if (rc != EOK) {
		log_msg(LOG_DEFAULT, LVL_ERROR, "Failed registering server (%d).", rc);
		return EEXIST;
	}

	rc = loc_service_register(SERVICE_NAME_VBD, &ctl_sid);
	if (rc != EOK) {
		log_msg(LOG_DEFAULT, LVL_ERROR, "Failed registering service (%d).", rc);
		return EEXIST;
	}

	return EOK;
}


static void vbds_disk_add_srv(ipc_callid_t iid, ipc_call_t *icall)
{
	service_id_t disk_sid;
	int rc;

	log_msg(LOG_DEFAULT, LVL_NOTE, "vbds_disk_add_srv()");

	disk_sid = IPC_GET_ARG1(*icall);
	rc = vbds_disk_add(disk_sid);
	async_answer_0(iid, (sysarg_t) rc);
}

static void vbds_disk_remove_srv(ipc_callid_t iid, ipc_call_t *icall)
{
	service_id_t disk_sid;
	int rc;

	log_msg(LOG_DEFAULT, LVL_NOTE, "vbds_disk_remove_srv()");

	disk_sid = IPC_GET_ARG1(*icall);
	rc = vbds_disk_remove(disk_sid);
	async_answer_0(iid, (sysarg_t) rc);
}

static void vbds_disk_info_srv(ipc_callid_t iid, ipc_call_t *icall)
{
	service_id_t disk_sid;
	vbds_disk_info_t dinfo;
	int rc;

	log_msg(LOG_DEFAULT, LVL_NOTE, "vbds_disk_info_srv()");

	disk_sid = IPC_GET_ARG1(*icall);
	rc = vbds_disk_info(disk_sid, &dinfo);
	async_answer_1(iid, (sysarg_t)rc, (sysarg_t)dinfo.ltype);
}

static void vbds_label_create_srv(ipc_callid_t iid, ipc_call_t *icall)
{
	service_id_t disk_sid;
	label_type_t ltype;
	int rc;

	log_msg(LOG_DEFAULT, LVL_NOTE, "vbds_label_create_srv()");

	disk_sid = IPC_GET_ARG1(*icall);
	ltype = IPC_GET_ARG2(*icall);
	rc = vbds_label_create(disk_sid, ltype);
	async_answer_0(iid, (sysarg_t) rc);
}

static void vbds_label_delete_srv(ipc_callid_t iid, ipc_call_t *icall)
{
	service_id_t disk_sid;
	int rc;

	log_msg(LOG_DEFAULT, LVL_NOTE, "vbds_label_delete_srv()");

	disk_sid = IPC_GET_ARG1(*icall);
	rc = vbds_label_delete(disk_sid);
	async_answer_0(iid, (sysarg_t) rc);
}

static void vbds_label_get_parts_srv(ipc_callid_t iid, ipc_call_t *icall)
{
//	service_id_t disk_sid;
//	int rc;

	log_msg(LOG_DEFAULT, LVL_NOTE, "vbds_label_get_parts_srv()");

//	disk_sid = IPC_GET_ARG1(*icall);
//	rc = vbds_label_delete(disk_sid);
//	async_answer_0(iid, (sysarg_t) rc);
	async_answer_0(iid, ENOTSUP);
}

static void vbds_part_get_info_srv(ipc_callid_t iid, ipc_call_t *icall)
{
	vbds_part_id_t part;
	vbds_part_info_t pinfo;
	int rc;

	log_msg(LOG_DEFAULT, LVL_NOTE, "vbds_part_get_info_srv()");

	part = IPC_GET_ARG1(*icall);
	rc = vbds_part_get_info(part, &pinfo);
	async_answer_0(iid, (sysarg_t)rc);
}

static void vbds_part_create_srv(ipc_callid_t iid, ipc_call_t *icall)
{
	service_id_t disk_sid;
	vbds_part_id_t part;
	int rc;

	log_msg(LOG_DEFAULT, LVL_NOTE, "vbds_part_create_srv()");

	disk_sid = IPC_GET_ARG1(*icall);
	rc = vbds_part_create(disk_sid, &part);
	async_answer_1(iid, (sysarg_t)rc, (sysarg_t)part);
}

static void vbds_part_delete_srv(ipc_callid_t iid, ipc_call_t *icall)
{
	vbds_part_id_t part;
	int rc;

	log_msg(LOG_DEFAULT, LVL_NOTE, "vbds_part_delete_srv()");

	part = IPC_GET_ARG1(*icall);
	rc = vbds_part_delete(part);
	async_answer_0(iid, (sysarg_t) rc);
}

static void vbds_ctl_conn(ipc_callid_t iid, ipc_call_t *icall, void *arg)
{
	log_msg(LOG_DEFAULT, LVL_NOTE, "vbds_client_conn()");

	/* Accept the connection */
	async_answer_0(iid, EOK);

	while (true) {
		ipc_call_t call;
		ipc_callid_t callid = async_get_call(&call);
		sysarg_t method = IPC_GET_IMETHOD(call);

		if (!method) {
			/* The other side has hung up */
			async_answer_0(callid, EOK);
			return;
		}

		switch (method) {
		case VBD_DISK_ADD:
			vbds_disk_add_srv(callid, &call);
			break;
		case VBD_DISK_REMOVE:
			vbds_disk_remove_srv(callid, &call);
			break;
		case VBD_DISK_INFO:
			vbds_disk_info_srv(callid, &call);
			break;
		case VBD_LABEL_CREATE:
			vbds_label_create_srv(callid, &call);
			break;
		case VBD_LABEL_DELETE:
			vbds_label_delete_srv(callid, &call);
			break;
		case VBD_LABEL_GET_PARTS:
			vbds_label_get_parts_srv(callid, &call);
			break;
		case VBD_PART_GET_INFO:
			vbds_part_get_info_srv(callid, &call);
			break;
		case VBD_PART_CREATE:
			vbds_part_create_srv(callid, &call);
			break;
		case VBD_PART_DELETE:
			vbds_part_delete_srv(callid, &call);
			break;
		default:
			async_answer_0(callid, EINVAL);
		}
	}
}

static void vbds_client_conn(ipc_callid_t iid, ipc_call_t *icall, void *arg)
{
	service_id_t sid;

	log_msg(LOG_DEFAULT, LVL_NOTE, "vbds_client_conn()");

	sid = (service_id_t)IPC_GET_ARG1(*icall);

	if (sid == ctl_sid)
		vbds_ctl_conn(iid, icall, arg);
	else
		vbds_bd_conn(iid, icall, arg);
}

int main(int argc, char *argv[])
{
	int rc;

	printf("%s: Virtual Block Device service\n", NAME);

	if (log_init(NAME) != EOK) {
		printf(NAME ": Failed to initialize logging.\n");
		return 1;
	}

	rc = vbds_init();
	if (rc != EOK)
		return 1;

	printf(NAME ": Accepting connections.\n");
	task_retval(0);
	async_manager();

	return 0;
}

/** @}
 */
