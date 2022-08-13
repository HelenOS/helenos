/*
 * SPDX-FileCopyrightText: 2021 Jiri Svoboda
 * SPDX-FileCopyrightText: 2006 Josef Cejka
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libcipc
 * @{
 */
/** @file
 */

#ifndef _LIBC_IPC_CONSOLE_H_
#define _LIBC_IPC_CONSOLE_H_

#include <ipc/vfs.h>

typedef enum {
	CONSOLE_GET_SIZE = VFS_OUT_LAST,
	CONSOLE_GET_COLOR_CAP,
	CONSOLE_GET_EVENT,
	CONSOLE_GET_POS,
	CONSOLE_SET_POS,
	CONSOLE_CLEAR,
	CONSOLE_SET_STYLE,
	CONSOLE_SET_COLOR,
	CONSOLE_SET_RGB_COLOR,
	CONSOLE_SET_CURSOR_VISIBILITY,
	CONSOLE_SET_CAPTION,
	CONSOLE_MAP,
	CONSOLE_UNMAP,
	CONSOLE_UPDATE
} console_request_t;

#endif

/** @}
 */
