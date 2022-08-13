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

#ifndef _MEMGFX_MEMGC_H
#define _MEMGFX_MEMGC_H

#include <errno.h>
#include <types/gfx/bitmap.h>
#include <types/gfx/context.h>
#include <types/gfx/ops/context.h>
#include <types/memgfx/memgc.h>

extern gfx_context_ops_t mem_gc_ops;

extern errno_t mem_gc_create(gfx_rect_t *, gfx_bitmap_alloc_t *,
    mem_gc_cb_t *, void *, mem_gc_t **);
extern errno_t mem_gc_delete(mem_gc_t *);
extern void mem_gc_retarget(mem_gc_t *, gfx_rect_t *, gfx_bitmap_alloc_t *);
extern gfx_context_t *mem_gc_get_ctx(mem_gc_t *);

#endif

/** @}
 */
