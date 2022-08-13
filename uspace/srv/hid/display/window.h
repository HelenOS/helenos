/*
 * SPDX-FileCopyrightText: 2022 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup display
 * @{
 */
/**
 * @file Display server window
 */

#ifndef WINDOW_H
#define WINDOW_H

#include <display/wndparams.h>
#include <display/wndresize.h>
#include <errno.h>
#include <io/pos_event.h>
#include <types/gfx/context.h>
#include <types/gfx/coord.h>
#include <types/gfx/ops/context.h>
#include "types/display/cursor.h"
#include "types/display/display.h"
#include "types/display/window.h"

extern gfx_context_ops_t window_gc_ops;

extern errno_t ds_window_create(ds_client_t *, display_wnd_params_t *,
    ds_window_t **);
extern void ds_window_destroy(ds_window_t *);
extern void ds_window_bring_to_top(ds_window_t *);
extern gfx_context_t *ds_window_get_ctx(ds_window_t *);
extern errno_t ds_window_paint(ds_window_t *, gfx_rect_t *);
errno_t ds_window_paint_preview(ds_window_t *, gfx_rect_t *);
extern errno_t ds_window_post_kbd_event(ds_window_t *, kbd_event_t *);
extern errno_t ds_window_post_pos_event(ds_window_t *, pos_event_t *);
extern errno_t ds_window_post_focus_event(ds_window_t *);
extern errno_t ds_window_post_unfocus_event(ds_window_t *);
extern void ds_window_move_req(ds_window_t *, gfx_coord2_t *);
extern void ds_window_move(ds_window_t *, gfx_coord2_t *);
extern void ds_window_get_pos(ds_window_t *, gfx_coord2_t *);
extern void ds_window_get_max_rect(ds_window_t *, gfx_rect_t *);
extern void ds_window_resize_req(ds_window_t *, display_wnd_rsztype_t,
    gfx_coord2_t *);
extern errno_t ds_window_resize(ds_window_t *, gfx_coord2_t *, gfx_rect_t *);
extern errno_t ds_window_maximize(ds_window_t *);
extern errno_t ds_window_unmaximize(ds_window_t *);
extern void ds_window_calc_resize(ds_window_t *, gfx_coord2_t *,
    gfx_rect_t *);
extern errno_t ds_window_set_cursor(ds_window_t *, display_stock_cursor_t);

#endif

/** @}
 */
