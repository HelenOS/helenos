/*
 * SPDX-FileCopyrightText: 2022 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libui
 * @{
 */
/**
 * @file Painting routines
 */

#ifndef _UI_PAINT_H
#define _UI_PAINT_H

#include <errno.h>
#include <gfx/color.h>
#include <gfx/coord.h>
#include <types/ui/paint.h>
#include <types/ui/resource.h>

extern errno_t ui_paint_bevel(gfx_context_t *, gfx_rect_t *, gfx_color_t *,
    gfx_color_t *, gfx_coord_t, gfx_rect_t *);
extern void ui_paint_get_bevel_inside(gfx_context_t *, gfx_rect_t *,
    gfx_coord_t, gfx_rect_t *);
extern errno_t ui_paint_inset_frame(ui_resource_t *, gfx_rect_t *,
    gfx_rect_t *);
extern void ui_paint_get_inset_frame_inside(ui_resource_t *, gfx_rect_t *,
    gfx_rect_t *);
extern errno_t ui_paint_outset_frame(ui_resource_t *, gfx_rect_t *,
    gfx_rect_t *);
extern errno_t ui_paint_filled_circle(gfx_context_t *, gfx_coord2_t *,
    gfx_coord_t, ui_fcircle_part_t);
extern errno_t ui_paint_up_triangle(gfx_context_t *, gfx_coord2_t *,
    gfx_coord_t);
extern errno_t ui_paint_down_triangle(gfx_context_t *, gfx_coord2_t *,
    gfx_coord_t);
extern errno_t ui_paint_left_triangle(gfx_context_t *, gfx_coord2_t *,
    gfx_coord_t);
extern errno_t ui_paint_right_triangle(gfx_context_t *, gfx_coord2_t *,
    gfx_coord_t);
extern errno_t ui_paint_cross(gfx_context_t *, gfx_coord2_t *, gfx_coord_t,
    gfx_coord_t, gfx_coord_t);
extern errno_t ui_paint_maxicon(ui_resource_t *, gfx_coord2_t *, gfx_coord_t,
    gfx_coord_t);
extern errno_t ui_paint_unmaxicon(ui_resource_t *, gfx_coord2_t *, gfx_coord_t,
    gfx_coord_t, gfx_coord_t, gfx_coord_t);
extern errno_t ui_paint_text_box(ui_resource_t *, gfx_rect_t *,
    ui_box_style_t, gfx_color_t *);
extern errno_t ui_paint_text_hbrace(ui_resource_t *, gfx_rect_t *,
    ui_box_style_t, gfx_color_t *);
extern errno_t ui_paint_text_rect(ui_resource_t *, gfx_rect_t *, gfx_color_t *,
    const char *);
extern void ui_text_fmt_init(ui_text_fmt_t *);
extern gfx_coord_t ui_text_width(gfx_font_t *, const char *);
extern errno_t ui_paint_text(gfx_coord2_t *, ui_text_fmt_t *, const char *);

#endif

/** @}
 */
