/*
 * Copyright (c) 2021 Jiri Svoboda
 * Copyright (c) 2013 Jan Vesely
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

/** @addtogroup amdm37x_dispc
 * @{
 */
/**
 * @file
 */

#ifndef AMDM37X_DISPC_H_
#define AMDM37X_DISPC_H_

#include <abi/fb/visuals.h>
#include <ddev_srv.h>
#include <ddf/driver.h>
#include <gfx/context.h>
#include <gfx/coord.h>
#include <io/pixel.h>
#include <pixconv.h>
#include <ddi.h>

#include "amdm37x_dispc_regs.h"

typedef struct {
	ddf_fun_t *fun;
	amdm37x_dispc_regs_t *regs;

	struct {
		pixel2visual_t pixel2visual;
		unsigned width;
		unsigned height;
		unsigned pitch;
		unsigned bpp;
	} active_fb;

	pixel_t color;
	gfx_rect_t rect;
	gfx_rect_t clip_rect;
	size_t size;
	void *fb_data;
} amdm37x_dispc_t;

typedef struct {
	/* Containing display controller */
	amdm37x_dispc_t *dispc;
	/** Allocation info */
	gfx_bitmap_alloc_t alloc;
	/** @c true if we allocated the bitmap, @c false if allocated by caller */
	bool myalloc;
	/** Rectangle covered by bitmap */
	gfx_rect_t rect;
	/** Bitmap flags */
	gfx_bitmap_flags_t flags;
	/** Key color */
	pixel_t key_color;
} amdm37x_bitmap_t;

extern ddev_ops_t amdm37x_ddev_ops;
extern gfx_context_ops_t amdm37x_gc_ops;

extern errno_t amdm37x_dispc_init(amdm37x_dispc_t *, ddf_fun_t *);
extern errno_t amdm37x_dispc_fini(amdm37x_dispc_t *);

#endif
/** @}
 */
