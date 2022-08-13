/*
 * SPDX-FileCopyrightText: 2013 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libcipc
 * @{
 */

#ifndef _LIBC_IPC_CORECFG_H_
#define _LIBC_IPC_CORECFG_H_

#include <ipc/common.h>

typedef enum {
	CORECFG_GET_ENABLE = IPC_FIRST_USER_METHOD,
	CORECFG_SET_ENABLE
} corecfg_request_t;

#endif

/** @}
 */
