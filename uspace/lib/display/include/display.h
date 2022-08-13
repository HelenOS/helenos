/*
 * SPDX-FileCopyrightText: 2022 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libdisplay
 * @{
 */
/** @file
 */

#ifndef _LIBDISPLAY_DISPLAY_H_
#define _LIBDISPLAY_DISPLAY_H_

#include <errno.h>
#include <gfx/context.h>
#include <gfx/coord.h>
#include <stdbool.h>
#include "display/wndparams.h"
#include "display/wndresize.h"
#include "types/display.h"
#include "types/display/cursor.h"
#include "types/display/info.h"

extern errno_t display_open(const char *, display_t **);
extern void display_close(display_t *);
extern errno_t display_get_info(display_t *, display_info_t *);

extern errno_t display_window_create(display_t *, display_wnd_params_t *,
    display_wnd_cb_t *, void *, display_window_t **);
extern errno_t display_window_destroy(display_window_t *);
extern errno_t display_window_get_gc(display_window_t *, gfx_context_t **);
extern errno_t display_window_move_req(display_window_t *, gfx_coord2_t *);
extern errno_t display_window_resize_req(display_window_t *,
    display_wnd_rsztype_t, gfx_coord2_t *);
extern errno_t display_window_move(display_window_t *, gfx_coord2_t *);
extern errno_t display_window_get_pos(display_window_t *, gfx_coord2_t *);
extern errno_t display_window_get_max_rect(display_window_t *, gfx_rect_t *);
extern errno_t display_window_resize(display_window_t *,
    gfx_coord2_t *, gfx_rect_t *);
extern errno_t display_window_maximize(display_window_t *);
extern errno_t display_window_unmaximize(display_window_t *);
extern errno_t display_window_set_cursor(display_window_t *,
    display_stock_cursor_t);

#endif

/** @}
 */
