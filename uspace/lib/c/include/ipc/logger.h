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

#ifndef LIBC_IPC_LOGGER_H_
#define LIBC_IPC_LOGGER_H_

#include <ipc/common.h>

typedef enum {
	LOGGER_CTL_GET_DEFAULT_LEVEL = IPC_FIRST_USER_METHOD,
	LOGGER_CTL_SET_DEFAULT_LEVEL,
	LOGGER_CTL_SET_NAMESPACE_LEVEL
} logger_control_request_t;

typedef enum {
	LOGGER_REGISTER = IPC_FIRST_USER_METHOD,
	LOGGER_CREATE_CONTEXT,
	LOGGER_MESSAGE,
	LOGGER_BLOCK_UNTIL_READER_CHANGED
} logger_sink_request_t;

typedef enum {
	 LOGGER_CONNECT = IPC_FIRST_USER_METHOD,
	 LOGGER_GET_MESSAGE
} logger_source_request_t;

typedef enum {
	/** Interface for controlling logger behavior. */
	LOGGER_INTERFACE_CONTROL,
	/** Interface for servers writing to the log. */
	LOGGER_INTERFACE_SINK,
	/** Interface for clients displaying the log. */
	LOGGER_INTERFACE_SOURCE
} logger_interface_t;

#endif

/** @}
 */
