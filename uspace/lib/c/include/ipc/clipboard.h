/*
 * SPDX-FileCopyrightText: 2009 Martin Decky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libcipc
 * @{
 */
/** @file
 */

#ifndef _LIBC_IPC_CLIPBOARD_H_
#define _LIBC_IPC_CLIPBOARD_H_

#include <ipc/common.h>

typedef enum {
	CLIPBOARD_PUT_DATA = IPC_FIRST_USER_METHOD,
	CLIPBOARD_GET_DATA,
	CLIPBOARD_CONTENT
} clipboard_request_t;

typedef enum {
	CLIPBOARD_TAG_NONE,
	CLIPBOARD_TAG_DATA
} clipboard_tag_t;

#endif

/** @}
 */
