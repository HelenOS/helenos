/*
 * SPDX-FileCopyrightText: 2011 Petr Koupy
 * SPDX-FileCopyrightText: 2014 Martin Sucha
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libc
 * @{
 */
/**
 * @file
 */

#ifndef _LIBC_IO_PIXELMAP_H_
#define _LIBC_IO_PIXELMAP_H_

#include <types/common.h>
#include <stdbool.h>
#include <stddef.h>
#include <io/pixel.h>

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
    pixelmap_t *pixelmap,
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

#endif

/** @}
 */
