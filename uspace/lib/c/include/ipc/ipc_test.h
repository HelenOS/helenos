/*
 * SPDX-FileCopyrightText: 2018 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libcipc
 * @{
 */
/** @file
 */

#ifndef _LIBC_IPC_IPC_TEST_H_
#define _LIBC_IPC_IPC_TEST_H_

#include <ipc/common.h>

typedef enum {
	IPC_TEST_PING = IPC_FIRST_USER_METHOD,
	IPC_TEST_GET_RO_AREA_SIZE,
	IPC_TEST_GET_RW_AREA_SIZE,
	IPC_TEST_SHARE_IN_RO,
	IPC_TEST_SHARE_IN_RW
} ipc_test_request_t;

#endif

/** @}
 */
