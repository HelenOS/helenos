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

/** @addtogroup draw
 * @{
 */
/**
 * @file
 */

#include <stdlib.h>
#include <byteorder.h>
#include <align.h>
#include <stdbool.h>
#include <pixconv.h>
#include "tga.h"

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

/** Decode Truevision TGA format
 *
 * Decode Truevision TGA format and create a surface
 * from it. The supported variants of TGA are currently
 * limited to uncompressed 24 bit true-color images without
 * alpha channel.
 *
 * @param[in] data  Memory representation of TGA.
 * @param[in] size  Size of the representation (in bytes).
 * @param[in] flags Surface creation flags.
 *
 * @return Newly allocated surface with the decoded content.
 * @return NULL on error or unsupported format.
 *
 */
surface_t *decode_tga(void *data, size_t size, surface_flags_t flags)
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

	surface_t *surface = surface_create(twidth, theight, NULL, flags);
	if (surface == NULL)
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
				surface_put_pixel(surface, x, theight - y - 1, pixel);
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
				surface_put_pixel(surface, x, theight - y - 1, pixel);
			}
		}
		break;
	default:
		break;
	}

	return surface;
}

/** Encode Truevision TGA format
 *
 * Encode Truevision TGA format into an array.
 *
 * @param[in]  surface Surface to be encoded into TGA.
 * @param[out] pdata   Pointer to the resulting array.
 * @param[out] psize   Pointer to the size of the resulting array.
 *
 * @return True on succesful encoding.
 * @return False on failure.
 *
 */
bool encode_tga(surface_t *surface, void **pdata, size_t *psize)
{
	// TODO
	return false;
}

/** @}
 */
