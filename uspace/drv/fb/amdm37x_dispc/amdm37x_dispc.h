/*
 * SPDX-FileCopyrightText: 2021 Jiri Svoboda
 * SPDX-FileCopyrightText: 2013 Jan Vesely
 *
 * SPDX-License-Identifier: BSD-3-Clause
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
