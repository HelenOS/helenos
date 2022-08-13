/*
 * SPDX-FileCopyrightText: 2019 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libddev
 * @{
 */
/** @file
 */

#ifndef LIBDDEV_IPC_DDEV_H
#define LIBDDEV_IPC_DDEV_H

#include <ipc/common.h>

typedef enum {
	DDEV_GET_GC = IPC_FIRST_USER_METHOD,
	DDEV_GET_INFO
} ddev_request_t;

#endif

/** @}
 */
