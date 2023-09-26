/*
 * Copyright (c) 2023 Jiri Svoboda
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * - Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 * - Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the distribution.
 * - The name of the author may not be used to endorse or promote products
 *   derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
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
#include <types/common.h>
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
	/** System menu handle is active */
	bool sysmenu_hdl_active;
	/** Minimize button */
	struct ui_pbutton *btn_min;
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
	/** System menu handle rectangle */
	gfx_rect_t sysmenu_hdl_rect;
	/** Window caption rectangle */
	gfx_rect_t caption_rect;
	/** Minimize button rectangle */
	gfx_rect_t btn_min_rect;
	/** Maximize button rectangle */
	gfx_rect_t btn_max_rect;
	/** Close button rectangle */
	gfx_rect_t btn_close_rect;
	/** Application area rectangle */
	gfx_rect_t app_area_rect;
} ui_wdecor_geom_t;

extern void ui_wdecor_sysmenu_open(ui_wdecor_t *, sysarg_t);
extern void ui_wdecor_sysmenu_left(ui_wdecor_t *, sysarg_t);
extern void ui_wdecor_sysmenu_right(ui_wdecor_t *, sysarg_t);
extern void ui_wdecor_sysmenu_accel(ui_wdecor_t *, char32_t, sysarg_t);
extern void ui_wdecor_minimize(ui_wdecor_t *);
extern void ui_wdecor_maximize(ui_wdecor_t *);
extern void ui_wdecor_unmaximize(ui_wdecor_t *);
extern void ui_wdecor_close(ui_wdecor_t *);
extern void ui_wdecor_move(ui_wdecor_t *, gfx_coord2_t *, sysarg_t);
extern void ui_wdecor_resize(ui_wdecor_t *, ui_wdecor_rsztype_t,
    gfx_coord2_t *, sysarg_t);
extern void ui_wdecor_set_cursor(ui_wdecor_t *, ui_stock_cursor_t);
extern void ui_wdecor_get_geom(ui_wdecor_t *, ui_wdecor_geom_t *);
extern errno_t ui_wdecor_sysmenu_hdl_paint_gfx(ui_wdecor_t *, gfx_rect_t *);
extern errno_t ui_wdecor_sysmenu_hdl_paint_text(ui_wdecor_t *, gfx_rect_t *);
extern errno_t ui_wdecor_sysmenu_hdl_paint(ui_wdecor_t *, gfx_rect_t *);
extern void ui_wdecor_frame_pos_event(ui_wdecor_t *, pos_event_t *);
extern ui_wdecor_rsztype_t ui_wdecor_get_rsztype(ui_wdecor_t *,
    gfx_coord2_t *);
extern ui_stock_cursor_t ui_wdecor_cursor_from_rsztype(ui_wdecor_rsztype_t);

#endif

/** @}
 */
