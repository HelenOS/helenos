/*
 * SPDX-FileCopyrightText: 2015 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libdevice
 * @{
 */

#ifndef LIBDEVICE_IPC_VBD_H
#define LIBDEVICE_IPC_VBD_H

#include <ipc/common.h>

typedef enum {
	VBD_GET_DISKS = IPC_FIRST_USER_METHOD,
	VBD_DISK_INFO,
	VBD_LABEL_CREATE,
	VBD_LABEL_DELETE,
	VBD_LABEL_GET_PARTS,
	VBD_PART_GET_INFO,
	VBD_PART_CREATE,
	VBD_PART_DELETE,
	VBD_SUGGEST_PTYPE
} vbd_request_t;

#endif

/** @}
 */
