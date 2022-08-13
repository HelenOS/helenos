/*
 * SPDX-FileCopyrightText: 2019 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libgfx
 * @{
 */
/**
 * @file Color types
 */

#ifndef _GFX_TYPES_BITMAP_H
#define _GFX_TYPES_BITMAP_H

#include <errno.h>
#include <io/pixel.h>
#include <stddef.h>
#include <types/gfx/coord.h>

struct gfx_bitmap;
typedef struct gfx_bitmap gfx_bitmap_t;

/** Bitmap flags */
typedef enum {
	/** Directly map GC output into this bitmap */
	bmpf_direct_output = 0x1,
	/** Enable color key */
	bmpf_color_key = 0x2,
	/** Paint non-background pixels with current drawing color */
	bmpf_colorize = 0x4
} gfx_bitmap_flags_t;

/** Bitmap parameters */
typedef struct {
	/** Rectangle represented in pixel array */
	gfx_rect_t rect;
	/** Bitmap flags */
	gfx_bitmap_flags_t flags;
	/** Key color */
	pixel_t key_color;
} gfx_bitmap_params_t;

/** Bitmap allocation info */
typedef struct {
	/** Byte offset from one successive line to the next */
	int pitch;
	/** Byte offset of first pixel */
	size_t off0;
	/** Pixel array */
	void *pixels;
} gfx_bitmap_alloc_t;

#endif

/** @}
 */
