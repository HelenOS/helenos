/*
 * SPDX-FileCopyrightText: 2008 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libcipc
 * @{
 */
/** @file
 */

#ifndef _LIBC_IPC_LOADER_H_
#define _LIBC_IPC_LOADER_H_

#include <ipc/common.h>

typedef enum {
	LOADER_HELLO = IPC_FIRST_USER_METHOD,
	LOADER_GET_TASKID,
	LOADER_SET_CWD,
	LOADER_SET_PROGRAM,
	LOADER_SET_ARGS,
	LOADER_ADD_INBOX,
	LOADER_LOAD,
	LOADER_RUN
} loader_request_t;

#endif

/** @}
 */
