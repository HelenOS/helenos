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
