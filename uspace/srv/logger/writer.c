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
 * @defgroup logger Logging service.
 * @brief HelenOS logging service.
 * @{
 */

/** @file
 */

#include <ipc/services.h>
#include <ipc/logger.h>
#include <io/log.h>
#include <io/logctl.h>
#include <ns.h>
#include <async.h>
#include <stdio.h>
#include <errno.h>
#include <str_error.h>
#include <malloc.h>
#include "logger.h"


static logging_namespace_t *find_namespace_and_attach_writer(void)
{
	ipc_call_t call;
	ipc_callid_t callid = async_get_call(&call);

	if (IPC_GET_IMETHOD(call) != LOGGER_REGISTER) {
		async_answer_0(callid, EINVAL);
		return NULL;
	}

	void *name;
	int rc = async_data_write_accept(&name, true, 1, MAX_NAMESPACE_LENGTH, 0, NULL);
	async_answer_0(callid, rc);

	if (rc != EOK) {
		return NULL;
	}

	logging_namespace_t *result = namespace_writer_attach((const char *) name);

	free(name);

	return result;
}

static int handle_receive_message(logging_namespace_t *namespace, sysarg_t context, int level)
{
	bool skip_message = !namespace_has_reader(namespace, context, level);
	if (skip_message) {
		/* Abort the actual message buffer transfer. */
		ipc_callid_t callid;
		size_t size;
		int rc = ENAK;
		if (!async_data_write_receive(&callid, &size))
			rc = EINVAL;

		async_answer_0(callid, rc);
		return rc;
	}

	void *message;
	int rc = async_data_write_accept(&message, true, 0, 0, 0, NULL);
	if (rc != EOK) {
		return rc;
	}

	namespace_add_message(namespace, message, context, level);

	free(message);

	return EOK;
}

static int handle_create_context(logging_namespace_t *namespace, sysarg_t *idx)
{
	void *name;
	int rc = async_data_write_accept(&name, true, 0, 0, 0, NULL);
	if (rc != EOK) {
		return rc;
	}

	rc = namespace_create_context(namespace, name);

	free(name);

	if (rc < 0)
		return rc;

	*idx = (sysarg_t) rc;
	return EOK;
}

void logger_connection_handler_writer(ipc_callid_t callid)
{
	/* First call has to be the registration. */
	async_answer_0(callid, EOK);
	logging_namespace_t *namespace = find_namespace_and_attach_writer();
	if (namespace == NULL) {
		fprintf(stderr, NAME ": failed to register namespace.\n");
		return;
	}

	printf(NAME "/writer: new client %s.\n", namespace_get_name(namespace));

	while (true) {
		ipc_call_t call;
		ipc_callid_t callid = async_get_call(&call);

		if (!IPC_GET_IMETHOD(call))
			break;

		int rc;
		sysarg_t arg = 0;

		switch (IPC_GET_IMETHOD(call)) {
		case LOGGER_CREATE_CONTEXT:
			rc = handle_create_context(namespace, &arg);
			async_answer_1(callid, rc, arg);
			break;
		case LOGGER_MESSAGE:
			rc = handle_receive_message(namespace, IPC_GET_ARG1(call), IPC_GET_ARG2(call));
			async_answer_0(callid, rc);
			break;
		default:
			async_answer_0(callid, EINVAL);
			break;
		}
	}

	printf(NAME "/sink: client %s terminated.\n", namespace_get_name(namespace));
	namespace_writer_detach(namespace);
}


/**
 * @}
 */
