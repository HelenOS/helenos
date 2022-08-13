/*
 * SPDX-FileCopyrightText: 2021 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libui
 * @{
 */
/**
 * @file Text entry structure
 *
 */

#ifndef _UI_PRIVATE_ENTRY_H
#define _UI_PRIVATE_ENTRY_H

#include <gfx/coord.h>
#include <gfx/text.h>
#include <stdbool.h>

/** Actual structure of text entry.
 *
 * This is private to libui.
 */
struct ui_entry {
	/** Base control object */
	struct ui_control *control;
	/** UI window */
	struct ui_window *window;
	/** Entry rectangle */
	gfx_rect_t rect;
	/** Horizontal alignment */
	gfx_halign_t halign;
	/** Text entry is read-only */
	bool read_only;
	/** Text */
	char *text;
	/** Current scroll position (in pixels) */
	gfx_coord_t scroll_pos;
	/** Cursor position in the text (offset in bytes) */
	unsigned pos;
	/** Selection start position in text (offset in bytes) */
	unsigned sel_start;
	/** Pointer is currently inside */
	bool pointer_inside;
	/** Entry is activated */
	bool active;
	/** Button is held down */
	bool held;
	/** Left shift is held down */
	bool lshift_held;
	/** Right shift is held down */
	bool rshift_held;
};

/** Text entry geometry.
 *
 * Computed geometry of text entry elements.
 */
typedef struct {
	/** Interior rectangle */
	gfx_rect_t interior_rect;
	/** Text rectangle */
	gfx_rect_t text_rect;
	/** Text anchor position */
	gfx_coord2_t text_pos;
	/** Text anchor X coordinate */
	gfx_coord_t anchor_x;
} ui_entry_geom_t;

extern errno_t ui_entry_insert_str(ui_entry_t *, const char *);
extern ui_evclaim_t ui_entry_key_press_ctrl(ui_entry_t *, kbd_event_t *);
extern ui_evclaim_t ui_entry_key_press_shift(ui_entry_t *, kbd_event_t *);
extern ui_evclaim_t ui_entry_key_press_unmod(ui_entry_t *, kbd_event_t *);
extern void ui_entry_get_geom(ui_entry_t *, ui_entry_geom_t *);
extern size_t ui_entry_find_pos(ui_entry_t *, gfx_coord2_t *);
extern void ui_entry_delete_sel(ui_entry_t *);
extern void ui_entry_scroll_update(ui_entry_t *, bool);

#endif

/** @}
 */
