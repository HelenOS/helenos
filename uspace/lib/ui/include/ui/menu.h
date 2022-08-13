/*
 * SPDX-FileCopyrightText: 2022 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libui
 * @{
 */
/**
 * @file Menu
 */

#ifndef _UI_MENU_H
#define _UI_MENU_H

#include <errno.h>
#include <gfx/coord.h>
#include <io/kbd_event.h>
#include <io/pos_event.h>
#include <stdbool.h>
#include <types/ui/menu.h>
#include <types/ui/menubar.h>
#include <types/ui/event.h>
#include <uchar.h>

extern errno_t ui_menu_create(ui_menu_bar_t *, const char *, ui_menu_t **);
extern void ui_menu_destroy(ui_menu_t *);
extern ui_menu_t *ui_menu_first(ui_menu_bar_t *);
extern ui_menu_t *ui_menu_next(ui_menu_t *);
extern ui_menu_t *ui_menu_last(ui_menu_bar_t *);
extern ui_menu_t *ui_menu_prev(ui_menu_t *);
extern const char *ui_menu_caption(ui_menu_t *);
extern void ui_menu_get_rect(ui_menu_t *, gfx_coord2_t *, gfx_rect_t *);
extern char32_t ui_menu_get_accel(ui_menu_t *);
extern errno_t ui_menu_open(ui_menu_t *, gfx_rect_t *);
extern void ui_menu_close(ui_menu_t *);
extern bool ui_menu_is_open(ui_menu_t *);
extern errno_t ui_menu_paint(ui_menu_t *, gfx_coord2_t *);
extern ui_evclaim_t ui_menu_kbd_event(ui_menu_t *, kbd_event_t *);
extern ui_evclaim_t ui_menu_pos_event(ui_menu_t *, gfx_coord2_t *,
    pos_event_t *);

#endif

/** @}
 */
