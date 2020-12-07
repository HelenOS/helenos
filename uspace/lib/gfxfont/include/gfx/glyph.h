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
 * @file Glyph
 */

#ifndef _GFX_GLYPH_H
#define _GFX_GLYPH_H

#include <errno.h>
#include <types/gfx/font.h>
#include <types/gfx/glyph.h>
#include <stdbool.h>
#include <stddef.h>

extern void gfx_glyph_metrics_init(gfx_glyph_metrics_t *);
extern errno_t gfx_glyph_create(gfx_font_t *, gfx_glyph_metrics_t *,
    gfx_glyph_t **);
extern void gfx_glyph_destroy(gfx_glyph_t *);
extern void gfx_glyph_get_metrics(gfx_glyph_t *, gfx_glyph_metrics_t *);
extern errno_t gfx_glyph_set_metrics(gfx_glyph_t *, gfx_glyph_metrics_t *);
extern errno_t gfx_glyph_set_pattern(gfx_glyph_t *, const char *);
extern void gfx_glyph_clear_pattern(gfx_glyph_t *, const char *);
extern bool gfx_glyph_matches(gfx_glyph_t *, const char *, size_t *);
extern gfx_glyph_pattern_t *gfx_glyph_first_pattern(gfx_glyph_t *);
extern gfx_glyph_pattern_t *gfx_glyph_next_pattern(gfx_glyph_pattern_t *);
extern const char *gfx_glyph_pattern_str(gfx_glyph_pattern_t *);
extern errno_t gfx_glyph_render(gfx_glyph_t *, gfx_coord2_t *);

#endif

/** @}
 */
