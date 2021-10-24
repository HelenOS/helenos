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
extern errno_t ui_paint_text_box(ui_resource_t *, gfx_rect_t *,
    ui_box_style_t, gfx_color_t *);
extern errno_t ui_paint_text_hbrace(ui_resource_t *, gfx_rect_t *,
    ui_box_style_t, gfx_color_t *);

#endif

/** @}
 */
