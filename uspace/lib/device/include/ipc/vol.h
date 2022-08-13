/*
 * SPDX-FileCopyrightText: 2015 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libdevice
 * @{
 */

#ifndef LIBDEVICE_IPC_VOL_H
#define LIBDEVICE_IPC_VOL_H

#include <ipc/common.h>

#define VOL_LABEL_MAXLEN 63
#define VOL_MOUNTP_MAXLEN 4096

typedef enum {
	VOL_GET_PARTS = IPC_FIRST_USER_METHOD,
	VOL_PART_ADD,
	VOL_PART_INFO,
	VOL_PART_EJECT,
	VOL_PART_EMPTY,
	VOL_PART_INSERT,
	VOL_PART_INSERT_BY_PATH,
	VOL_PART_LSUPP,
	VOL_PART_MKFS,
	VOL_PART_SET_MOUNTP,
	VOL_GET_VOLUMES,
	VOL_INFO
} vol_request_t;

#endif

/** @}
 */
