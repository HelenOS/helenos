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
 * @file Font types
 */

#ifndef _TYPES_GFX_FONT_H
#define _TYPES_GFX_FONT_H

#include <types/gfx/coord.h>

struct gfx_font;
typedef struct gfx_font gfx_font_t;

struct gfx_font_info;
typedef struct gfx_font_info gfx_font_info_t;

/** Font flags */
typedef enum {
	/** Bold */
	gff_bold = 0x1,
	/** Italic */
	gff_italic = 0x2,
	/** Bold, italic */
	gff_bold_italic = gff_bold | gff_italic,
	/** Text mode */
	gff_text_mode = 0x4
} gfx_font_flags_t;

/** Font properties */
typedef struct {
	/** Size */
	gfx_coord_t size;
	/** Flags */
	gfx_font_flags_t flags;
} gfx_font_props_t;

#endif

/** @}
 */
