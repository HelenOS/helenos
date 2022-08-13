/*
 * SPDX-FileCopyrightText: 2019 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libipcgfx
 * @{
 */
/**
 * @file IPC GFX backend structure
 *
 */

#ifndef _IPCGFX_PRIVATE_SERVER_H
#define _IPCGFX_PRIVATE_SERVER_H

#include <adt/list.h>
#include <async.h>
#include <gfx/bitmap.h>
#include <gfx/context.h>
#include <gfx/coord.h>
#include <stdbool.h>

/** Server-side of IPC GC connection.
 */
typedef struct {
	/** Graphics context to serve */
	gfx_context_t *gc;
	/** List of bitmaps (ipc_gc_srv_bitmap_t) */
	list_t bitmaps;
	/** Next bitmap ID to allocate */
	sysarg_t next_bmp_id;
} ipc_gc_srv_t;

/** Bitmap in canvas GC */
typedef struct {
	/** Containing canvas GC */
	ipc_gc_srv_t *srvgc;
	/** Link to srvgc->bitmaps */
	link_t lbitmaps;
	/** Backing bitmap */
	gfx_bitmap_t *bmp;
	/** Bitmap ID */
	sysarg_t bmp_id;
	/** @c true if we allocated the pixels */
	bool myalloc;
	/** Pixels of the bitmap */
	void *pixels;
} ipc_gc_srv_bitmap_t;

#endif

/** @}
 */
