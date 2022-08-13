/*
 * SPDX-FileCopyrightText: 2020 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libgfxfont
 * @{
 */
/**
 * @file Typeface structure
 *
 */

#ifndef _GFX_PRIVATE_TYPEFACE_H
#define _GFX_PRIVATE_TYPEFACE_H

#include <adt/list.h>
#include <errno.h>
#include <riff/chunk.h>
#include <types/gfx/bitmap.h>
#include <types/gfx/context.h>
#include <types/gfx/typeface.h>

/** Typeface
 *
 * A typeface can contain several fonts of varying size and attributes
 * (such as bold, italic). Fonts present in a typeface can be enumerated
 * without actually loading them into memory.
 *
 * This is private to libgfxfont.
 */
struct gfx_typeface {
	/** Graphics context of the typeface */
	gfx_context_t *gc;
	/** Fonts (of gfx_font_info_t) */
	list_t fonts;
	/** RIFF reader of the open typeface file or @c NULL */
	riffr_t *riffr;
};

#endif

/** @}
 */
