/*
 * SPDX-FileCopyrightText: 2022 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libui
 * @{
 */
/**
 * @file Menu bar structure
 *
 */

#ifndef _UI_PRIVATE_MENUBAR_H
#define _UI_PRIVATE_MENUBAR_H

#include <adt/list.h>
#include <gfx/coord.h>
#include <stdbool.h>
#include <types/ui/menu.h>
#include <types/ui/menubar.h>

/** Actual structure of menu bar.
 *
 * This is private to libui.
 */
struct ui_menu_bar {
	/** Base control object */
	struct ui_control *control;
	/** UI */
	struct ui *ui;
	/** UI window containing menu bar */
	struct ui_window *window;
	/** Menu bar rectangle */
	gfx_rect_t rect;
	/** Menu bar is active */
	bool active;
	/** Selected menu or @c NULL */
	struct ui_menu *selected;
	/** List of menus (ui_menu_t) */
	list_t menus;
};

extern void ui_menu_bar_select(ui_menu_bar_t *, ui_menu_t *, bool);
extern void ui_menu_bar_left(ui_menu_bar_t *);
extern void ui_menu_bar_right(ui_menu_bar_t *);
extern ui_evclaim_t ui_menu_bar_key_press_unmod(ui_menu_bar_t *, kbd_event_t *);
extern void ui_menu_bar_entry_rect(ui_menu_bar_t *, ui_menu_t *, gfx_rect_t *);

#endif

/** @}
 */
