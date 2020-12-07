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
