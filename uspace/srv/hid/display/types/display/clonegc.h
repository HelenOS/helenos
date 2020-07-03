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
