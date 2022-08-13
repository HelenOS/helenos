/*
 * SPDX-FileCopyrightText: 2021 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
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
