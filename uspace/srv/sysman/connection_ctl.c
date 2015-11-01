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
#include <stdlib.h>

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
		default:
			async_answer_0(callid, ENOENT);
		}
	}
}

