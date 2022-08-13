/*
 * SPDX-FileCopyrightText: 2022 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libui
 * @{
 */
/**
 * @file Scrollbar
 */

#ifndef _UI_SCROLLBAR_H
#define _UI_SCROLLBAR_H

#include <errno.h>
#include <gfx/coord.h>
#include <io/pos_event.h>
#include <stdbool.h>
#include <types/ui/control.h>
#include <types/ui/event.h>
#include <types/ui/scrollbar.h>
#include <types/ui/ui.h>
#include <types/ui/window.h>

extern errno_t ui_scrollbar_create(ui_t *, ui_window_t *, ui_scrollbar_dir_t,
    ui_scrollbar_t **);
extern void ui_scrollbar_destroy(ui_scrollbar_t *);
extern ui_control_t *ui_scrollbar_ctl(ui_scrollbar_t *);
extern void ui_scrollbar_set_cb(ui_scrollbar_t *, ui_scrollbar_cb_t *, void *);
extern void ui_scrollbar_set_rect(ui_scrollbar_t *, gfx_rect_t *);
extern errno_t ui_scrollbar_paint(ui_scrollbar_t *);
extern gfx_coord_t ui_scrollbar_through_length(ui_scrollbar_t *);
extern gfx_coord_t ui_scrollbar_move_length(ui_scrollbar_t *);
extern gfx_coord_t ui_scrollbar_get_pos(ui_scrollbar_t *);
extern void ui_scrollbar_set_thumb_length(ui_scrollbar_t *, gfx_coord_t);
extern void ui_scrollbar_set_pos(ui_scrollbar_t *, gfx_coord_t);
extern void ui_scrollbar_thumb_press(ui_scrollbar_t *, gfx_coord2_t *);
extern void ui_scrollbar_up_through_press(ui_scrollbar_t *);
extern void ui_scrollbar_down_through_press(ui_scrollbar_t *);
extern void ui_scrollbar_release(ui_scrollbar_t *, gfx_coord2_t *);
extern void ui_scrollbar_update(ui_scrollbar_t *, gfx_coord2_t *);
extern void ui_scrollbar_throughs_update(ui_scrollbar_t *, gfx_coord2_t *);
extern void ui_scrollbar_up(ui_scrollbar_t *);
extern void ui_scrollbar_down(ui_scrollbar_t *);
extern void ui_scrollbar_page_up(ui_scrollbar_t *);
extern void ui_scrollbar_page_down(ui_scrollbar_t *);
extern void ui_scrollbar_moved(ui_scrollbar_t *, gfx_coord_t);
extern ui_evclaim_t ui_scrollbar_pos_event(ui_scrollbar_t *, pos_event_t *);

#endif

/** @}
 */
