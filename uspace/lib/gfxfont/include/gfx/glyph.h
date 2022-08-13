/*
 * SPDX-FileCopyrightText: 2020 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
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
