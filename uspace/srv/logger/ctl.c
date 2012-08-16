/*
 * Copyright (c) 2012 Vojtech Horky
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

/**
 * @addtogroup logger
 * @{
 */

/** @file
 */

#include <unistd.h>
#include <malloc.h>
#include <stdio.h>
#include <errno.h>
#include <io/logctl.h>
#include <ipc/logger.h>
#include "logger.h"

static int handle_toplog_level_change(sysarg_t new_level)
{
	void *top_name;
	int rc = async_data_write_accept(&top_name, true, 0, 0, 0, NULL);
	if (rc != EOK) {
		return rc;
	}

	logger_toplevel_log_t *toplog = find_or_create_toplevel_log(top_name);
	free(top_name);
	if (toplog == NULL)
		return ENOENT;

	toplog->logged_level = new_level;

	return EOK;
}

static int handle_log_level_change(sysarg_t new_level)
{
	void *top_name;
	int rc = async_data_write_accept(&top_name, true, 0, 0, 0, NULL);
	if (rc != EOK) {
		return rc;
	}

	logger_toplevel_log_t *toplog = find_or_create_toplevel_log(top_name);
	free(top_name);
	if (toplog == NULL)
		return ENOENT;


	void *log_name;
	rc = async_data_write_accept(&log_name, true, 0, 0, 0, NULL);
	if (rc != EOK)
		return rc;

	rc = ENOENT;
	for (size_t i = 0; i < toplog->sublog_count; i++) {
		if (str_cmp(toplog->sublogs[i].name, (const char *) log_name) == 0) {
			toplog->sublogs[i].logged_level = new_level;
			rc = EOK;
			break;
		}
	}

	free(log_name);

	return rc;
}

void logger_connection_handler_control(ipc_callid_t callid)
{
	async_answer_0(callid, EOK);
	printf(NAME "/control: new client.\n");

	while (true) {
		ipc_call_t call;
		ipc_callid_t callid = async_get_call(&call);

		if (!IPC_GET_IMETHOD(call))
			break;

		switch (IPC_GET_IMETHOD(call)) {
		case LOGGER_CTL_SET_DEFAULT_LEVEL: {
			int rc = set_default_logging_level(IPC_GET_ARG1(call));
			async_answer_0(callid, rc);
			break;
		}
		case LOGGER_CTL_SET_TOP_LOG_LEVEL: {
			int rc = handle_toplog_level_change(IPC_GET_ARG1(call));
			async_answer_0(callid, rc);
			break;
		}
		case LOGGER_CTL_SET_LOG_LEVEL: {
			int rc = handle_log_level_change(IPC_GET_ARG1(call));
			async_answer_0(callid, rc);
			break;
		}
		default:
			async_answer_0(callid, EINVAL);
			break;
		}
	}

	printf(NAME "/control: client terminated.\n");
}


/**
 * @}
 */
