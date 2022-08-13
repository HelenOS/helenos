/*
 * SPDX-FileCopyrightText: 2022 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libui
 * @{
 */
/**
 * @file Scrollbar structure
 *
 */

#ifndef _UI_PRIVATE_SCROLLBAR_H
#define _UI_PRIVATE_SCROLLBAR_H

#include <gfx/coord.h>
#include <stdbool.h>
#include <types/ui/scrollbar.h>

/** Actual structure of scrollbar.
 *
 * This is private to libui.
 */
struct ui_scrollbar {
	/** Base control object */
	struct ui_control *control;
	/** UI */
	struct ui *ui;
	/** UI window containing scrollbar */
	struct ui_window *window;
	/** Callbacks */
	struct ui_scrollbar_cb *cb;
	/** Callback argument */
	void *arg;
	/** Scrollbar rectangle */
	gfx_rect_t rect;
	/** Scrollbar direction */
	ui_scrollbar_dir_t dir;
	/** Thumb length */
	gfx_coord_t thumb_len;
	/** Up button */
	struct ui_pbutton *up_btn;
	/** Down button */
	struct ui_pbutton *down_btn;
	/** Thumb is currently held down */
	bool thumb_held;
	/** Up through is currently held down */
	bool up_through_held;
	/** Pointer is inside up through */
	bool up_through_inside;
	/** Down through is currently held down */
	bool down_through_held;
	/** Pointer is inside down through */
	bool down_through_inside;
	/** Position where thumb was pressed */
	gfx_coord2_t press_pos;
	/** Last thumb position */
	gfx_coord_t last_pos;
	/** Thumb position */
	gfx_coord_t pos;
	/** Last cursor position (when through is held) */
	gfx_coord2_t last_curs_pos;
};

/** Scrollbar geometry.
 *
 * Computed rectangles of scrollbar elements.
 */
typedef struct {
	/** Up button rectangle */
	gfx_rect_t up_btn_rect;
	/** Through rectangle */
	gfx_rect_t through_rect;
	/** Up through rectangle */
	gfx_rect_t up_through_rect;
	/** Thumb rectangle */
	gfx_rect_t thumb_rect;
	/** Down through rectangle */
	gfx_rect_t down_through_rect;
	/** Down button rectangle */
	gfx_rect_t down_btn_rect;
} ui_scrollbar_geom_t;

extern errno_t ui_scrollbar_paint_gfx(ui_scrollbar_t *);
extern errno_t ui_scrollbar_paint_text(ui_scrollbar_t *);
extern void ui_scrollbar_get_geom(ui_scrollbar_t *, ui_scrollbar_geom_t *);

#endif

/** @}
 */
