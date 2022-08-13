/*
 * SPDX-FileCopyrightText: 2006 Ondrej Palkovsky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libcipc
 * @{
 */
/** @file
 */

#ifndef _LIBC_IPC_NS_H_
#define _LIBC_IPC_NS_H_

#include <ipc/common.h>

typedef enum {
	NS_PING = IPC_FIRST_USER_METHOD,
	NS_REGISTER,
	NS_REGISTER_BROKER,
	NS_TASK_WAIT,
	NS_ID_INTRO,
	NS_RETVAL
} ns_request_t;

#endif

/** @}
 */
