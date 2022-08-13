/*
 * SPDX-FileCopyrightText: 2014 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libdevice
 * @{
 */
/** @file
 * @brief Character device interface.
 */

#ifndef LIBDEVICE_IPC_CHARDEV_H
#define LIBDEVICE_IPC_CHARDEV_H

#include <ipc/common.h>

typedef enum {
	CHARDEV_READ = IPC_FIRST_USER_METHOD,
	CHARDEV_WRITE,
} chardev_request_t;

enum {
	CHARDEV_LIMIT = CHARDEV_WRITE + 1
};

#endif

/** @}
 */
