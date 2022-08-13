/*
 * SPDX-FileCopyrightText: 2021 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libgfx
 * @{
 */
/**
 * @file Graphics context ops
 *
 * The ops structure describing an implementation of a graphics context.
 */

#ifndef _GFX_TYPES_OPS_CONTEXT_H
#define _GFX_TYPES_OPS_CONTEXT_H

#include <errno.h>
#include <types/gfx/bitmap.h>
#include <types/gfx/color.h>
#include <types/gfx/coord.h>
#include <types/gfx/context.h>
#include <stdbool.h>

/** Graphics context ops */
typedef struct {
	/** Set clipping rectangle */
	errno_t (*set_clip_rect)(void *, gfx_rect_t *);
	/** Set drawing color */
	errno_t (*set_color)(void *, gfx_color_t *);
	/** Fill rectangle using the current drawing color */
	errno_t (*fill_rect)(void *, gfx_rect_t *);
	/** Update display */
	errno_t (*update)(void *);
	/** Create bitmap */
	errno_t (*bitmap_create)(void *, gfx_bitmap_params_t *,
	    gfx_bitmap_alloc_t *, void **);
	/** Destroy bitmap */
	errno_t (*bitmap_destroy)(void *);
	/** Render bitmap */
	errno_t (*bitmap_render)(void *, gfx_rect_t *, gfx_coord2_t *);
	/** Get bitmap allocation info */
	errno_t (*bitmap_get_alloc)(void *, gfx_bitmap_alloc_t *);
	/** Get hardware cursor position */
	errno_t (*cursor_get_pos)(void *, gfx_coord2_t *);
	/** Set hardware cursor position */
	errno_t (*cursor_set_pos)(void *, gfx_coord2_t *);
	/** Set hardware cursor visibility */
	errno_t (*cursor_set_visible)(void *, bool);
} gfx_context_ops_t;

#endif

/** @}
 */
