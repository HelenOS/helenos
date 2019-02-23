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

#include <ipc/services.h>
#include <ipc/logger.h>
#include <io/log.h>
#include <io/logctl.h>
#include <io/klog.h>
#include <ns.h>
#include <async.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <str_error.h>
#include "logger.h"

static logger_log_t *handle_create_log(sysarg_t parent)
{
	void *name;
	errno_t rc = async_data_write_accept(&name, true, 1, 0, 0, NULL);
	if (rc != EOK)
		return NULL;

	logger_log_t *log = find_or_create_log_and_lock(name, parent);

	free(name);

	return log;
}

static errno_t handle_receive_message(sysarg_t log_id, sysarg_t level)
{
	logger_log_t *log = find_log_by_id_and_lock(log_id);
	if (log == NULL)
		return ENOENT;

	void *message = NULL;
	errno_t rc = async_data_write_accept(&message, true, 1, 0, 0, NULL);
	if (rc != EOK)
		goto leave;

	if (!shall_log_message(log, level)) {
		rc = EOK;
		goto leave;
	}

	KLOG_PRINTF(level, "[%s] %s: %s",
	    log->full_name, log_level_str(level),
	    (const char *) message);
	write_to_log(log, level, message);

	rc = EOK;

leave:
	log_unlock(log);
	free(message);

	return rc;
}

void logger_connection_handler_writer(ipc_call_t *icall)
{
	logger_log_t *log;
	errno_t rc;

	/* Acknowledge the connection. */
	async_accept_0(icall);

	logger_log("writer: new client.\n");

	logger_registered_logs_t registered_logs;
	registered_logs_init(&registered_logs);

	while (true) {
		ipc_call_t call;
		async_get_call(&call);

		if (!ipc_get_imethod(&call)) {
			async_answer_0(&call, EOK);
			break;
		}

		switch (ipc_get_imethod(&call)) {
		case LOGGER_WRITER_CREATE_LOG:
			log = handle_create_log(ipc_get_arg1(&call));
			if (log == NULL) {
				async_answer_0(&call, ENOMEM);
				break;
			}
			if (!register_log(&registered_logs, log)) {
				log_unlock(log);
				async_answer_0(&call, ELIMIT);
				break;
			}
			log_unlock(log);
			async_answer_1(&call, EOK, (sysarg_t) log);
			break;
		case LOGGER_WRITER_MESSAGE:
			rc = handle_receive_message(ipc_get_arg1(&call),
			    ipc_get_arg2(&call));
			async_answer_0(&call, rc);
			break;
		default:
			async_answer_0(&call, EINVAL);
			break;
		}
	}

	unregister_logs(&registered_logs);
	logger_log("writer: client terminated.\n");
}

/**
 * @}
 */
