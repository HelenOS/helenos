/*
 * SPDX-FileCopyrightText: 2021 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libmemgfx
 * @{
 */
/**
 * @file Translating graphics context structure
 *
 */

#ifndef _MEMGFX_PRIVATE_XLATEGC_H
#define _MEMGFX_PRIVATE_XLATEGC_H

#include <gfx/bitmap.h>
#include <gfx/context.h>
#include <gfx/coord.h>
#include <io/pixel.h>

/** Actual structure of translating GC. */
struct xlate_gc {
	/** Graphic context */
	gfx_context_t *gc;
	/** Backing graphic context */
	gfx_context_t *bgc;
	/** Translation offset */
	gfx_coord2_t off;
};

/** Bitmap in translating GC */
typedef struct {
	/** Containing translating GC */
	struct xlate_gc *xgc;
	/** Base bitmap in base GC */
	gfx_bitmap_t *bm;
} xlate_gc_bitmap_t;

#endif

/** @}
 */
