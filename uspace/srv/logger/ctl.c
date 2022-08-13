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

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <io/logctl.h>
#include <ipc/logger.h>
#include <vfs/vfs.h>
#include "logger.h"

static errno_t handle_log_level_change(sysarg_t new_level)
{
	void *full_name;
	errno_t rc = async_data_write_accept(&full_name, true, 0, 0, 0, NULL);
	if (rc != EOK) {
		return rc;
	}

	logger_log_t *log = find_log_by_name_and_lock(full_name);
	free(full_name);
	if (log == NULL)
		return ENOENT;

	log->logged_level = new_level;

	log_unlock(log);

	return EOK;
}

void logger_connection_handler_control(ipc_call_t *icall)
{
	errno_t rc;
	int fd;

	async_accept_0(icall);
	logger_log("control: new client.\n");

	while (true) {
		ipc_call_t call;
		async_get_call(&call);

		if (!ipc_get_imethod(&call)) {
			async_answer_0(&call, EOK);
			break;
		}

		switch (ipc_get_imethod(&call)) {
		case LOGGER_CONTROL_SET_DEFAULT_LEVEL:
			rc = set_default_logging_level(ipc_get_arg1(&call));
			async_answer_0(&call, rc);
			break;
		case LOGGER_CONTROL_SET_LOG_LEVEL:
			rc = handle_log_level_change(ipc_get_arg1(&call));
			async_answer_0(&call, rc);
			break;
		case LOGGER_CONTROL_SET_ROOT:
			rc = vfs_receive_handle(true, &fd);
			if (rc == EOK) {
				rc = vfs_root_set(fd);
				vfs_put(fd);
			}
			async_answer_0(&call, rc);
			break;
		default:
			async_answer_0(&call, EINVAL);
			break;
		}
	}

	logger_log("control: client terminated.\n");
}

/**
 * @}
 */
