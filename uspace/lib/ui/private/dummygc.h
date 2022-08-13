/*
 * SPDX-FileCopyrightText: 2020 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libui
 * @{
 */
/**
 * @file Dummy graphic context
 *
 */

#ifndef _UI_PRIVATE_DUMMYGC_H
#define _UI_PRIVATE_DUMMYGC_H

#include <gfx/context.h>

/** Dummy graphic context */
typedef struct {
	gfx_context_t *gc;
	bool bm_created;
	bool bm_destroyed;
	gfx_bitmap_params_t bm_params;
	void *bm_pixels;
	gfx_rect_t bm_srect;
	gfx_coord2_t bm_offs;
	bool bm_rendered;
	bool bm_got_alloc;
} dummy_gc_t;

/** Dummy GC bitmap */
typedef struct {
	dummy_gc_t *dgc;
	gfx_bitmap_alloc_t alloc;
	bool myalloc;
} dummygc_bitmap_t;

/** Dummy GC ops */
extern gfx_context_ops_t dummygc_ops;

extern errno_t dummygc_create(dummy_gc_t **);
extern void dummygc_destroy(dummy_gc_t *);
extern gfx_context_t *dummygc_get_ctx(dummy_gc_t *);

#endif

/** @}
 */
