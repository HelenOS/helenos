/*
 * SPDX-FileCopyrightText: 2021 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libui
 * @{
 */
/**
 * @file Menu entry structure
 *
 */

#ifndef _UI_PRIVATE_MENUENTRY_H
#define _UI_PRIVATE_MENUENTRY_H

#include <adt/list.h>
#include <gfx/coord.h>
#include <types/ui/menuentry.h>

/** Actual structure of menu entry.
 *
 * This is private to libui.
 */
struct ui_menu_entry {
	/** Containing menu */
	struct ui_menu *menu;
	/** Link to @c menu->entries */
	link_t lentries;
	/** Callbacks */
	ui_menu_entry_cb_t cb;
	/** This entry is a separator entry */
	bool separator;
	/** Menu entry is currently held down */
	bool held;
	/** Pointer is currently inside */
	bool inside;
	/** Callback argument */
	void *arg;
	/** Caption */
	char *caption;
	/** Shortcut key(s) */
	char *shortcut;
};

/** Menu entry geometry.
 *
 * Computed positions of menu entry elements.
 */
typedef struct {
	/** Outer rectangle */
	gfx_rect_t outer_rect;
	/** Caption position */
	gfx_coord2_t caption_pos;
	/** Shortcut position */
	gfx_coord2_t shortcut_pos;
} ui_menu_entry_geom_t;

extern void ui_menu_entry_get_geom(ui_menu_entry_t *, gfx_coord2_t *,
    ui_menu_entry_geom_t *);
extern void ui_menu_entry_cb(ui_menu_entry_t *);

#endif

/** @}
 */
