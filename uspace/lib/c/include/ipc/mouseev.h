/*
 * SPDX-FileCopyrightText: 2009 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libc
 * @brief
 * @{
 */
/** @file
 */

#ifndef _LIBC_IPC_MOUSEEV_H_
#define _LIBC_IPC_MOUSEEV_H_

#include <ipc/common.h>
#include <ipc/dev_iface.h>

typedef enum {
	MOUSEEV_YIELD = DEV_FIRST_CUSTOM_METHOD,
	MOUSEEV_RECLAIM
} mouseev_request_t;

typedef enum {
	MOUSEEV_MOVE_EVENT = IPC_FIRST_USER_METHOD,
	MOUSEEV_ABS_MOVE_EVENT,
	MOUSEEV_BUTTON_EVENT
} mouseev_notif_t;

#endif

/**
 * @}
 */
