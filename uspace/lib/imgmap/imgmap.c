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
#include "imgmap.h"

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
 * Decode Truevision TGA format and create an image map
 * from it. The supported variants of TGA are currently
 * limited to uncompressed 24 bit true-color images without
 * alpha channel.
 *
 * @param[in] data Memory representation of TGA.
 * @param[in] size Size of the representation (in bytes).
 *
 * @return Newly allocated image map.
 * @return NULL on error or unsupported format.
 *
 */
imgmap_t *imgmap_decode_tga(void *data, size_t size)
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
	size_t bsize = twidth * theight * 3;
	
	imgmap_t *imgmap = (imgmap_t *) malloc(sizeof(imgmap_t) + bsize);
	if (imgmap == NULL)
		return NULL;
	
	imgmap->size = sizeof(imgmap_t) + bsize;
	imgmap->width = twidth;
	imgmap->height = theight;
	imgmap->visual = VISUAL_BGR_8_8_8;
	
	memset(imgmap->data, 0, bsize);
	
	/*
	 * TGA is encoded in a bottom-up manner.
	 */
	
	switch (tga.img_type) {
	case IMG_BGRA:
		for (sysarg_t y = tga.starty; y < theight; y++) {
			size_t src_offset = (y - tga.starty) * tga.width * 3;
			size_t dst_offset = ((theight - y - 1) * twidth + tga.startx) * 3;
			
			memcpy(imgmap->data + dst_offset, tga.img_data + src_offset,
			    tga.width * 3);
		}
		break;
	case IMG_GRAY:
		for (sysarg_t y = tga.starty; y < theight; y++) {
			for (sysarg_t x = tga.startx; x < twidth; x++) {
				uint8_t val =
				    ((uint8_t *) tga.img_data)[(y - tga.starty) * tga.width + (x - tga.startx)];
				size_t dst_offset = ((theight - y - 1) * twidth + x) * 3;
				memset(imgmap->data + dst_offset, val, 3);
			}
		}
		break;
	default:
		break;
	}
	
	return imgmap;
}

/** @}
 */
