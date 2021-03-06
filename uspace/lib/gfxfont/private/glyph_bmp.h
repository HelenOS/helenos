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
 * @file Glyph structure
 *
 */

#ifndef _GFX_PRIVATE_GLYPH_BMP_H
#define _GFX_PRIVATE_GLYPH_BMP_H

#include <gfx/coord.h>
#include <types/gfx/glyph_bmp.h>

/** Glyph bitmap
 *
 * Glyph bitmap open for editing. This is used to edit glyph bitmap.
 * Updating the entire font bitmap whenever the glyph is resized could
 * be costly. This allows to postpone the update until we are done editing.
 *
 * This is private to libgfxfont.
 */
struct gfx_glyph_bmp {
	/** Containing glyph */
	struct gfx_glyph *glyph;
	/** Rectangle covered by bitmap */
	gfx_rect_t rect;
	/** Pixel array */
	int *pixels;
};

extern void gfx_glyph_bmp_find_used_rect(gfx_glyph_bmp_t *, gfx_rect_t *);

#endif

/** @}
 */
