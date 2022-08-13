/*
 * SPDX-FileCopyrightText: 2020 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libgfxfont
 * @{
 */
/**
 * @file Glyph types
 */

#ifndef _TYPES_GFX_GLYPH_H
#define _TYPES_GFX_GLYPH_H

#include <types/gfx/coord.h>

struct gfx_glyph;
typedef struct gfx_glyph gfx_glyph_t;

struct gfx_glyph_pattern;
typedef struct gfx_glyph_pattern gfx_glyph_pattern_t;

/** Glyph metrics */
typedef struct {
	/** Advance */
	gfx_coord_t advance;
} gfx_glyph_metrics_t;

#endif

/** @}
 */
