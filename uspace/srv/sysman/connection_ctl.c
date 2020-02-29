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

#include "repo.h"
#include "connection_ctl.h"
#include "job.h"
#include "job_closure.h"
#include "log.h"
#include "shutdown.h"
#include "sysman.h"

// TODO possibly provide as type-safe function + macro in sysman.h for generic boxing
static ipc_call_t *box_callid(ipc_call_t *icall)
{
	ipc_call_t *copy = malloc(sizeof(ipc_call_t));
	if (copy) {
		memcpy(copy, icall, sizeof(ipc_call_t));
	}
	return copy;
}

static void answer_callback(void *object, void *arg)
{
	job_t *job = object;
	assert(job->state == JOB_FINISHED);
	assert(job->retval != JOB_UNDEFINED_);

	ipc_call_t *icall = arg;
	// TODO use descriptive return value (probably refactor job retval)
	sysarg_t retval = (job->retval == JOB_OK) ? EOK : EIO;
	async_answer_0(icall, retval);
	free(icall);
	job_del_ref(&job);
}

static void sysman_unit_handle(ipc_call_t *icall)
{
	char *unit_name = NULL;
	sysarg_t retval;

	errno_t rc = async_data_write_accept((void **) &unit_name, true,
	    0, 0, 0, NULL);
	if (rc != EOK) {
		retval = rc;
		goto fail;
	}

	unit_t *unit = repo_find_unit_by_name(unit_name);
	if (unit == NULL) {
		retval = ENOENT;
		goto fail;
	}

	async_answer_1(icall, EOK, unit->handle);
	goto finish;

fail:
	async_answer_0(icall, retval);
finish:
	free(unit_name);
}

static void sysman_unit_start_by_name(ipc_call_t *icall)
{
	char *unit_name = NULL;
	sysarg_t retval;

	errno_t rc = async_data_write_accept((void **) &unit_name, true,
	    0, 0, 0, NULL);
	if (rc != EOK) {
		retval = rc;
		goto answer;
	}

	ipc_start_flag_t flags = ipc_get_arg1(icall);
	sysman_log(LVL_DEBUG2, "%s(%s, %x)", __func__, unit_name, flags);

	unit_t *unit = repo_find_unit_by_name(unit_name);
	if (unit == NULL) {
		sysman_log(LVL_NOTE, "Unit '%s' not found.", unit_name);
		retval = ENOENT;
		goto answer;
	}

	if (!(flags & IPC_FLAG_BLOCKING)) {
		retval = sysman_run_job(unit, STATE_STARTED, 0, NULL, NULL);
		goto answer;
	}

	ipc_call_t *icall_copy = box_callid(icall);
	if (icall_copy == NULL) {
		retval = ENOMEM;
		goto answer;
	}
	retval = sysman_run_job(unit, STATE_STARTED, 0, &answer_callback,
	    icall_copy);
	if (retval != EOK) {
		goto answer;
	}

	/* Answer asynchronously from callback */
	goto finish;

answer:
	async_answer_0(icall, retval);
finish:
	free(unit_name);
}

static void sysman_unit_operation(ipc_call_t *icall, unit_state_t state)
{
	sysarg_t retval;

	unit_handle_t handle = ipc_get_arg1(icall);
	ipc_start_flag_t flags = ipc_get_arg2(icall);
	sysman_log(LVL_DEBUG2, "%s(%p, %u, %i)", __func__, icall->cap_handle, flags, state);

	unit_t *unit = repo_find_unit_by_handle(handle);
	if (unit == NULL) {
		retval = ENOENT;
		goto answer;
	}

	if (!(flags & IPC_FLAG_BLOCKING)) {
		retval = sysman_run_job(unit, state, 0, NULL, NULL);
		goto answer;
	}

	ipc_call_t *icall_copy = box_callid(icall);
	if (icall_copy == NULL) {
		retval = ENOMEM;
		goto answer;
	}
	retval = sysman_run_job(unit, state, 0, &answer_callback,
	    icall_copy);
	if (retval != EOK) {
		goto answer;
	}

	/* Answer asynchronously from callback */
	return;

answer:
	async_answer_0(icall, retval);
}

static void sysman_unit_start(ipc_call_t *icall)
{
	sysman_unit_operation(icall, STATE_STARTED);
}

static void sysman_unit_stop(ipc_call_t *icall)
{
	sysman_unit_operation(icall, STATE_STOPPED);
}

static errno_t fill_handles_buffer(unit_handle_t *buffer, size_t size,
    size_t *act_size)
{
	if (size % sizeof(unit_handle_t) != 0) {
		return EINVAL;
	}

	size_t filled = 0;
	size_t to_fill = size / sizeof(unit_handle_t);
	size_t total = 0;
	repo_rlock();
	repo_foreach(u) {
		if (filled < to_fill) {
			buffer[filled++] = u->handle;
		}
		++total;
	}
	repo_runlock();
	*act_size = total * sizeof(unit_handle_t);
	return EOK;
}

static void sysman_get_units(ipc_call_t *icall)
{
	ipc_call_t call;
	size_t size;
	size_t act_size;
	errno_t rc;

	if (!async_data_read_receive(&call, &size)) {
		async_answer_0(&call, EREFUSED);
		async_answer_0(icall, EREFUSED);
		return;
	}

	unit_handle_t *handles = malloc(size);
	if (handles == NULL && size > 0) {
		async_answer_0(&call, ENOMEM);
		async_answer_0(icall, ENOMEM);
		return;
	}

	rc = fill_handles_buffer(handles, size, &act_size);
	if (rc != EOK) {
		async_answer_0(&call, rc);
		async_answer_0(icall, rc);
		return;
	}

	size_t real_size = min(act_size, size);
	sysarg_t retval = async_data_read_finalize(&call, handles, real_size);
	free(handles);

	async_answer_1(icall, retval, act_size);
}

static void sysman_unit_get_name(ipc_call_t *icall)
{
	ipc_call_t call;
	size_t size;

	if (!async_data_read_receive(&call, &size)) {
		async_answer_0(&call, EREFUSED);
		async_answer_0(icall, EREFUSED);
		return;
	}

	unit_t *u = repo_find_unit_by_handle(ipc_get_arg1(icall));
	if (u == NULL) {
		async_answer_0(&call, ENOENT);
		async_answer_0(icall, ENOENT);
		return;
	}

	size_t real_size = min(str_size(u->name) + 1, size);
	sysarg_t retval = async_data_read_finalize(&call, u->name, real_size);

	async_answer_0(icall, retval);
}

static void sysman_unit_get_state(ipc_call_t *icall)
{
	unit_t *u = repo_find_unit_by_handle(ipc_get_arg1(icall));
	if (u == NULL) {
		async_answer_0(icall, ENOENT);
	} else {
		async_answer_1(icall, EOK, u->state);
	}
}

static void sysman_shutdown(ipc_call_t *icall)
{
	errno_t retval;
	unit_t *u = repo_find_unit_by_name(TARGET_SHUTDOWN);
	if (u == NULL) {
		retval = ENOENT;
		goto finish;
	}

	retval = sysman_run_job(u, STATE_STARTED, CLOSURE_ISOLATE,
	    shutdown_cb, NULL);

finish:
	async_answer_0(icall, retval);
}

void sysman_connection_ctl(ipc_call_t *icall)
{
	sysman_log(LVL_DEBUG2, "%s", __func__);

	while (true) {
		ipc_call_t call;

		if (!async_get_call(&call) || !ipc_get_imethod(&call)) {
			/* Client disconnected */
			break;
		}

		switch (ipc_get_imethod(&call)) {
		case SYSMAN_CTL_UNIT_HANDLE:
			sysman_unit_handle(&call);
			break;
		case SYSMAN_CTL_UNIT_START_BY_NAME:
			sysman_unit_start_by_name(&call);
			break;
		case SYSMAN_CTL_UNIT_START:
			sysman_unit_start(&call);
			break;
		case SYSMAN_CTL_UNIT_STOP:
			sysman_unit_stop(&call);
			break;
		case SYSMAN_CTL_GET_UNITS:
			sysman_get_units(&call);
			break;
		case SYSMAN_CTL_UNIT_GET_NAME:
			sysman_unit_get_name(&call);
			break;
		case SYSMAN_CTL_UNIT_GET_STATE:
			sysman_unit_get_state(&call);
			break;
		case SYSMAN_CTL_SHUTDOWN:
			sysman_shutdown(&call);
			break;
		default:
			async_answer_0(&call, ENOENT);
		}
	}
}
