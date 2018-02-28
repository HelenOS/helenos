/*
 * Copyright (c) 2011 Petr Koupy
 * Copyright (c) 2014 Martin Sucha
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

/** @addtogroup libc
 * @{
 */
/**
 * @file
 */

#ifndef LIBC_IO_PIXELMAP_H_
#define LIBC_IO_PIXELMAP_H_

#include <types/common.h>
#include <stdbool.h>
#include <stddef.h>
#include <io/pixel.h>

/* Defines how a pixel outside of pixmap rectangle shall be treated */
typedef enum {
	/* Pixels outside of a pixmap are PIXEL(0, 0, 0, 0) */
	PIXELMAP_EXTEND_TRANSPARENT_BLACK = 0,

	/* The pixmap is repeated infinetely */
	PIXELMAP_EXTEND_TILE,

	/* If outside of a pixmap, return closest pixel from the edge */
	PIXELMAP_EXTEND_SIDES,

	/* If outside of a pixmap, return closest pixel from the edge,
	 * with alpha = 0
	 */
	PIXELMAP_EXTEND_TRANSPARENT_SIDES
} pixelmap_extend_t;

typedef struct {
	sysarg_t width;
	sysarg_t height;
	pixel_t *data;
} pixelmap_t;

static inline pixel_t *pixelmap_pixel_at(
    pixelmap_t *pixelmap,
    sysarg_t x,
    sysarg_t y)
{
	if (x < pixelmap->width && y < pixelmap->height) {
		size_t offset = y * pixelmap->width + x;
		pixel_t *pixel = pixelmap->data + offset;
		return pixel;
	} else {
		return NULL;
	}
}

static inline void pixelmap_put_pixel(
    pixelmap_t * pixelmap,
    sysarg_t x,
    sysarg_t y,
    pixel_t pixel)
{
	pixel_t *target = pixelmap_pixel_at(pixelmap, x, y);
	if (target != NULL) {
		*target = pixel;
	}
}

static inline pixel_t pixelmap_get_pixel(
    pixelmap_t *pixelmap,
    sysarg_t x,
    sysarg_t y)
{
	pixel_t *source = pixelmap_pixel_at(pixelmap, x, y);
	if (source != NULL) {
		return *source;
	} else {
		return 0;
	}
}

static inline pixel_t pixelmap_get_extended_pixel(pixelmap_t *pixmap,
    native_t x, native_t y, pixelmap_extend_t extend)
{
	bool transparent = false;
	if (extend == PIXELMAP_EXTEND_TILE) {
		x %= pixmap->width;
		y %= pixmap->height;
	}
	else if (extend == PIXELMAP_EXTEND_SIDES ||
	    extend == PIXELMAP_EXTEND_TRANSPARENT_SIDES) {
		bool transparent_outside =
		    (extend == PIXELMAP_EXTEND_TRANSPARENT_SIDES);
		if (x < 0) {
			x = 0;
			transparent = transparent_outside;
		}
		else if (((sysarg_t) x) >= pixmap->width) {
			x = pixmap->width - 1;
			transparent = transparent_outside;
		}

		if (y < 0) {
			y = 0;
			transparent = transparent_outside;
		}
		else if (((sysarg_t) y) >= pixmap->height) {
			y = pixmap->height - 1;
			transparent = transparent_outside;
		}
	}

	if (x < 0 || ((sysarg_t) x) >= pixmap->width ||
	    y < 0 || ((sysarg_t) y) >= pixmap->height)
		return PIXEL(0, 0, 0, 0);

	pixel_t pixel = pixelmap_get_pixel(pixmap, x, y);

	if (transparent)
		pixel = PIXEL(0, RED(pixel), GREEN(pixel), BLUE(pixel));

	return pixel;
}


#endif

/** @}
 */
