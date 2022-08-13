/*
 * SPDX-FileCopyrightText: 2022 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libui
 * @{
 */
/**
 * @file Menu entry
 */

#ifndef _UI_MENUENTRY_H
#define _UI_MENUENTRY_H

#include <errno.h>
#include <gfx/coord.h>
#include <io/pos_event.h>
#include <stdbool.h>
#include <types/ui/menu.h>
#include <types/ui/menuentry.h>
#include <types/ui/event.h>

extern errno_t ui_menu_entry_create(ui_menu_t *, const char *, const char *,
    ui_menu_entry_t **);
extern errno_t ui_menu_entry_sep_create(ui_menu_t *, ui_menu_entry_t **);
extern void ui_menu_entry_destroy(ui_menu_entry_t *);
extern void ui_menu_entry_set_cb(ui_menu_entry_t *, ui_menu_entry_cb_t,
    void *);
extern ui_menu_entry_t *ui_menu_entry_first(ui_menu_t *);
extern ui_menu_entry_t *ui_menu_entry_last(ui_menu_t *);
extern ui_menu_entry_t *ui_menu_entry_next(ui_menu_entry_t *);
extern ui_menu_entry_t *ui_menu_entry_prev(ui_menu_entry_t *);
extern gfx_coord_t ui_menu_entry_calc_width(ui_menu_t *,
    gfx_coord_t, gfx_coord_t);
extern void ui_menu_entry_column_widths(ui_menu_entry_t *,
    gfx_coord_t *, gfx_coord_t *);
extern gfx_coord_t ui_menu_entry_height(ui_menu_entry_t *);
extern char32_t ui_menu_entry_get_accel(ui_menu_entry_t *);
extern errno_t ui_menu_entry_paint(ui_menu_entry_t *, gfx_coord2_t *);
extern bool ui_menu_entry_selectable(ui_menu_entry_t *);
extern void ui_menu_entry_press(ui_menu_entry_t *, gfx_coord2_t *);
extern void ui_menu_entry_release(ui_menu_entry_t *);
extern void ui_menu_entry_enter(ui_menu_entry_t *, gfx_coord2_t *);
extern void ui_menu_entry_leave(ui_menu_entry_t *, gfx_coord2_t *);
extern void ui_menu_entry_activate(ui_menu_entry_t *);
extern ui_evclaim_t ui_menu_entry_pos_event(ui_menu_entry_t *, gfx_coord2_t *,
    pos_event_t *);

#endif

/** @}
 */
