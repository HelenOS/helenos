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
