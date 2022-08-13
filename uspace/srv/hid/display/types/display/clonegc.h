/*
 * SPDX-FileCopyrightText: 2020 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup display
 * @{
 */
/**
 * @file Cloning graphic context
 */

#ifndef TYPES_DISPLAY_CLONEGC_H
#define TYPES_DISPLAY_CLONEGC_H

#include <adt/list.h>
#include <gfx/context.h>

/** Cloning graphic context.
 *
 * A graphic context that clones rendering to a number of GCs. We need
 * to clone every bitmap to every GC so we end up with a matrix-like
 * structure (made of linked lists).
 *
 * ds_clonegc_output_t x ds_clonegc_bitmap_t -> ds_clonegc_outbitmap_t.
 */
typedef struct {
	/** Graphic context */
	gfx_context_t *gc;
	/** Output GCs (of ds_clonegc_output_t) */
	list_t outputs;
	/** Bitmaps (of ds_clonegc_bitmap_t) */
	list_t bitmaps;
} ds_clonegc_t;

/** Clone GC output */
typedef struct {
	/** Containing clone GC */
	ds_clonegc_t *clonegc;
	/** Link to @c clonegc->outputs */
	link_t loutputs;
	/** Output GC */
	gfx_context_t *gc;
	/** Output bitmaps (of ds_clonegc_outbitmap_t) */
	list_t obitmaps;
} ds_clonegc_output_t;

/** Bitmap in cloning GC.
 *
 * Has a list of output bitmaps for all outputs.
 */
typedef struct {
	/** Containing clone GC */
	ds_clonegc_t *clonegc;
	/** Bitmap parameters */
	gfx_bitmap_params_t params;
	/** Bitmap allocation */
	gfx_bitmap_alloc_t alloc;
	/** Link to @c clonegc->bitmaps */
	link_t lbitmaps;
	/** Output bitmaps (of ds_clonegc_outbitmap_t) */
	list_t obitmaps;
} ds_clonegc_bitmap_t;

/** Output bitmap in cloning GC.
 *
 * It is contained in two linked lists. The list of all output bitmaps for
 * a particular output and the list of all output bitmaps for a particular
 * bitmap.
 */
typedef struct {
	/** Containing output */
	ds_clonegc_output_t *output;
	/** Containing bitmap */
	ds_clonegc_bitmap_t *bitmap;
	/** Link to @c output->obitmaps */
	link_t lobitmaps;
	/** Linkt ot @c bitmap->obitmaps */
	link_t lbbitmaps;
	/** Output-specific bitmap */
	gfx_bitmap_t *obitmap;
} ds_clonegc_outbitmap_t;

#endif

/** @}
 */
