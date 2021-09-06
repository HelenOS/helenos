/*
 * Copyright (c) 2021 Jiri Svoboda
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

/** @addtogroup libmemgfx
 * @{
 */
/**
 * @file GFX memory backend structure
 *
 */

#ifndef _MEMGFX_PRIVATE_MEMGC_H
#define _MEMGFX_PRIVATE_MEMGC_H

#include <gfx/bitmap.h>
#include <gfx/context.h>
#include <gfx/coord.h>
#include <io/pixel.h>

/** Actual structure of memory GC. */
struct mem_gc {
	/** Base graphic context */
	gfx_context_t *gc;
	/** Bounding rectangle */
	gfx_rect_t rect;
	/** Clipping rectangle */
	gfx_rect_t clip_rect;
	/** Allocation info */
	gfx_bitmap_alloc_t alloc;
	/** Callbacks */
	mem_gc_cb_t *cb;
	/** Argument to callbacks */
	void *cb_arg;
	/** Current drawing color */
	pixel_t color;
};

/** Bitmap in memory GC */
typedef struct {
	/** Containing memory GC */
	struct mem_gc *mgc;
	/** Allocation info */
	gfx_bitmap_alloc_t alloc;
	/** @c true if we allocated the bitmap, @c false if allocated by caller */
	bool myalloc;
	/** Rectangle covered by bitmap */
	gfx_rect_t rect;
	/** Bitmap flags */
	gfx_bitmap_flags_t flags;
	/** Key color */
	pixel_t key_color;
} mem_gc_bitmap_t;

#endif

/** @}
 */
