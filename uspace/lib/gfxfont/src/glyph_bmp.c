/*
 * Copyright (c) 2020 Jiri Svoboda
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

/** @addtogroup libgfxfont
 * @{
 */
/**
 * @file Glyph bitmap
 */

#include <errno.h>
#include <gfx/glyph_bmp.h>
#include <stdlib.h>
#include "../private/glyph_bmp.h"

/** Save glyph bitmap.
 *
 * @param bmp Glyph bitmap
 * @return EOK on success, ENOMEM if out of memory
 */
errno_t gfx_glyph_bmp_save(gfx_glyph_bmp_t *bmp)
{
	return EOK;
}

/** Close glyph bitmap.
 *
 * @param bmp Glyph bitmap
 */
void gfx_glyph_bmp_close(gfx_glyph_bmp_t *bmp)
{
}

/** Get pixel from glyph bitmap.
 *
 * @param bmp Glyph bitmap
 * @param x X-coordinate
 * @param y Y-coordinate
 * @return Pixel value
 */
int gfx_glyph_bmp_getpix(gfx_glyph_bmp_t *bmp, gfx_coord_t x, gfx_coord_t y)
{
	return 0;
}

/** Set pixel in glyph bitmap.
 *
 * @param bmp Glyph bitmap
 * @param x X-coordinate
 * @param y Y-coordinate
 *
 * @reutrn EOK on success, ENOMEM if out of memory
 */
errno_t gfx_glyph_bmp_setpix(gfx_glyph_bmp_t *bmp, gfx_coord_t x,
    gfx_coord_t y, int value)
{
	return EOK;
}

/** @}
 */
