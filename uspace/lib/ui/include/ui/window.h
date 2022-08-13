/*
 * SPDX-FileCopyrightText: 2022 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libui
 * @{
 */
/**
 * @file Window
 */

#ifndef _UI_WINDOW_H
#define _UI_WINDOW_H

#include <errno.h>
#include <gfx/context.h>
#include <gfx/coord.h>
#include <io/kbd_event.h>
#include <io/pos_event.h>
#include <types/ui/control.h>
#include <types/ui/ui.h>
#include <types/ui/resource.h>
#include <types/ui/window.h>

extern void ui_wnd_params_init(ui_wnd_params_t *);
extern errno_t ui_window_create(ui_t *, ui_wnd_params_t *,
    ui_window_t **);
extern void ui_window_set_cb(ui_window_t *, ui_window_cb_t *, void *);
extern errno_t ui_window_set_caption(ui_window_t *, const char *);
extern void ui_window_destroy(ui_window_t *);
extern void ui_window_add(ui_window_t *, ui_control_t *);
extern void ui_window_remove(ui_window_t *, ui_control_t *);
extern ui_window_t *ui_window_get_active(ui_t *);
extern errno_t ui_window_resize(ui_window_t *, gfx_rect_t *);
extern ui_t *ui_window_get_ui(ui_window_t *);
extern ui_resource_t *ui_window_get_res(ui_window_t *);
extern gfx_context_t *ui_window_get_gc(ui_window_t *);
extern errno_t ui_window_get_pos(ui_window_t *, gfx_coord2_t *);
extern errno_t ui_window_get_app_gc(ui_window_t *, gfx_context_t **);
extern void ui_window_get_app_rect(ui_window_t *, gfx_rect_t *);
extern void ui_window_set_ctl_cursor(ui_window_t *, ui_stock_cursor_t);
extern errno_t ui_window_paint(ui_window_t *);
extern errno_t ui_window_def_maximize(ui_window_t *);
extern errno_t ui_window_def_unmaximize(ui_window_t *);
extern ui_evclaim_t ui_window_def_kbd(ui_window_t *, kbd_event_t *);
extern errno_t ui_window_def_paint(ui_window_t *);
extern void ui_window_def_pos(ui_window_t *, pos_event_t *);
extern void ui_window_def_unfocus(ui_window_t *);

#endif

/** @}
 */
