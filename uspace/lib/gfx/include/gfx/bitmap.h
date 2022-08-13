/*
 * SPDX-FileCopyrightText: 2019 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libgfx
 * @{
 */
/**
 * @file Bitmap
 */

#ifndef _GFX_BITMAP_H
#define _GFX_BITMAP_H

#include <errno.h>
#include <types/gfx/context.h>
#include <types/gfx/coord.h>
#include <types/gfx/bitmap.h>

extern void gfx_bitmap_params_init(gfx_bitmap_params_t *);
extern errno_t gfx_bitmap_create(gfx_context_t *, gfx_bitmap_params_t *,
    gfx_bitmap_alloc_t *, gfx_bitmap_t **);
extern errno_t gfx_bitmap_destroy(gfx_bitmap_t *);
extern errno_t gfx_bitmap_render(gfx_bitmap_t *, gfx_rect_t *, gfx_coord2_t *);
extern errno_t gfx_bitmap_get_alloc(gfx_bitmap_t *, gfx_bitmap_alloc_t *);

#endif

/** @}
 */
