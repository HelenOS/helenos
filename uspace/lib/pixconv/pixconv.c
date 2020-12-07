/*
 * Copyright (c) 2011 Martin Decky
 * Copyright (c) 2011 Petr Koupy
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
 * @file Pixel conversion and mask functions.
 *
 * These functions write an ARGB pixel value to a memory location
 * in a predefined format. The naming convention corresponds to
 * the names of the visuals and the format created by these functions.
 * The functions use the so called network bit order (i.e. big endian)
 * with respect to their names.
 */

#include <byteorder.h>
#include "pixconv.h"

void pixel2argb_8888(void *dst, pixel_t pix)
{
	*((uint32_t *) dst) = host2uint32_t_be(
	    (ALPHA(pix) << 24) | (RED(pix) << 16) | (GREEN(pix) << 8) | (BLUE(pix)));
}

void pixel2abgr_8888(void *dst, pixel_t pix)
{
	*((uint32_t *) dst) = host2uint32_t_be(
	    (ALPHA(pix) << 24) | (BLUE(pix) << 16) | (GREEN(pix) << 8) | (RED(pix)));
}

void pixel2rgba_8888(void *dst, pixel_t pix)
{
	*((uint32_t *) dst) = host2uint32_t_be(
	    (RED(pix) << 24) | (GREEN(pix) << 16) | (BLUE(pix) << 8) | (ALPHA(pix)));
}

void pixel2bgra_8888(void *dst, pixel_t pix)
{
	*((uint32_t *) dst) = host2uint32_t_be(
	    (BLUE(pix) << 24) | (GREEN(pix) << 16) | (RED(pix) << 8) | (ALPHA(pix)));
}

void pixel2rgb_0888(void *dst, pixel_t pix)
{
	*((uint32_t *) dst) = host2uint32_t_be(
	    (RED(pix) << 16) | (GREEN(pix) << 8) | (BLUE(pix)));
}

void pixel2bgr_0888(void *dst, pixel_t pix)
{
	*((uint32_t *) dst) = host2uint32_t_be(
	    (BLUE(pix) << 16) | (GREEN(pix) << 8) | (RED(pix)));
}

void pixel2rgb_8880(void *dst, pixel_t pix)
{
	*((uint32_t *) dst) = host2uint32_t_be(
	    (RED(pix) << 24) | (GREEN(pix) << 16) | (BLUE(pix) << 8));
}

void pixel2bgr_8880(void *dst, pixel_t pix)
{
	*((uint32_t *) dst) = host2uint32_t_be(
	    (BLUE(pix) << 24) | (GREEN(pix) << 16) | (RED(pix) << 8));
}

void pixel2rgb_888(void *dst, pixel_t pix)
{
	((uint8_t *) dst)[0] = RED(pix);
	((uint8_t *) dst)[1] = GREEN(pix);
	((uint8_t *) dst)[2] = BLUE(pix);
}

void pixel2bgr_888(void *dst, pixel_t pix)
{
	((uint8_t *) dst)[0] = BLUE(pix);
	((uint8_t *) dst)[1] = GREEN(pix);
	((uint8_t *) dst)[2] = RED(pix);
}

void pixel2rgb_555_be(void *dst, pixel_t pix)
{
	*((uint16_t *) dst) = host2uint16_t_be(
	    (NARROW(RED(pix), 5) << 10) | (NARROW(GREEN(pix), 5) << 5) | (NARROW(BLUE(pix), 5)));
}

void pixel2rgb_555_le(void *dst, pixel_t pix)
{
	*((uint16_t *) dst) = host2uint16_t_le(
	    (NARROW(RED(pix), 5) << 10) | (NARROW(GREEN(pix), 5) << 5) | (NARROW(BLUE(pix), 5)));
}

void pixel2rgb_565_be(void *dst, pixel_t pix)
{
	*((uint16_t *) dst) = host2uint16_t_be(
	    (NARROW(RED(pix), 5) << 11) | (NARROW(GREEN(pix), 6) << 5) | (NARROW(BLUE(pix), 5)));
}

void pixel2rgb_565_le(void *dst, pixel_t pix)
{
	*((uint16_t *) dst) = host2uint16_t_le(
	    (NARROW(RED(pix), 5) << 11) | (NARROW(GREEN(pix), 6) << 5) | (NARROW(BLUE(pix), 5)));
}

void pixel2bgr_323(void *dst, pixel_t pix)
{
	*((uint8_t *) dst) =
	    ~((NARROW(RED(pix), 3) << 5) | (NARROW(GREEN(pix), 2) << 3) | NARROW(BLUE(pix), 3));
}

void pixel2gray_8(void *dst, pixel_t pix)
{
	uint32_t red = RED(pix) * 5034375;
	uint32_t green = GREEN(pix) * 9886846;
	uint32_t blue = BLUE(pix) * 1920103;

	*((uint8_t *) dst) = (red + green + blue) >> 24;
}

void visual_mask_8888(void *dst, bool mask)
{
	pixel2abgr_8888(dst, mask ? 0xffffffff : 0);
}

void visual_mask_0888(void *dst, bool mask)
{
	pixel2bgr_0888(dst, mask ? 0xffffffff : 0);
}

void visual_mask_8880(void *dst, bool mask)
{
	pixel2bgr_8880(dst, mask ? 0xffffffff : 0);
}

void visual_mask_888(void *dst, bool mask)
{
	pixel2bgr_888(dst, mask ? 0xffffffff : 0);
}

void visual_mask_555(void *dst, bool mask)
{
	pixel2rgb_555_be(dst, mask ? 0xffffffff : 0);
}

void visual_mask_565(void *dst, bool mask)
{
	pixel2rgb_565_be(dst, mask ? 0xffffffff : 0);
}

void visual_mask_323(void *dst, bool mask)
{
	pixel2bgr_323(dst, mask ? 0x0 : ~0x0);
}

void visual_mask_8(void *dst, bool mask)
{
	pixel2gray_8(dst, mask ? 0xffffffff : 0);
}

pixel_t argb_8888_2pixel(void *src)
{
	return (uint32_t_be2host(*((uint32_t *) src)));
}

pixel_t abgr_8888_2pixel(void *src)
{
	uint32_t val = uint32_t_be2host(*((uint32_t *) src));
	return ((val & 0xff000000) | ((val & 0xff0000) >> 16) | (val & 0xff00) | ((val & 0xff) << 16));
}

pixel_t rgba_8888_2pixel(void *src)
{
	uint32_t val = uint32_t_be2host(*((uint32_t *) src));
	return ((val << 24) | (val >> 8));
}

pixel_t bgra_8888_2pixel(void *src)
{
	uint32_t val = uint32_t_be2host(*((uint32_t *) src));
	return ((val >> 24) | ((val & 0xff0000) >> 8) | ((val & 0xff00) << 8) | (val << 24));
}

pixel_t rgb_0888_2pixel(void *src)
{
	return (0xff000000 | (uint32_t_be2host(*((uint32_t *) src)) & 0xffffff));
}

pixel_t bgr_0888_2pixel(void *src)
{
	uint32_t val = uint32_t_be2host(*((uint32_t *) src));
	return (0xff000000 | ((val & 0xff0000) >> 16) | (val & 0xff00) | ((val & 0xff) << 16));
}

pixel_t rgb_8880_2pixel(void *src)
{
	return (0xff000000 | (uint32_t_be2host(*((uint32_t *) src)) >> 8));
}

pixel_t bgr_8880_2pixel(void *src)
{
	uint32_t val = uint32_t_be2host(*((uint32_t *) src));
	return (0xff000000 | (val >> 24) | ((val & 0xff0000) >> 8) | ((val & 0xff00) << 8));
}

pixel_t rgb_888_2pixel(void *src)
{
	uint8_t red = ((uint8_t *) src)[0];
	uint8_t green = ((uint8_t *) src)[1];
	uint8_t blue = ((uint8_t *) src)[2];

	return (0xff000000 | (red << 16) | (green << 8) | (blue));
}

pixel_t bgr_888_2pixel(void *src)
{
	uint8_t blue = ((uint8_t *) src)[0];
	uint8_t green = ((uint8_t *) src)[1];
	uint8_t red = ((uint8_t *) src)[2];

	return (0xff000000 | (red << 16) | (green << 8) | (blue));
}

pixel_t rgb_555_be_2pixel(void *src)
{
	uint16_t val = uint16_t_be2host(*((uint16_t *) src));
	return (0xff000000 | ((val & 0x7c00) << 9) | ((val & 0x3e0) << 6) | ((val & 0x1f) << 3));
}

pixel_t rgb_555_le_2pixel(void *src)
{
	uint16_t val = uint16_t_le2host(*((uint16_t *) src));
	return (0xff000000 | ((val & 0x7c00) << 9) | ((val & 0x3e0) << 6) | ((val & 0x1f) << 3));
}

pixel_t rgb_565_be_2pixel(void *src)
{
	uint16_t val = uint16_t_be2host(*((uint16_t *) src));
	return (0xff000000 | ((val & 0xf800) << 8) | ((val & 0x7e0) << 5) | ((val & 0x1f) << 3));
}

pixel_t rgb_565_le_2pixel(void *src)
{
	uint16_t val = uint16_t_le2host(*((uint16_t *) src));
	return (0xff000000 | ((val & 0xf800) << 8) | ((val & 0x7e0) << 5) | ((val & 0x1f) << 3));
}

pixel_t bgr_323_2pixel(void *src)
{
	uint8_t val = ~(*((uint8_t *) src));
	return (0xff000000 | ((val & 0xe0) << 16) | ((val & 0x18) << 11) | ((val & 0x7) << 5));
}

pixel_t gray_8_2pixel(void *src)
{
	uint8_t val = *((uint8_t *) src);
	return (0xff000000 | (val << 16) | (val << 8) | (val));
}

/** @}
 */
