/*
 * SPDX-FileCopyrightText: 2014 Martin Decky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libgfximage
 * @{
 */
/**
 * @file
 */

#include <errno.h>
#include <gzip.h>
#include <stdlib.h>
#include <gfximage/tga.h>
#include <gfximage/tga_gz.h>

/** Decode gzipped Truevision TGA format
 *
 * Decode gzipped Truevision TGA format and create a bitmap
 * from it. The supported variants of TGA are limited those
 * supported by decode_tga().
 *
 * @param gc      Graphic context
 * @param data    Memory representation of gzipped TGA.
 * @param size    Size of the representation (in bytes).
 * @param rbitmap Place to store pointer to new bitmap
 * @param rrect   Place to store bitmap rectangle
 *
 * @return EOK un success or an error code
 */
errno_t decode_tga_gz(gfx_context_t *gc, void *data, size_t size,
    gfx_bitmap_t **rbitmap, gfx_rect_t *rrect)
{
	void *data_expanded;
	size_t size_expanded;
	errno_t rc;

	rc = gzip_expand(data, size, &data_expanded, &size_expanded);
	if (rc != EOK)
		return rc;

	rc = decode_tga(gc, data_expanded, size_expanded, rbitmap, rrect);
	free(data_expanded);
	return rc;
}

/** @}
 */
