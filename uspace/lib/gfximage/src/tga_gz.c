/*
 * Copyright (c) 2014 Martin Decky
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
