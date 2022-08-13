/*
 * SPDX-FileCopyrightText: 2022 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libui
 * @{
 */
/**
 * @file Window structure
 *
 */

#ifndef _UI_PRIVATE_WINDOW_H
#define _UI_PRIVATE_WINDOW_H

#include <adt/list.h>
#include <errno.h>
#include <congfx/console.h>
#include <display.h>
#include <gfx/context.h>
#include <io/kbd_event.h>
#include <io/pos_event.h>
#include <memgfx/memgc.h>
#include <memgfx/xlategc.h>
#include <types/ui/cursor.h>
#include <types/ui/window.h>

/** Actual structure of window.
 *
 * This is private to libui.
 */
struct ui_window {
	/** Containing user interface */
	struct ui *ui;
	/** Link to @c ui->windows */
	link_t lwindows;
	/** Callbacks */
	struct ui_window_cb *cb;
	/** Callback argument */
	void *arg;
	/** Display window */
	display_window_t *dwindow;
	/** Window GC */
	gfx_context_t *gc;
	/** Window bitmap (if client-side rendering) */
	gfx_bitmap_t *bmp;
	/** Window memory GC (if client-side rendering) */
	mem_gc_t *mgc;
	/** Translating GC (if full screen & server-side rendering) */
	xlate_gc_t *xgc;
	/** Real window GC (if client-side rendering) */
	gfx_context_t *realgc;
	/** Window rectangle */
	gfx_rect_t rect;
	/** Normal window rectangle (when not maximized) */
	gfx_rect_t normal_rect;
	/** Display position (if fullscreen mode) */
	gfx_coord2_t dpos;
	/** Application area bitmap */
	gfx_bitmap_t *app_bmp;
	/** Application area memory GC */
	mem_gc_t *app_mgc;
	/** Application area GC */
	gfx_context_t *app_gc;
	/** Dirty rectangle */
	gfx_rect_t dirty_rect;
	/** UI resource. Ideally this would be in ui_t. */
	struct ui_resource *res;
	/** Window decoration */
	struct ui_wdecor *wdecor;
	/** Top-level control in the application area */
	struct ui_control *control;
	/** Current cursor */
	ui_stock_cursor_t cursor;
	/** Window placement */
	ui_wnd_placement_t placement;
};

/** Size change operation */
typedef enum {
	/** Resize window */
	ui_wsc_resize,
	/** Maximize window */
	ui_wsc_maximize,
	/** Unmaximize window */
	ui_wsc_unmaximize
} ui_wnd_sc_op_t;

extern display_stock_cursor_t wnd_dcursor_from_cursor(ui_stock_cursor_t);
extern void ui_window_send_maximize(ui_window_t *);
extern void ui_window_send_unmaximize(ui_window_t *);
extern void ui_window_send_close(ui_window_t *);
extern void ui_window_send_focus(ui_window_t *);
extern void ui_window_send_kbd(ui_window_t *, kbd_event_t *);
extern errno_t ui_window_send_paint(ui_window_t *);
extern void ui_window_send_pos(ui_window_t *, pos_event_t *);
extern void ui_window_send_unfocus(ui_window_t *);
extern errno_t ui_window_size_change(ui_window_t *, gfx_rect_t *,
    ui_wnd_sc_op_t);

#endif

/** @}
 */
