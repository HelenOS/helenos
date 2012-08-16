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

static int handle_namespace_level_change(sysarg_t new_level)
{
	void *namespace_name;
	int rc = async_data_write_accept(&namespace_name, true, 0, 0, 0, NULL);
	if (rc != EOK) {
		return rc;
	}

	logging_namespace_t *namespace = namespace_writer_attach((const char *) namespace_name);
	free(namespace_name);
	if (namespace == NULL)
		return ENOENT;

	rc = namespace_change_level(namespace, (log_level_t) new_level);
	namespace_writer_detach(namespace);

	return rc;
}

static int handle_context_level_change(sysarg_t new_level)
{
	void *namespace_name;
	int rc = async_data_write_accept(&namespace_name, true, 0, 0, 0, NULL);
	if (rc != EOK) {
		return rc;
	}

	logging_namespace_t *namespace = namespace_writer_attach((const char *) namespace_name);
	free(namespace_name);
	if (namespace == NULL)
		return ENOENT;

	void *context_name;
	rc = async_data_write_accept(&context_name, true, 0, 0, 0, NULL);
	if (rc != EOK) {
		namespace_writer_detach(namespace);
		return rc;
	}

	rc = namespace_change_context_level(namespace, context_name, new_level);
	free(context_name);
	namespace_writer_detach(namespace);

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

		int rc;

		switch (IPC_GET_IMETHOD(call)) {
		case LOGGER_CTL_GET_DEFAULT_LEVEL:
			async_answer_1(callid, EOK, get_default_logging_level());
			break;
		case LOGGER_CTL_SET_DEFAULT_LEVEL:
			rc = set_default_logging_level(IPC_GET_ARG1(call));
			async_answer_0(callid, rc);
			break;
		case LOGGER_CTL_SET_NAMESPACE_LEVEL:
			rc = handle_namespace_level_change(IPC_GET_ARG1(call));
			async_answer_0(callid, rc);
			break;
		case LOGGER_CTL_SET_CONTEXT_LEVEL:
			rc = handle_context_level_change(IPC_GET_ARG1(call));
			async_answer_0(callid, rc);
			break;
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
