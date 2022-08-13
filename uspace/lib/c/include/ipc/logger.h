/*
 * SPDX-FileCopyrightText: 2012 Vojtech Horky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libc
 * @{
 */

#ifndef _LIBC_IPC_LOGGER_H_
#define _LIBC_IPC_LOGGER_H_

#include <ipc/common.h>

typedef enum {
	/** Set (global) default displayed logging level.
	 *
	 * Arguments: new log level.
	 * Returns: error code
	 */
	LOGGER_CONTROL_SET_DEFAULT_LEVEL = IPC_FIRST_USER_METHOD,
	/** Set displayed level for given log.
	 *
	 * Arguments: new log level.
	 * Returns: error code
	 * Followed by: string with full log name.
	 */
	LOGGER_CONTROL_SET_LOG_LEVEL,
	/** Set VFS root.
	 *
	 * Returns: error code
	 * Followed by: vfs_pass_handle() request.
	 */
	LOGGER_CONTROL_SET_ROOT
} logger_control_request_t;

typedef enum {
	/** Create new log.
	 *
	 * Arguments: parent log id (0 for top-level log).
	 * Returns: error code, log id
	 * Followed by: string with log name.
	 */
	LOGGER_WRITER_CREATE_LOG = IPC_FIRST_USER_METHOD,
	/** Write a message to a given log.
	 *
	 * Arguments: log id, message severity level (log_level_t)
	 * Returns: error code
	 * Followed by: string with the message.
	 */
	LOGGER_WRITER_MESSAGE
} logger_writer_request_t;

#endif

/** @}
 */
