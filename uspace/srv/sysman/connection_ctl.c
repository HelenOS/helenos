/*
 * Copyright (c) 2015 Michal Koutny
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AS IS'' AND ANY EXPRESS OR
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

#include <errno.h>
#include <ipc/sysman.h>
#include <macros.h>
#include <stdlib.h>
#include <str.h>

#include "configuration.h"
#include "connection_ctl.h"
#include "job.h"
#include "log.h"
#include "sysman.h"


// TODO possibly provide as type-safe function + macro in sysman.h for generic boxing
static ipc_callid_t *box_callid(ipc_callid_t iid)
{
	ipc_callid_t *result = malloc(sizeof(ipc_callid_t));
	if (result) {
		*result = iid;
	}
	return result;
}

static void answer_callback(void *object, void *arg)
{
	job_t *job = object;
	assert(job->state == JOB_FINISHED);
	assert(job->retval != JOB_UNDEFINED_);

	ipc_callid_t *iid_ptr = arg;
	// TODO use descriptive return value (probably refactor job retval)
	sysarg_t retval = (job->retval == JOB_OK) ? EOK : EIO;
	async_answer_0(*iid_ptr, retval);
	free(iid_ptr);
	job_del_ref(&job);
}
static void sysman_unit_start(ipc_callid_t iid, ipc_call_t *icall)
{
	char *unit_name = NULL;
	sysarg_t retval;

	int rc = async_data_write_accept((void **) &unit_name, true,
	    0, 0, 0, NULL);
	if (rc != EOK) {
		retval = rc;
		goto answer;
	}

	int flags = IPC_GET_ARG1(*icall);
	sysman_log(LVL_DEBUG2, "%s(%s, %x)", __func__, unit_name, flags);

	// TODO this is connection fibril, UNSYNCHRONIZED access to units!
	unit_t *unit = configuration_find_unit_by_name(unit_name);
	if (unit == NULL) {
		sysman_log(LVL_NOTE, "Unit '%s' not found.", unit_name);
		retval = ENOENT;
		goto answer;
	}

	if (!(flags & IPC_FLAG_BLOCKING)) {
		retval = sysman_run_job(unit, STATE_STARTED, NULL, NULL);
		goto answer;
	}

	ipc_callid_t *iid_ptr = box_callid(iid);
	if (iid_ptr == NULL) {
		retval = ENOMEM;
		goto answer;
	}
	retval = sysman_run_job(unit, STATE_STARTED, &answer_callback,
	    iid_ptr);
	if (retval != EOK) {
		goto answer;
	}

	/* Answer asynchronously from callback */
	goto finish;

answer:
	async_answer_0(iid, retval);
finish:
	free(unit_name);
}

static int fill_handles_buffer(unit_handle_t *buffer, size_t size,
    size_t *act_size)
{
	if (size % sizeof(unit_handle_t) != 0) {
		return EINVAL;
	}

	size_t filled = 0;
	size_t to_fill = size / sizeof(unit_handle_t);
	size_t total = 0;
	list_foreach(units, units, unit_t, u) {
		if (filled < to_fill) {
			buffer[filled++] = u->handle;
		}
		++total;
	}
	*act_size = total * sizeof(unit_handle_t);
	return EOK;
}

static void sysman_get_units(ipc_callid_t iid, ipc_call_t *icall)
{
	ipc_callid_t callid;
	size_t size;
	size_t act_size;
	int rc;
	
	if (!async_data_read_receive(&callid, &size)) {
		async_answer_0(callid, EREFUSED);
		async_answer_0(iid, EREFUSED);
		return;
	}
	
	
	unit_handle_t *handles = malloc(size);
	if (handles == NULL && size > 0) {
		async_answer_0(callid, ENOMEM);
		async_answer_0(iid, ENOMEM);
		return;
	}
	
	
	// TODO UNSYNCHRONIZED access to units!
	rc = fill_handles_buffer(handles, size, &act_size);
	if (rc != EOK) {
		async_answer_0(callid, rc);
		async_answer_0(iid, rc);
		return;
	}
	
	size_t real_size = min(act_size, size);
	sysarg_t retval = async_data_read_finalize(callid, handles, real_size);
	free(handles);
	
	async_answer_1(iid, retval, act_size);
}

static void sysman_unit_get_name(ipc_callid_t iid, ipc_call_t *icall)
{
	ipc_callid_t callid;
	size_t size;
	
	if (!async_data_read_receive(&callid, &size)) {
		async_answer_0(callid, EREFUSED);
		async_answer_0(iid, EREFUSED);
		return;
	}
	
	// TODO UNSYNCHRONIZED access to units!
	unit_t *u = configuration_find_unit_by_handle(IPC_GET_ARG1(*icall));
	if (u == NULL) {
		async_answer_0(callid, ENOENT);
		async_answer_0(iid, ENOENT);
		return;
	}
	
	size_t real_size = min(str_size(u->name) + 1, size);
	sysarg_t retval = async_data_read_finalize(callid, u->name, real_size);
	
	async_answer_0(iid, retval);
}

static void sysman_unit_get_state(ipc_callid_t iid, ipc_call_t *icall)
{
	// TODO UNSYNCHRONIZED access to units!
	unit_t *u = configuration_find_unit_by_handle(IPC_GET_ARG1(*icall));
	if (u == NULL) {
		async_answer_0(iid, ENOENT);
	} else {
		async_answer_1(iid, EOK, u->state);
	}
}

void sysman_connection_ctl(ipc_callid_t iid, ipc_call_t *icall)
{
	sysman_log(LVL_DEBUG2, "%s", __func__);
	/* First, accept connection */
	async_answer_0(iid, EOK);

	while (true) {
		ipc_call_t call;
		ipc_callid_t callid = async_get_call(&call);

		if (!IPC_GET_IMETHOD(call)) {
			/* Client disconnected */
			break;
		}

		switch (IPC_GET_IMETHOD(call)) {
		case SYSMAN_CTL_UNIT_START:
			sysman_unit_start(callid, &call);
			break;
		case SYSMAN_CTL_GET_UNITS:
			sysman_get_units(callid, &call);
			break;
		case SYSMAN_CTL_UNIT_GET_NAME:
			sysman_unit_get_name(callid, &call);
			break;
		case SYSMAN_CTL_UNIT_GET_STATE:
			sysman_unit_get_state(callid, &call);
			break;
		default:
			async_answer_0(callid, ENOENT);
		}
	}
}

