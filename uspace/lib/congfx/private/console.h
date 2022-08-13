/*
 * SPDX-FileCopyrightText: 2021 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libcongfx
 * @{
 */
/**
 * @file GFX console backend structure
 *
 */

#ifndef _CONGFX_PRIVATE_CONSOLE_H
#define _CONGFX_PRIVATE_CONSOLE_H

#include <gfx/context.h>
#include <io/charfield.h>
#include <io/console.h>
#include <io/pixel.h>
#include <stdio.h>

/** Actual structure of graphics context.
 *
 * This is private to libcongfx. It is not visible to clients nor backends.
 */
struct console_gc {
	/** Base graphic context */
	gfx_context_t *gc;
	/** Console control structure */
	console_ctrl_t *con;
	/** Console bounding rectangle */
	gfx_rect_t rect;
	/** Clipping rectangle */
	gfx_rect_t clip_rect;
	/** File for printing characters */
	FILE *fout;
	/** Current drawing color */
	pixel_t clr;
	/** Shared console buffer */
	charfield_t *buf;
};

/** Bitmap in console GC */
typedef struct {
	/** Containing console GC */
	struct console_gc *cgc;
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
} console_gc_bitmap_t;

#endif

/** @}
 */
