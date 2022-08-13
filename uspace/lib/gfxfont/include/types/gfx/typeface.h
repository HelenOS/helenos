/*
 * SPDX-FileCopyrightText: 2022 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
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
