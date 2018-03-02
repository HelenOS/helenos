/*
 * Copyright (c) 2011 Martin Decky
 * Copyright (c) 2012 Petr Koupy
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

/** @addtogroup draw
 * @{
 */
/**
 * @file
 */

#include <mem.h>
#include <as.h>
#include <assert.h>
#include <stdlib.h>
#include "surface.h"

struct surface {
	surface_flags_t flags;

	surface_coord_t dirty_x_lo;
	surface_coord_t dirty_x_hi;
	surface_coord_t dirty_y_lo;
	surface_coord_t dirty_y_hi;

	pixelmap_t pixmap;
};

surface_t *surface_create(surface_coord_t width, surface_coord_t height,
    pixel_t *pixbuf, surface_flags_t flags)
{
	surface_t *surface = (surface_t *) malloc(sizeof(surface_t));
	if (!surface) {
		return NULL;
	}

	size_t pixbuf_size = width * height * sizeof(pixel_t);

	if (!pixbuf) {
		if ((flags & SURFACE_FLAG_SHARED) == SURFACE_FLAG_SHARED) {
			pixbuf = (pixel_t *) as_area_create(AS_AREA_ANY,
			    pixbuf_size,
			    AS_AREA_READ | AS_AREA_WRITE | AS_AREA_CACHEABLE,
			    AS_AREA_UNPAGED);
			if (pixbuf == AS_MAP_FAILED) {
				free(surface);
				return NULL;
			}
		} else {
			pixbuf = (pixel_t *) malloc(pixbuf_size);
			if (pixbuf == NULL) {
				free(surface);
				return NULL;
			}
		}

		memset(pixbuf, 0, pixbuf_size);
	}

	surface->flags = flags;
	surface->pixmap.width = width;
	surface->pixmap.height = height;
	surface->pixmap.data = pixbuf;

	surface_reset_damaged_region(surface);

	return surface;
}

void surface_destroy(surface_t *surface)
{
	pixel_t *pixbuf = surface->pixmap.data;

	if ((surface->flags & SURFACE_FLAG_SHARED) == SURFACE_FLAG_SHARED)
		as_area_destroy((void *) pixbuf);
	else
		free(pixbuf);

	free(surface);
}

bool surface_is_shared(surface_t *surface)
{
	return ((surface->flags & SURFACE_FLAG_SHARED) == SURFACE_FLAG_SHARED);
}

pixel_t *surface_direct_access(surface_t *surface)
{
	return surface->pixmap.data;
}

pixelmap_t *surface_pixmap_access(surface_t *surface)
{
	return &surface->pixmap;
}

void surface_get_resolution(surface_t *surface, surface_coord_t *width, surface_coord_t *height)
{
	assert(width);
	assert(height);

	*width = surface->pixmap.width;
	*height = surface->pixmap.height;
}

void surface_get_damaged_region(surface_t *surface, surface_coord_t *x, surface_coord_t *y,
    surface_coord_t *width, surface_coord_t *height)
{
	assert(x);
	assert(y);
	assert(width);
	assert(height);

	*x = surface->dirty_x_lo;
	*y = surface->dirty_y_lo;
	*width = surface->dirty_x_lo <= surface->dirty_x_hi ?
	    (surface->dirty_x_hi - surface->dirty_x_lo) + 1 : 0;
	*height = surface->dirty_y_lo <= surface->dirty_y_hi ?
	    (surface->dirty_y_hi - surface->dirty_y_lo) + 1 : 0;
}

void surface_add_damaged_region(surface_t *surface, surface_coord_t x, surface_coord_t y,
    surface_coord_t width, surface_coord_t height)
{
	surface->dirty_x_lo = surface->dirty_x_lo > x ? x : surface->dirty_x_lo;
	surface->dirty_y_lo = surface->dirty_y_lo > y ? y : surface->dirty_y_lo;

	surface_coord_t x_hi = x + width - 1;
	surface_coord_t y_hi = y + height - 1;

	surface->dirty_x_hi = surface->dirty_x_hi < x_hi ? x_hi : surface->dirty_x_hi;
	surface->dirty_y_hi = surface->dirty_y_hi < y_hi ? y_hi : surface->dirty_y_hi;
}

void surface_reset_damaged_region(surface_t *surface)
{
	surface->dirty_x_lo = surface->pixmap.width - 1;
	surface->dirty_x_hi = 0;
	surface->dirty_y_lo = surface->pixmap.height - 1;
	surface->dirty_y_hi = 0;
}

void surface_put_pixel(surface_t *surface, surface_coord_t x, surface_coord_t y, pixel_t pixel)
{
	surface->dirty_x_lo = surface->dirty_x_lo > x ? x : surface->dirty_x_lo;
	surface->dirty_x_hi = surface->dirty_x_hi < x ? x : surface->dirty_x_hi;
	surface->dirty_y_lo = surface->dirty_y_lo > y ? y : surface->dirty_y_lo;
	surface->dirty_y_hi = surface->dirty_y_hi < y ? y : surface->dirty_y_hi;

	pixelmap_put_pixel(&surface->pixmap, x, y, pixel);
}

pixel_t surface_get_pixel(surface_t *surface, surface_coord_t x, surface_coord_t y)
{
	return pixelmap_get_pixel(&surface->pixmap, x, y);
}

/** @}
 */
