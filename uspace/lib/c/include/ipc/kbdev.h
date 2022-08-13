/*
 * SPDX-FileCopyrightText: 2011 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libcipc
 * @{
 */
/** @file
 */

#ifndef _LIBC_IPC_KBDEV_H_
#define _LIBC_IPC_KBDEV_H_

#include <ipc/common.h>
#include <ipc/dev_iface.h>

typedef enum {
	KBDEV_YIELD = DEV_FIRST_CUSTOM_METHOD,
	KBDEV_RECLAIM,
	KBDEV_SET_IND
} kbdev_request_t;

typedef enum {
	KBDEV_EVENT = IPC_FIRST_USER_METHOD
} kbdev_notif_t;

#endif

/**
 * @}
 */
