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
 * @file Font
 */

#ifndef _GFX_FONT_H
#define _GFX_FONT_H

#include <errno.h>
#include <stddef.h>
#include <types/gfx/context.h>
#include <types/gfx/font.h>
#include <types/gfx/glyph.h>
#include <types/gfx/text.h>
#include <types/gfx/typeface.h>

extern void gfx_font_metrics_init(gfx_font_metrics_t *);
extern void gfx_font_props_init(gfx_font_props_t *);
extern void gfx_font_get_props(gfx_font_info_t *, gfx_font_props_t *);
extern errno_t gfx_font_create(gfx_typeface_t *, gfx_font_props_t *,
    gfx_font_metrics_t *, gfx_font_t **);
extern errno_t gfx_font_create_textmode(gfx_typeface_t *, gfx_font_t **);
extern errno_t gfx_font_open(gfx_font_info_t *, gfx_font_t **);
extern void gfx_font_close(gfx_font_t *);
extern void gfx_font_get_metrics(gfx_font_t *, gfx_font_metrics_t *);
extern errno_t gfx_font_set_metrics(gfx_font_t *,
    gfx_font_metrics_t *);
extern gfx_glyph_t *gfx_font_first_glyph(gfx_font_t *);
extern gfx_glyph_t *gfx_font_next_glyph(gfx_glyph_t *);
extern gfx_glyph_t *gfx_font_last_glyph(gfx_font_t *);
extern gfx_glyph_t *gfx_font_prev_glyph(gfx_glyph_t *);
extern errno_t gfx_font_search_glyph(gfx_font_t *, const char *, gfx_glyph_t **,
    size_t *);

#endif

/** @}
 */
