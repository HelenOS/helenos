/*
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

/** @addtogroup softrend
 * @{
 */
/**
 * @file
 */

#include "filter.h"

pixel_t filter_nearest(pixelmap_t *pixmap, double x, double y, bool tile)
{
	long _x = x > 0 ? (long) (x + 0.5) : (long) (x - 0.5);
	long _y = y > 0 ? (long) (y + 0.5) : (long) (y - 0.5);

	if (tile) {
		_x %= pixmap->width;
		_y %= pixmap->height;
	} else if (_x < 0 || _x >= (long) pixmap->width || _y < 0 || _y >= (long) pixmap->height) {
		return 0;
	}

	return pixelmap_get_pixel(pixmap, (sysarg_t) _x, (sysarg_t) _y);
}

pixel_t filter_bilinear(pixelmap_t *pixmap, double x, double y, bool tile)
{
	// TODO
	return 0;
}

pixel_t filter_bicubic(pixelmap_t *pixmap, double x, double y, bool tile)
{
	// TODO
	return 0;
}

/** @}
 */
