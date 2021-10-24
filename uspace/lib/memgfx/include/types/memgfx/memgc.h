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

/** @addtogroup libmemgfx
 * @{
 */
/**
 * @file GFX memory backend
 */

#ifndef _MEMGFX_TYPES_MEMGC_H
#define _MEMGFX_TYPES_MEMGC_H

#include <errno.h>
#include <stdbool.h>
#include <types/gfx/coord.h>

struct mem_gc;
typedef struct mem_gc mem_gc_t;

typedef struct {
	/** Invalidate rectangle */
	void (*invalidate)(void *, gfx_rect_t *);
	/** Update display */
	void (*update)(void *);
	/** Get cursor position */
	errno_t (*cursor_get_pos)(void *, gfx_coord2_t *);
	/** Set cursor position */
	errno_t (*cursor_set_pos)(void *, gfx_coord2_t *);
	/** Set cursor visibility */
	errno_t (*cursor_set_visible)(void *, bool);
} mem_gc_cb_t;

#endif

/** @}
 */
