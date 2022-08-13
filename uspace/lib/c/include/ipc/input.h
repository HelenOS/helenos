/*
 * SPDX-FileCopyrightText: 2021 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libcipc
 * @{
 */
/** @file
 */

#ifndef _LIBC_IPC_INPUT_H_
#define _LIBC_IPC_INPUT_H_

#include <ipc/common.h>

typedef enum {
	INPUT_ACTIVATE = IPC_FIRST_USER_METHOD
} input_request_t;

typedef enum {
	INPUT_EVENT_ACTIVE = IPC_FIRST_USER_METHOD,
	INPUT_EVENT_DEACTIVE,
	INPUT_EVENT_KEY,
	INPUT_EVENT_MOVE,
	INPUT_EVENT_ABS_MOVE,
	INPUT_EVENT_BUTTON,
	INPUT_EVENT_DCLICK
} input_notif_t;

#endif

/**
 * @}
 */
