/*
 * SPDX-FileCopyrightText: 2022 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libui
 * @{
 */
/**
 * @file Window decoration structure
 *
 */

#ifndef _UI_PRIVATE_WDECOR_H
#define _UI_PRIVATE_WDECOR_H

#include <gfx/coord.h>
#include <io/pos_event.h>
#include <stdbool.h>
#include <types/ui/cursor.h>
#include <types/ui/wdecor.h>

/** Actual structure of push button.
 *
 * This is private to libui.
 */
struct ui_wdecor {
	/** UI resource */
	struct ui_resource *res;
	/** Callbacks */
	struct ui_wdecor_cb *cb;
	/** Callback argument */
	void *arg;
	/** Window decoration rectangle */
	gfx_rect_t rect;
	/** Style */
	ui_wdecor_style_t style;
	/** Caption */
	char *caption;
	/** Window is active */
	bool active;
	/** Window is maximized */
	bool maximized;
	/** Maximize button */
	struct ui_pbutton *btn_max;
	/** Close button */
	struct ui_pbutton *btn_close;
};

/** Window decoration geometry.
 *
 * Computed rectangles of window decoration elements.
 */
typedef struct {
	/** Interior rectangle */
	gfx_rect_t interior_rect;
	/** Title bar rectangle */
	gfx_rect_t title_bar_rect;
	/** Maximize button rectangle */
	gfx_rect_t btn_max_rect;
	/** Close button rectangle */
	gfx_rect_t btn_close_rect;
	/** Application area rectangle */
	gfx_rect_t app_area_rect;
} ui_wdecor_geom_t;

extern void ui_wdecor_maximize(ui_wdecor_t *);
extern void ui_wdecor_unmaximize(ui_wdecor_t *);
extern void ui_wdecor_close(ui_wdecor_t *);
extern void ui_wdecor_move(ui_wdecor_t *, gfx_coord2_t *);
extern void ui_wdecor_resize(ui_wdecor_t *, ui_wdecor_rsztype_t,
    gfx_coord2_t *);
extern void ui_wdecor_set_cursor(ui_wdecor_t *, ui_stock_cursor_t);
extern void ui_wdecor_get_geom(ui_wdecor_t *, ui_wdecor_geom_t *);
extern void ui_wdecor_frame_pos_event(ui_wdecor_t *, pos_event_t *);
extern ui_wdecor_rsztype_t ui_wdecor_get_rsztype(ui_wdecor_t *,
    gfx_coord2_t *);
extern ui_stock_cursor_t ui_wdecor_cursor_from_rsztype(ui_wdecor_rsztype_t);

#endif

/** @}
 */
