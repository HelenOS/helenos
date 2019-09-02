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

#include "repo.h"
#include "connection_broker.h"
#include "log.h"
#include "sysman.h"

static void sysman_broker_register(ipc_call_t *icall)
{
	sysman_log(LVL_DEBUG2, "%s", __func__);
	async_answer_0(icall, EOK);
	/*
	 *  What exactly do here? Similar behavior that has locsrv with
	 *  servers, so that subsequent calls can be assigned to broker. Still
	 *  that makes sense only when brokers will somehow scope unit/exposee
	 *  names. Why I wanted this registration?
	 */
}

static void sysman_ipc_forwarded(ipc_call_t *icall)
{
	sysman_log(LVL_DEBUG2, "%s", __func__);
	async_answer_0(icall, ENOTSUP);
	// TODO implement
}

static void sysman_main_exposee_added(ipc_call_t *icall)
{
	char *unit_name = NULL;
	sysarg_t retval;

	int rc = async_data_write_accept((void **) &unit_name, true,
	    0, 0, 0, NULL);
	if (rc != EOK) {
		retval = rc;
		goto finish;
	}

	unit_t *unit = repo_find_unit_by_name(unit_name);
	if (unit == NULL) {
		retval = ENOENT;
		goto finish;
	}

	// TODO propagate caller task ID
	sysman_raise_event(&sysman_event_unit_exposee_created, unit);

	retval = EOK;

finish:
	async_answer_0(icall, retval);
	free(unit_name);
}

static void sysman_exposee_added(ipc_call_t *icall)
{
	char *exposee = NULL;
	sysarg_t retval;

	/* Just accept data and further not supported. */
	int rc = async_data_write_accept((void **) &exposee, true,
	    0, 0, 0, NULL);
	if (rc != EOK) {
		retval = rc;
		goto finish;
	}

	retval = ENOTSUP;

finish:
	async_answer_0(icall, retval);
	free(exposee);
}

static void sysman_exposee_removed(ipc_call_t *icall)
{
	sysman_log(LVL_DEBUG2, "%s", __func__);
	async_answer_0(icall, ENOTSUP);
	// TODO implement
}

void sysman_connection_broker(ipc_call_t *icall)
{
	sysman_log(LVL_DEBUG2, "%s", __func__);
	/* First, accept connection */
	async_answer_0(icall, EOK);

	while (true) {
		ipc_call_t call;

		if (!async_get_call(&call) || !ipc_get_imethod(&call)) {
			/* Client disconnected */
			break;
		}

		switch (ipc_get_imethod(&call)) {
		case SYSMAN_BROKER_REGISTER:
			sysman_broker_register(&call);
			break;
		case SYSMAN_BROKER_IPC_FWD:
			sysman_ipc_forwarded(&call);
			break;
		case SYSMAN_BROKER_MAIN_EXP_ADDED:
			sysman_main_exposee_added(&call);
			break;
		case SYSMAN_BROKER_EXP_ADDED:
			sysman_exposee_added(&call);
			break;
		case SYSMAN_BROKER_EXP_REMOVED:
			sysman_exposee_removed(&call);
			break;
		default:
			async_answer_0(&call, ENOENT);
		}
	}
}
