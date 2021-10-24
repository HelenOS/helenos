/*
 * Copyright (c) 2021 Jiri Svoboda
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
