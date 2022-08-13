/*
 * SPDX-FileCopyrightText: 2020 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
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
