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
#include <str_error.h>
#include <io/log.h>
#include <ipc/services.h>
#include <ipc/vbd.h>
#include <label/label.h>
#include <loc.h>
#include <macros.h>
#include <stdio.h>
#include <stdlib.h>
#include <task.h>
#include <vbd.h>

#include "disk.h"
#include "types/vbd.h"

#define NAME  "vbd"

static void vbds_client_conn(ipc_callid_t, ipc_call_t *, void *);

static service_id_t ctl_sid;

static errno_t vbds_init(void)
{
	errno_t rc;
	log_msg(LOG_DEFAULT, LVL_DEBUG, "vbds_init()");

	rc = vbds_disks_init();
	if (rc != EOK)
		return rc;

	async_set_fallback_port_handler(vbds_client_conn, NULL);

	rc = loc_server_register(NAME);
	if (rc != EOK) {
		log_msg(LOG_DEFAULT, LVL_ERROR, "Failed registering server: %s.", str_error(rc));
		return EEXIST;
	}

	rc = loc_service_register(SERVICE_NAME_VBD, &ctl_sid);
	if (rc != EOK) {
		log_msg(LOG_DEFAULT, LVL_ERROR, "Failed registering service: %s.", str_error(rc));
		return EEXIST;
	}

	rc = vbds_disk_discovery_start();
	if (rc != EOK)
		return rc;

	return EOK;
}

static void vbds_get_disks_srv(ipc_callid_t iid, ipc_call_t *icall)
{
	ipc_callid_t callid;
	size_t size;
	size_t act_size;
	errno_t rc;

	if (!async_data_read_receive(&callid, &size)) {
		async_answer_0(callid, EREFUSED);
		async_answer_0(iid, EREFUSED);
		return;
	}

	service_id_t *id_buf = (service_id_t *) malloc(size);
	if (id_buf == NULL) {
		async_answer_0(callid, ENOMEM);
		async_answer_0(iid, ENOMEM);
		return;
	}

	rc = vbds_disk_get_ids(id_buf, size, &act_size);
	if (rc != EOK) {
		free(id_buf);
		async_answer_0(callid, rc);
		async_answer_0(iid, rc);
		return;
	}

	errno_t retval = async_data_read_finalize(callid, id_buf, size);
	free(id_buf);

	async_answer_1(iid, retval, act_size);
}

static void vbds_disk_info_srv(ipc_callid_t iid, ipc_call_t *icall)
{
	service_id_t disk_sid;
	vbd_disk_info_t dinfo;
	errno_t rc;

	log_msg(LOG_DEFAULT, LVL_DEBUG, "vbds_disk_info_srv()");

	disk_sid = IPC_GET_ARG1(*icall);
	rc = vbds_disk_info(disk_sid, &dinfo);
	if (rc != EOK) {
		async_answer_0(iid, rc);
		return;
	}

	ipc_callid_t callid;
	size_t size;
	if (!async_data_read_receive(&callid, &size)) {
		async_answer_0(callid, EREFUSED);
		async_answer_0(iid, EREFUSED);
		return;
	}

	if (size != sizeof(vbd_disk_info_t)) {
		async_answer_0(callid, EINVAL);
		async_answer_0(iid, EINVAL);
		return;
	}

	rc = async_data_read_finalize(callid, &dinfo,
	    min(size, sizeof(dinfo)));
	if (rc != EOK) {
		async_answer_0(callid, rc);
		async_answer_0(iid, rc);
		return;
	}

	async_answer_0(iid, EOK);
}

static void vbds_label_create_srv(ipc_callid_t iid, ipc_call_t *icall)
{
	service_id_t disk_sid;
	label_type_t ltype;
	errno_t rc;

	log_msg(LOG_DEFAULT, LVL_DEBUG, "vbds_label_create_srv()");

	disk_sid = IPC_GET_ARG1(*icall);
	ltype = IPC_GET_ARG2(*icall);
	rc = vbds_label_create(disk_sid, ltype);
	async_answer_0(iid, rc);
}

static void vbds_label_delete_srv(ipc_callid_t iid, ipc_call_t *icall)
{
	service_id_t disk_sid;
	errno_t rc;

	log_msg(LOG_DEFAULT, LVL_DEBUG, "vbds_label_delete_srv()");

	disk_sid = IPC_GET_ARG1(*icall);
	rc = vbds_label_delete(disk_sid);
	async_answer_0(iid, rc);
}

static void vbds_label_get_parts_srv(ipc_callid_t iid, ipc_call_t *icall)
{
	ipc_callid_t callid;
	size_t size;
	size_t act_size;
	service_id_t sid;
	errno_t rc;

	log_msg(LOG_DEFAULT, LVL_DEBUG, "vbds_label_get_parts_srv()");

	if (!async_data_read_receive(&callid, &size)) {
		async_answer_0(callid, EREFUSED);
		async_answer_0(iid, EREFUSED);
		return;
	}

	sid = IPC_GET_ARG1(*icall);

	category_id_t *id_buf = (category_id_t *) malloc(size);
	if (id_buf == NULL) {
		async_answer_0(callid, ENOMEM);
		async_answer_0(iid, ENOMEM);
		return;
	}

	rc = vbds_get_parts(sid, id_buf, size, &act_size);
	if (rc != EOK) {
		async_answer_0(callid, rc);
		async_answer_0(iid, rc);
		return;
	}

	errno_t retval = async_data_read_finalize(callid, id_buf, size);
	free(id_buf);

	async_answer_1(iid, retval, act_size);
}

static void vbds_part_get_info_srv(ipc_callid_t iid, ipc_call_t *icall)
{
	vbds_part_id_t part;
	vbd_part_info_t pinfo;
	errno_t rc;

	log_msg(LOG_DEFAULT, LVL_DEBUG, "vbds_part_get_info_srv()");

	part = IPC_GET_ARG1(*icall);
	rc = vbds_part_get_info(part, &pinfo);
	if (rc != EOK) {
		async_answer_0(iid, rc);
		return;
	}

	ipc_callid_t callid;
	size_t size;
	if (!async_data_read_receive(&callid, &size)) {
		async_answer_0(callid, EREFUSED);
		async_answer_0(iid, EREFUSED);
		return;
	}

	if (size != sizeof(vbd_part_info_t)) {
		async_answer_0(callid, EINVAL);
		async_answer_0(iid, EINVAL);
		return;
	}

	rc = async_data_read_finalize(callid, &pinfo,
	    min(size, sizeof(pinfo)));
	if (rc != EOK) {
		async_answer_0(callid, rc);
		async_answer_0(iid, rc);
		return;
	}

	async_answer_0(iid, EOK);
}

static void vbds_part_create_srv(ipc_callid_t iid, ipc_call_t *icall)
{
	service_id_t disk_sid;
	vbd_part_spec_t pspec;
	vbds_part_id_t part;
	errno_t rc;

	log_msg(LOG_DEFAULT, LVL_DEBUG, "vbds_part_create_srv()");

	disk_sid = IPC_GET_ARG1(*icall);

	ipc_callid_t callid;
	size_t size;
	if (!async_data_write_receive(&callid, &size)) {
		async_answer_0(callid, EREFUSED);
		async_answer_0(iid, EREFUSED);
		return;
	}

	if (size != sizeof(vbd_part_spec_t)) {
		async_answer_0(callid, EINVAL);
		async_answer_0(iid, EINVAL);
		return;
	}

	rc = async_data_write_finalize(callid, &pspec, sizeof(vbd_part_spec_t));
	if (rc != EOK) {
		async_answer_0(callid, rc);
		async_answer_0(iid, rc);
		return;
	}

	rc = vbds_part_create(disk_sid, &pspec, &part);
	if (rc != EOK) {
		async_answer_0(iid, rc);
		return;
	}

	async_answer_1(iid, rc, (sysarg_t)part);
}

static void vbds_part_delete_srv(ipc_callid_t iid, ipc_call_t *icall)
{
	vbds_part_id_t part;
	errno_t rc;

	log_msg(LOG_DEFAULT, LVL_DEBUG, "vbds_part_delete_srv()");

	part = IPC_GET_ARG1(*icall);
	rc = vbds_part_delete(part);
	async_answer_0(iid, rc);
}

static void vbds_suggest_ptype_srv(ipc_callid_t iid, ipc_call_t *icall)
{
	service_id_t disk_sid;
	label_ptype_t ptype;
	label_pcnt_t pcnt;
	errno_t rc;

	log_msg(LOG_DEFAULT, LVL_DEBUG, "vbds_suggest_ptype_srv()");

	disk_sid = IPC_GET_ARG1(*icall);
	pcnt = IPC_GET_ARG2(*icall);

	rc = vbds_suggest_ptype(disk_sid, pcnt, &ptype);
	if (rc != EOK) {
		async_answer_0(iid, rc);
		return;
	}

	ipc_callid_t callid;
	size_t size;
	if (!async_data_read_receive(&callid, &size)) {
		async_answer_0(callid, EREFUSED);
		async_answer_0(iid, EREFUSED);
		return;
	}

	if (size != sizeof(label_ptype_t)) {
		async_answer_0(callid, EINVAL);
		async_answer_0(iid, EINVAL);
		return;
	}

	rc = async_data_read_finalize(callid, &ptype, sizeof(label_ptype_t));
	if (rc != EOK) {
		async_answer_0(callid, rc);
		async_answer_0(iid, rc);
		return;
	}

	async_answer_0(iid, EOK);
}

static void vbds_ctl_conn(ipc_callid_t iid, ipc_call_t *icall, void *arg)
{
	log_msg(LOG_DEFAULT, LVL_DEBUG, "vbds_client_conn()");

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
		case VBD_GET_DISKS:
			vbds_get_disks_srv(callid, &call);
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
		case VBD_SUGGEST_PTYPE:
			vbds_suggest_ptype_srv(callid, &call);
			break;
		default:
			async_answer_0(callid, EINVAL);
		}
	}
}

static void vbds_client_conn(ipc_callid_t iid, ipc_call_t *icall, void *arg)
{
	service_id_t sid;

	log_msg(LOG_DEFAULT, LVL_DEBUG, "vbds_client_conn()");

	sid = (service_id_t)IPC_GET_ARG2(*icall);

	if (sid == ctl_sid)
		vbds_ctl_conn(iid, icall, arg);
	else
		vbds_bd_conn(iid, icall, arg);
}

int main(int argc, char *argv[])
{
	errno_t rc;

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
