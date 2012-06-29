/*
 * Copyright (c) 2011 Martin Decky
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

/** @addtogroup imgmap
 * @{
 */
/**
 * @file
 */

#include <stdlib.h>
#include <byteorder.h>
#include <align.h>
#include <bool.h>
#include <mem.h>
#include <as.h>
#include "imgmap.h"

struct imgmap {
	size_t size;
	imgmap_flags_t flags;
	sysarg_t width;
	sysarg_t height;
	visual_t visual;
	uint8_t data[];
};

/** RGB conversion and mask functions.
 *
 * These functions write an RGB pixel value to a memory location
 * in a predefined format. The naming convention corresponds to
 * the names of the visuals and the format created by these functions.
 * The functions use the so called network bit order (i.e. big endian)
 * with respect to their names.
 */

#define RED(pixel, bits)    (((pixel) >> (8 + 8 + 8 - (bits))) & ((1 << (bits)) - 1))
#define GREEN(pixel, bits)  (((pixel) >> (8 + 8 - (bits))) & ((1 << (bits)) - 1))
#define BLUE(pixel, bits)   (((pixel) >> (8 - (bits))) & ((1 << (bits)) - 1))

void pixel2rgb_0888(void *dst, pixel_t pixel)
{
	*((uint32_t *) dst) = host2uint32_t_be(
	    (RED(pixel, 8) << 16) | (GREEN(pixel, 8) << 8) | (BLUE(pixel, 8)));
}

void pixel2bgr_0888(void *dst, pixel_t pixel)
{
	*((uint32_t *) dst) = host2uint32_t_be(
	    (BLUE(pixel, 8) << 16) | (GREEN(pixel, 8) << 8) | (RED(pixel, 8)));
}

void pixel2rgb_8880(void *dst, pixel_t pixel)
{
	*((uint32_t *) dst) = host2uint32_t_be(
	    (RED(pixel, 8) << 24) | (GREEN(pixel, 8) << 16) | (BLUE(pixel, 8) << 8));
}

void pixel2bgr_8880(void *dst, pixel_t pixel)
{
	*((uint32_t *) dst) = host2uint32_t_be(
	    (BLUE(pixel, 8) << 24) | (GREEN(pixel, 8) << 16) | (RED(pixel, 8) << 8));
}

void pixel2rgb_888(void *dst, pixel_t pixel)
{
	((uint8_t *) dst)[0] = RED(pixel, 8);
	((uint8_t *) dst)[1] = GREEN(pixel, 8);
	((uint8_t *) dst)[2] = BLUE(pixel, 8);
}

void pixel2bgr_888(void *dst, pixel_t pixel)
{
	((uint8_t *) dst)[0] = BLUE(pixel, 8);
	((uint8_t *) dst)[1] = GREEN(pixel, 8);
	((uint8_t *) dst)[2] = RED(pixel, 8);
}

void pixel2rgb_555_be(void *dst, pixel_t pixel)
{
	*((uint16_t *) dst) = host2uint16_t_be(
	    (RED(pixel, 5) << 10) | (GREEN(pixel, 5) << 5) | (BLUE(pixel, 5)));
}

void pixel2rgb_555_le(void *dst, pixel_t pixel)
{
	*((uint16_t *) dst) = host2uint16_t_le(
	    (RED(pixel, 5) << 10) | (GREEN(pixel, 5) << 5) | (BLUE(pixel, 5)));
}

void pixel2rgb_565_be(void *dst, pixel_t pixel)
{
	*((uint16_t *) dst) = host2uint16_t_be(
	    (RED(pixel, 5) << 11) | (GREEN(pixel, 6) << 5) | (BLUE(pixel, 5)));
}

void pixel2rgb_565_le(void *dst, pixel_t pixel)
{
	*((uint16_t *) dst) = host2uint16_t_le(
	    (RED(pixel, 5) << 11) | (GREEN(pixel, 6) << 5) | (BLUE(pixel, 5)));
}

void pixel2bgr_323(void *dst, pixel_t pixel)
{
	*((uint8_t *) dst) =
	    ~((RED(pixel, 3) << 5) | (GREEN(pixel, 2) << 3) | BLUE(pixel, 3));
}

void pixel2gray_8(void *dst, pixel_t pixel)
{
	uint32_t red = RED(pixel, 8) * 5034375;
	uint32_t green = GREEN(pixel, 8) * 9886846;
	uint32_t blue = BLUE(pixel, 8) * 1920103;
	
	*((uint8_t *) dst) = (red + green + blue) >> 24;
}

void visual_mask_0888(void *dst, bool mask)
{
	pixel2bgr_0888(dst, mask ? 0xffffff : 0);
}

void visual_mask_8880(void *dst, bool mask)
{
	pixel2bgr_8880(dst, mask ? 0xffffff : 0);
}

void visual_mask_888(void *dst, bool mask)
{
	pixel2bgr_888(dst, mask ? 0xffffff : 0);
}

void visual_mask_555(void *dst, bool mask)
{
	pixel2rgb_555_be(dst, mask ? 0xffffff : 0);
}

void visual_mask_565(void *dst, bool mask)
{
	pixel2rgb_565_be(dst, mask ? 0xffffff : 0);
}

void visual_mask_323(void *dst, bool mask)
{
	pixel2bgr_323(dst, mask ? 0x0 : ~0x0);
}

void visual_mask_8(void *dst, bool mask)
{
	pixel2gray_8(dst, mask ? 0xffffff : 0);
}

pixel_t rgb_0888_2pixel(void *src)
{
	return (uint32_t_be2host(*((uint32_t *) src)) & 0xffffff);
}

pixel_t bgr_0888_2pixel(void *src)
{
	uint32_t val = uint32_t_be2host(*((uint32_t *) src));
	return (((val & 0xff0000) >> 16) | (val & 0xff00) | ((val & 0xff) << 16));
}

pixel_t rgb_8880_2pixel(void *src)
{
	return (uint32_t_be2host(*((uint32_t *) src)) >> 8);
}

pixel_t bgr_8880_2pixel(void *src)
{
	uint32_t val = uint32_t_be2host(*((uint32_t *) src));
	return (((val & 0xff000000) >> 24) | ((val & 0xff0000) >> 8) | ((val & 0xff00) << 8));
}

pixel_t rgb_888_2pixel(void *src)
{
	uint8_t red = ((uint8_t *) src)[0];
	uint8_t green = ((uint8_t *) src)[1];
	uint8_t blue = ((uint8_t *) src)[2];
	
	return ((red << 16) | (green << 8) | (blue));
}

pixel_t bgr_888_2pixel(void *src)
{
	uint8_t blue = ((uint8_t *) src)[0];
	uint8_t green = ((uint8_t *) src)[1];
	uint8_t red = ((uint8_t *) src)[2];
	
	return ((red << 16) | (green << 8) | (blue));
}

pixel_t rgb_555_be_2pixel(void *src)
{
	uint16_t val = uint16_t_be2host(*((uint16_t *) src));
	return (((val & 0x7c00) << 9) | ((val & 0x3e0) << 6) | ((val & 0x1f) << 3));
}

pixel_t rgb_555_le_2pixel(void *src)
{
	uint16_t val = uint16_t_le2host(*((uint16_t *) src));
	return (((val & 0x7c00) << 9) | ((val & 0x3e0) << 6) | ((val & 0x1f) << 3));
}

pixel_t rgb_565_be_2pixel(void *src)
{
	uint16_t val = uint16_t_be2host(*((uint16_t *) src));
	return (((val & 0xf800) << 8) | ((val & 0x7e0) << 5) | ((val & 0x1f) << 3));
}

pixel_t rgb_565_le_2pixel(void *src)
{
	uint16_t val = uint16_t_le2host(*((uint16_t *) src));
	return (((val & 0xf800) << 8) | ((val & 0x7e0) << 5) | ((val & 0x1f) << 3));
}

pixel_t bgr_323_2pixel(void *src)
{
	uint8_t val = ~(*((uint8_t *) src));
	return (((val & 0xe0) << 16) | ((val & 0x18) << 11) | ((val & 0x7) << 5));
}

pixel_t gray_8_2pixel(void *src)
{
	uint8_t val = *((uint8_t *) src);
	return ((val << 16) | (val << 8) | (val));
}

typedef struct {
	uint8_t id_length;
	uint8_t cmap_type;
	uint8_t img_type;
	
	uint16_t cmap_first_entry;
	uint16_t cmap_entries;
	uint8_t cmap_bpp;
	
	uint16_t startx;
	uint16_t starty;
	uint16_t width;
	uint16_t height;
	uint8_t img_bpp;
	uint8_t img_descr;
} __attribute__((packed)) tga_header_t;

typedef enum {
	CMAP_NOT_PRESENT = 0,
	CMAP_PRESENT = 1,
	CMAP_RESERVED_START = 2,
	CMAP_PRIVATE_START = 128
} cmap_type_t;

typedef enum {
	IMG_EMTPY = 0,
	IMG_CMAP = 1,
	IMG_BGRA = 2,
	IMG_GRAY = 3,
	IMG_CMAP_RLE = 9,
	IMG_BGRA_RLE = 10,
	IMG_GRAY_RLE = 11
} img_type_t;

typedef struct {
	cmap_type_t cmap_type;
	img_type_t img_type;
	
	uint16_t cmap_first_entry;
	uint16_t cmap_entries;
	uint8_t cmap_bpp;
	
	uint16_t startx;
	uint16_t starty;
	uint16_t width;
	uint16_t height;
	uint8_t img_bpp;
	uint8_t img_alpha_bpp;
	uint8_t img_alpha_dir;
	
	void *id_data;
	size_t id_length;
	
	void *cmap_data;
	size_t cmap_length;
	
	void *img_data;
	size_t img_length;
} tga_t;

/** Decode Truevision TGA header
 *
 * @param[in]  data Memory representation of TGA.
 * @param[in]  size Size of the representation (in bytes).
 * @param[out] tga  Decoded TGA.
 *
 * @return True on succesful decoding.
 * @return False on failure.
 *
 */
static bool decode_tga_header(void *data, size_t size, tga_t *tga)
{
	/* Header sanity check */
	if (size < sizeof(tga_header_t))
		return false;
	
	tga_header_t *head = (tga_header_t *) data;
	
	/* Image ID field */
	tga->id_data = data + sizeof(tga_header_t);
	tga->id_length = head->id_length;
	
	if (size < sizeof(tga_header_t) + tga->id_length)
		return false;
	
	/* Color map type */
	tga->cmap_type = head->cmap_type;
	
	/* Image type */
	tga->img_type = head->img_type;
	
	/* Color map specification */
	tga->cmap_first_entry = uint16_t_le2host(head->cmap_first_entry);
	tga->cmap_entries = uint16_t_le2host(head->cmap_entries);
	tga->cmap_bpp = head->cmap_bpp;
	tga->cmap_data = tga->id_data + tga->id_length;
	tga->cmap_length = ALIGN_UP(tga->cmap_entries * tga->cmap_bpp, 8) >> 3;
	
	if (size < sizeof(tga_header_t) + tga->id_length +
	    tga->cmap_length)
		return false;
	
	/* Image specification */
	tga->startx = uint16_t_le2host(head->startx);
	tga->starty = uint16_t_le2host(head->starty);
	tga->width = uint16_t_le2host(head->width);
	tga->height = uint16_t_le2host(head->height);
	tga->img_bpp = head->img_bpp;
	tga->img_alpha_bpp = head->img_descr & 0x0f;
	tga->img_alpha_dir = (head->img_descr & 0xf0) >> 4;
	tga->img_data = tga->cmap_data + tga->cmap_length;
	tga->img_length = ALIGN_UP(tga->width * tga->height * tga->img_bpp, 8) >> 3;
	
	if (size < sizeof(tga_header_t) + tga->id_length +
	    tga->cmap_length + tga->img_length)
		return false;
	
	return true;
}

void imgmap_put_pixel(imgmap_t *imgmap, sysarg_t x, sysarg_t y, pixel_t pixel)
{
	if ((x >= imgmap->width) || (y >= imgmap->height))
		return;
	
	size_t offset = y * imgmap->width + x;
	
	switch (imgmap->visual) {
	case VISUAL_RGB_0_8_8_8:
		pixel2rgb_0888(((uint32_t *) imgmap->data) + offset, pixel);
		break;
	default:
		break;
	}
}

pixel_t imgmap_get_pixel(imgmap_t *imgmap, sysarg_t x, sysarg_t y)
{
	if ((x >= imgmap->width) || (y >= imgmap->height))
		return 0;
	
	size_t offset = y * imgmap->width + x;
	
	switch (imgmap->visual) {
	case VISUAL_RGB_0_8_8_8:
		return rgb_0888_2pixel(((uint32_t *) imgmap->data) + offset);
	default:
		return 0;
	}
}

imgmap_t *imgmap_create(sysarg_t width, sysarg_t height, visual_t visual,
    imgmap_flags_t flags)
{
	size_t bsize;
	
	switch (visual) {
	case VISUAL_RGB_0_8_8_8:
		bsize = (width * height) << 2;
		break;
	default:
		return NULL;
	}
	
	size_t size = sizeof(imgmap_t) + bsize;
	imgmap_t *imgmap;
	
	if ((flags & IMGMAP_FLAG_SHARED) == IMGMAP_FLAG_SHARED) {
		imgmap = (imgmap_t *) as_area_create(AS_AREA_ANY, size,
		    AS_AREA_READ | AS_AREA_WRITE);
		if (imgmap == AS_MAP_FAILED)
			return NULL;
	} else {
		imgmap = (imgmap_t *) malloc(size);
		if (imgmap == NULL)
			return NULL;
	}
	
	imgmap->size = size;
	imgmap->flags = flags;
	imgmap->width = width;
	imgmap->height = height;
	imgmap->visual = visual;
	
	memset(imgmap->data, 0, bsize);
	
	return imgmap;
}

/** Decode Truevision TGA format
 *
 * Decode Truevision TGA format and create an image map
 * from it. The supported variants of TGA are currently
 * limited to uncompressed 24 bit true-color images without
 * alpha channel.
 *
 * @param[in] data  Memory representation of TGA.
 * @param[in] size  Size of the representation (in bytes).
 * @param[in] flags Image map creation flags.
 *
 * @return Newly allocated image map.
 * @return NULL on error or unsupported format.
 *
 */
imgmap_t *imgmap_decode_tga(void *data, size_t size, imgmap_flags_t flags)
{
	tga_t tga;
	if (!decode_tga_header(data, size, &tga))
		return NULL;
	
	/*
	 * Check for unsupported features.
	 */
	
	switch (tga.cmap_type) {
	case CMAP_NOT_PRESENT:
		break;
	default:
		/* Unsupported */
		return NULL;
	}
	
	switch (tga.img_type) {
	case IMG_BGRA:
		if (tga.img_bpp != 24)
			return NULL;
		break;
	case IMG_GRAY:
		if (tga.img_bpp != 8)
			return NULL;
		break;
	default:
		/* Unsupported */
		return NULL;
	}
	
	if (tga.img_alpha_bpp != 0)
		return NULL;
	
	sysarg_t twidth = tga.startx + tga.width;
	sysarg_t theight = tga.starty + tga.height;
	
	imgmap_t *imgmap = imgmap_create(twidth, theight, VISUAL_RGB_0_8_8_8,
	    flags);
	if (imgmap == NULL)
		return NULL;
	
	/*
	 * TGA is encoded in a bottom-up manner, the true-color
	 * variant is in BGR 8:8:8 encoding.
	 */
	
	switch (tga.img_type) {
	case IMG_BGRA:
		for (sysarg_t y = tga.starty; y < theight; y++) {
			for (sysarg_t x = tga.startx; x < twidth; x++) {
				size_t offset =
				    ((y - tga.starty) * tga.width + (x - tga.startx)) * 3;
				
				pixel_t pixel =
				    bgr_888_2pixel(((uint8_t *) tga.img_data) + offset);
				imgmap_put_pixel(imgmap, x, theight - y - 1, pixel);
			}
		}
		break;
	case IMG_GRAY:
		for (sysarg_t y = tga.starty; y < theight; y++) {
			for (sysarg_t x = tga.startx; x < twidth; x++) {
				size_t offset =
				    (y - tga.starty) * tga.width + (x - tga.startx);
				
				pixel_t pixel =
				    gray_8_2pixel(((uint8_t *) tga.img_data) + offset);
				imgmap_put_pixel(imgmap, x, theight - y - 1, pixel);
			}
		}
		break;
	default:
		break;
	}
	
	return imgmap;
}

void imgmap_get_resolution(imgmap_t *imgmap, sysarg_t *width, sysarg_t *height)
{
	assert(width);
	assert(height);
	
	*width = imgmap->width;
	*height = imgmap->height;
}

/** @}
 */
