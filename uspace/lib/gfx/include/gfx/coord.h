/*
 * SPDX-FileCopyrightText: 2019 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libgfx
 * @{
 */
/**
 * @file Graphic coordinates
 */

#ifndef _GFX_COORD_H
#define _GFX_COORD_H

#include <stdbool.h>
#include <types/gfx/coord.h>

extern gfx_coord_t gfx_coord_div_rneg(gfx_coord_t, gfx_coord_t);
extern void gfx_coord2_add(gfx_coord2_t *, gfx_coord2_t *, gfx_coord2_t *);
extern void gfx_coord2_subtract(gfx_coord2_t *, gfx_coord2_t *, gfx_coord2_t *);
extern void gfx_coord2_clip(gfx_coord2_t *, gfx_rect_t *, gfx_coord2_t *);
extern void gfx_coord2_project(gfx_coord2_t *, gfx_rect_t *, gfx_rect_t *,
    gfx_coord2_t *);
extern void gfx_span_points_sort(gfx_coord_t, gfx_coord_t, gfx_coord_t *,
    gfx_coord_t *);
extern void gfx_rect_translate(gfx_coord2_t *, gfx_rect_t *, gfx_rect_t *);
extern void gfx_rect_rtranslate(gfx_coord2_t *, gfx_rect_t *, gfx_rect_t *);
extern void gfx_rect_envelope(gfx_rect_t *, gfx_rect_t *, gfx_rect_t *);
extern void gfx_rect_clip(gfx_rect_t *, gfx_rect_t *, gfx_rect_t *);
extern void gfx_rect_ctr_on_rect(gfx_rect_t *, gfx_rect_t *, gfx_rect_t *);
extern void gfx_rect_points_sort(gfx_rect_t *, gfx_rect_t *);
extern void gfx_rect_dims(gfx_rect_t *, gfx_coord2_t *);
extern bool gfx_rect_is_empty(gfx_rect_t *);
extern bool gfx_rect_is_incident(gfx_rect_t *, gfx_rect_t *);
extern bool gfx_rect_is_inside(gfx_rect_t *, gfx_rect_t *);
extern bool gfx_pix_inside_rect(gfx_coord2_t *, gfx_rect_t *);

#endif

/** @}
 */
