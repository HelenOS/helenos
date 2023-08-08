/*
 * Copyright (c) 2023 Jiri Svoboda
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
#include <stdbool.h>
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
extern bool ds_window_is_visible(ds_window_t *);
extern errno_t ds_window_paint(ds_window_t *, gfx_rect_t *);
errno_t ds_window_paint_preview(ds_window_t *, gfx_rect_t *);
extern errno_t ds_window_post_kbd_event(ds_window_t *, kbd_event_t *);
extern errno_t ds_window_post_pos_event(ds_window_t *, pos_event_t *);
extern errno_t ds_window_post_focus_event(ds_window_t *);
extern errno_t ds_window_post_unfocus_event(ds_window_t *);
extern void ds_window_move_req(ds_window_t *, gfx_coord2_t *, sysarg_t);
extern void ds_window_move(ds_window_t *, gfx_coord2_t *);
extern void ds_window_get_pos(ds_window_t *, gfx_coord2_t *);
extern void ds_window_get_max_rect(ds_window_t *, gfx_rect_t *);
extern void ds_window_resize_req(ds_window_t *, display_wnd_rsztype_t,
    gfx_coord2_t *, sysarg_t);
extern errno_t ds_window_resize(ds_window_t *, gfx_coord2_t *, gfx_rect_t *);
extern errno_t ds_window_minimize(ds_window_t *);
extern errno_t ds_window_unminimize(ds_window_t *);
extern errno_t ds_window_maximize(ds_window_t *);
extern errno_t ds_window_unmaximize(ds_window_t *);
extern void ds_window_calc_resize(ds_window_t *, gfx_coord2_t *,
    gfx_rect_t *);
extern errno_t ds_window_set_cursor(ds_window_t *, display_stock_cursor_t);
extern errno_t ds_window_set_caption(ds_window_t *, const char *);
extern ds_window_t *ds_window_find_next(ds_window_t *, display_wnd_flags_t);
extern ds_window_t *ds_window_find_prev(ds_window_t *, display_wnd_flags_t);
extern void ds_window_unfocus(ds_window_t *);
extern bool ds_window_orig_seat(ds_window_t *, sysarg_t);

#endif

/** @}
 */
