/*
 * SPDX-FileCopyrightText: 2020 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
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
