/*
 * SPDX-FileCopyrightText: 2021 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
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
