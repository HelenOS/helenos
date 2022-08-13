/*
 * SPDX-FileCopyrightText: 2019 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libipcgfx
 * @{
 */
/**
 * @file GFX console backend structure
 *
 */

#ifndef _IPCGFX_PRIVATE_CLIENT_H
#define _IPCGFX_PRIVATE_CLIENT_H

#include <async.h>
#include <gfx/context.h>

/** Actual structure of graphics context.
 *
 * This is private to libcongfx. It is not visible to clients nor backends.
 */
struct ipc_gc {
	/** Base graphic context */
	gfx_context_t *gc;
	/** Session with GFX server */
	async_sess_t *sess;
};

/** Bitmap in IPC GC */
typedef struct {
	/** Containing IPC GC */
	struct ipc_gc *ipcgc;
	/** Allocation info */
	gfx_bitmap_alloc_t alloc;
	/** @c true if we allocated the bitmap, @c false if allocated by caller */
	bool myalloc;
	/** Rectangle covered by bitmap */
	gfx_rect_t rect;
	/** Server bitmap ID */
	sysarg_t bmp_id;
} ipc_gc_bitmap_t;

#endif

/** @}
 */
