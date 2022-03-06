/*
 * Copyright (c) 2022 Jiri Svoboda
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
 * @file Font types
 */

#ifndef _TYPES_GFX_TYPEFACE_H
#define _TYPES_GFX_TYPEFACE_H

#include <types/gfx/coord.h>

struct gfx_typeface;
typedef struct gfx_typeface gfx_typeface_t;

/** Font metrics */
typedef struct {
	/** Ascent */
	gfx_coord_t ascent;
	/** Descent */
	gfx_coord_t descent;
	/** Leading */
	gfx_coord_t leading;
	/** Underline start Y coordinate (inclusive) */
	gfx_coord_t underline_y0;
	/** Underline end Y coordinate (exclusive) */
	gfx_coord_t underline_y1;
} gfx_font_metrics_t;

/** Text metrics */
typedef struct {
	/** Bounding rectangle (not including oversize elements) */
	gfx_rect_t bounds;
} gfx_text_metrics_t;

#endif

/** @}
 */
