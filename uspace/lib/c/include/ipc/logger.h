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
