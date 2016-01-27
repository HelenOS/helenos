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

/** @addtogroup libcipc
 * @{
 */
/** @file
 */

#ifndef LIBC_IPC_SYSMAN_H_
#define LIBC_IPC_SYSMAN_H_

#include <ipc/common.h>

typedef enum {
	SYSMAN_BROKER_REGISTER = IPC_FIRST_USER_METHOD,
	SYSMAN_BROKER_IPC_FWD,
	SYSMAN_BROKER_MAIN_EXP_ADDED,
	SYSMAN_BROKER_EXP_ADDED,
	SYSMAN_BROKER_EXP_REMOVED,
	SYSMAN_CTL_UNIT_HANDLE,
	SYSMAN_CTL_UNIT_START_BY_NAME,
	SYSMAN_CTL_UNIT_START,
	SYSMAN_CTL_UNIT_STOP,
	SYSMAN_CTL_GET_UNITS,
	SYSMAN_CTL_UNIT_GET_NAME,
	SYSMAN_CTL_UNIT_GET_STATE,
	SYSMAN_CTL_SHUTDOWN
} sysman_ipc_method_t;

typedef enum {
	SYSMAN_PORT_BROKER = 0,
	SYSMAN_PORT_CTL,
	SYSMAN_PORT_MAX_
} sysman_interface_t;

typedef sysarg_t unit_handle_t;

typedef enum {
	UNIT_TYPE_INVALID = -1,
	UNIT_CONFIGURATION = 0,
	UNIT_MOUNT,
	UNIT_SERVICE,
	UNIT_TARGET
} unit_type_t;

typedef enum {
	STATE_STARTING,
	STATE_STARTED,
	STATE_STOPPED,
	STATE_STOPPING,
	STATE_FAILED
} unit_state_t;

#endif

/** @}
 */
