/*
 * SPDX-FileCopyrightText: 2022 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libui
 * @{
 */
/**
 * @file Menu bar
 */

#ifndef _UI_MENUBAR_H
#define _UI_MENUBAR_H

#include <errno.h>
#include <gfx/coord.h>
#include <io/pos_event.h>
#include <types/ui/menubar.h>
#include <types/ui/control.h>
#include <types/ui/event.h>
#include <types/ui/resource.h>
#include <types/ui/ui.h>
#include <types/ui/window.h>
#include <uchar.h>

extern errno_t ui_menu_bar_create(ui_t *, ui_window_t *,
    ui_menu_bar_t **);
extern void ui_menu_bar_destroy(ui_menu_bar_t *);
extern ui_control_t *ui_menu_bar_ctl(ui_menu_bar_t *);
extern void ui_menu_bar_set_rect(ui_menu_bar_t *, gfx_rect_t *);
extern errno_t ui_menu_bar_paint(ui_menu_bar_t *);
extern ui_evclaim_t ui_menu_bar_kbd_event(ui_menu_bar_t *, kbd_event_t *);
extern ui_evclaim_t ui_menu_bar_pos_event(ui_menu_bar_t *, pos_event_t *);
extern void ui_menu_bar_press_accel(ui_menu_bar_t *, char32_t);
extern void ui_menu_bar_unfocus(ui_menu_bar_t *);
extern void ui_menu_bar_activate(ui_menu_bar_t *);
extern void ui_menu_bar_deactivate(ui_menu_bar_t *);

#endif

/** @}
 */
