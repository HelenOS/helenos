/*
 * SPDX-FileCopyrightText: 2022 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libui
 * @{
 */
/**
 * @file Menu structure
 *
 */

#ifndef _UI_PRIVATE_MENU_H
#define _UI_PRIVATE_MENU_H

#include <adt/list.h>
#include <gfx/coord.h>
#include <stdbool.h>
#include <types/ui/menu.h>
#include <types/ui/resource.h>

/** Actual structure of menu.
 *
 * This is private to libui.
 */
struct ui_menu {
	/** Containing menu bar */
	struct ui_menu_bar *mbar;
	/** Link to @c bar->menus */
	link_t lmenus;
	/** Caption */
	char *caption;
	/** Popup window or @c NULL if menu is not currently open */
	struct ui_popup *popup;
	/** Selected menu entry or @c NULL */
	struct ui_menu_entry *selected;
	/** Maximum caption width */
	gfx_coord_t max_caption_w;
	/** Maximum shortcut width */
	gfx_coord_t max_shortcut_w;
	/** Total entry height */
	gfx_coord_t total_h;
	/** Menu entries (ui_menu_entry_t) */
	list_t entries;
};

/** Menu geometry.
 *
 * Computed rectangles of menu elements.
 */
typedef struct {
	/** Outer rectangle */
	gfx_rect_t outer_rect;
	/** Entries rectangle */
	gfx_rect_t entries_rect;
} ui_menu_geom_t;

extern void ui_menu_get_geom(ui_menu_t *, gfx_coord2_t *, ui_menu_geom_t *);
extern ui_resource_t *ui_menu_get_res(ui_menu_t *);
extern errno_t ui_menu_paint_bg_gfx(ui_menu_t *, gfx_coord2_t *);
extern errno_t ui_menu_paint_bg_text(ui_menu_t *, gfx_coord2_t *);
extern void ui_menu_up(ui_menu_t *);
extern void ui_menu_down(ui_menu_t *);

#endif

/** @}
 */
