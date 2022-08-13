/*
 * SPDX-FileCopyrightText: 2012 Vojtech Horky
 *
 * SPDX-License-Identifier: BSD-3-Clause
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
