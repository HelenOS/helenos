/*
 * Copyright (c) 2019 Jiri Svoboda
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * - Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 * - Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the distribution.
 * - The name of the author may not be used to endorse or promote products
 *   derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
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
