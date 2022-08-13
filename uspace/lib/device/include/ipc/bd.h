/*
 * SPDX-FileCopyrightText: 2009 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libdevice
 * @{
 */
/** @file
 */

#ifndef LIBDEVICE_IPC_BD_H
#define LIBDEVICE_IPC_BD_H

#include <ipc/common.h>

typedef enum {
	BD_GET_BLOCK_SIZE = IPC_FIRST_USER_METHOD,
	BD_GET_NUM_BLOCKS,
	BD_READ_BLOCKS,
	BD_SYNC_CACHE,
	BD_WRITE_BLOCKS,
	BD_READ_TOC
} bd_request_t;

#endif

/** @}
 */
