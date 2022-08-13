/*
 * SPDX-FileCopyrightText: 2010 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libdevice
 * @{
 */
/** @file
 * @brief ADB device interface.
 */

#ifndef LIBDEVICE_IPC_ADB_H
#define LIBDEVICE_IPC_ADB_H

#include <ipc/common.h>

typedef enum {
	ADB_REG_WRITE = IPC_FIRST_USER_METHOD
} adb_request_t;

typedef enum {
	ADB_REG_NOTIF = IPC_FIRST_USER_METHOD
} adb_notif_t;

#endif

/** @}
 */
