/*
 * SPDX-FileCopyrightText: 2021 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libipcgfx
 * @{
 */
/** @file
 */

#ifndef _IPCGFX_IPC_GC_H_
#define _IPCGFX_IPC_GC_H_

#include <ipc/common.h>

typedef enum {
	GC_SET_CLIP_RECT = IPC_FIRST_USER_METHOD,
	GC_SET_CLIP_RECT_NULL,
	GC_SET_RGB_COLOR,
	GC_FILL_RECT,
	GC_UPDATE,
	GC_BITMAP_CREATE,
	GC_BITMAP_CREATE_DOUTPUT,
	GC_BITMAP_DESTROY,
	GC_BITMAP_RENDER,
	GC_BITMAP_GET_ALLOC,
} gc_request_t;

#endif

/** @}
 */
