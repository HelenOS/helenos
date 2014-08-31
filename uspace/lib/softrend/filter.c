/*
 * Copyright (c) 2012 Petr Koupy
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

/** @addtogroup softrend
 * @{
 */
/**
 * @file
 */

#include "filter.h"
#include <io/pixel.h>

static long round(double val)
{
	return val > 0 ? (long) (val + 0.5) : (long) (val - 0.5);
}

static long floor(double val)
{
	long lval = (long) val;
	if (val < 0 && lval != val)
		return lval - 1;
	return lval;
}

static long ceil(double val)
{
	long lval = (long) val;
	if (val > 0 && lval != val)
		return lval + 1;
	return lval;
}

static pixel_t get_pixel(pixelmap_t *pixmap, sysarg_t x, sysarg_t y, bool tile)
{
	if (tile) {
		x %= pixmap->width;
		y %= pixmap->height;
	}

	return pixelmap_get_pixel(pixmap, (sysarg_t) x, (sysarg_t) y);
}

static inline pixel_t blend_pixels(size_t count, float *weights,
    pixel_t *pixels)
{
	float alpha = 0, red = 0, green = 0, blue = 0;
	for (size_t index = 0; index < count; index++) {
		alpha += weights[index] * ALPHA(pixels[index]);
		red   += weights[index] *   RED(pixels[index]);
		green += weights[index] * GREEN(pixels[index]);
		blue  += weights[index] *  BLUE(pixels[index]);
	}
	
	return PIXEL((uint8_t) alpha, (uint8_t) red, (uint8_t) green,
	    (uint8_t) blue);
}

pixel_t filter_nearest(pixelmap_t *pixmap, double x, double y, bool tile)
{
	return get_pixel(pixmap, round(x), round(y), tile);
}

pixel_t filter_bilinear(pixelmap_t *pixmap, double x, double y, bool tile)
{
	long x1 = floor(x);
	long x2 = ceil(x);
	long y1 = floor(y);
	long y2 = ceil(y);
	
	if (y1 == y2 && x1 == x2) {
		return get_pixel(pixmap, (sysarg_t) x1, (sysarg_t) y1,
		    tile);
	}
	
	double x_delta = x - x1;
	double y_delta = y - y1;
	
	pixel_t pixels[4];
	pixels[0] = get_pixel(pixmap, x1, y1, tile);
	pixels[1] = get_pixel(pixmap, x2, y1, tile);
	pixels[2] = get_pixel(pixmap, x1, y2, tile);
	pixels[3] = get_pixel(pixmap, x2, y2, tile);
	
	float weights[4];
	weights[0] = (1 - x_delta) * (1 - y_delta);
	weights[1] = (    x_delta) * (1 - y_delta);
	weights[2] = (1 - x_delta) * (    y_delta);
	weights[3] = (    x_delta) * (    y_delta);
	
	return blend_pixels(4, weights, pixels);
}

pixel_t filter_bicubic(pixelmap_t *pixmap, double x, double y, bool tile)
{
	// TODO
	return 0;
}

/** @}
 */
