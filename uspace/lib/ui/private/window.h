/*
 * Copyright (c) 2021 Jiri Svoboda
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
 * @file Window structure
 *
 */

#ifndef _UI_PRIVATE_WINDOW_H
#define _UI_PRIVATE_WINDOW_H

#include <errno.h>
#include <congfx/console.h>
#include <display.h>
#include <gfx/context.h>
#include <io/kbd_event.h>
#include <io/pos_event.h>
#include <memgfx/memgc.h>
#include <types/ui/cursor.h>
#include <types/ui/window.h>

/** Actual structure of window.
 *
 * This is private to libui.
 */
struct ui_window {
	/** Containing user interface */
	struct ui *ui;
	/** Callbacks */
	struct ui_window_cb *cb;
	/** Callback argument */
	void *arg;
	/** Display window */
	display_window_t *dwindow;
	/** Console GC */
	console_gc_t *cgc;
	/** Window GC */
	gfx_context_t *gc;
	/** Window bitmap (if client-side rendering) */
	gfx_bitmap_t *bmp;
	/** Window memory GC (if client-side rendering) */
	mem_gc_t *mgc;
	/** Real window GC (if client-side rendering) */
	gfx_context_t *realgc;
	/** Window rectangle */
	gfx_rect_t rect;
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
};

extern void ui_window_send_close(ui_window_t *);
extern void ui_window_send_focus(ui_window_t *);
extern void ui_window_send_kbd(ui_window_t *, kbd_event_t *);
extern errno_t ui_window_send_paint(ui_window_t *);
extern void ui_window_send_pos(ui_window_t *, pos_event_t *);
extern void ui_window_send_unfocus(ui_window_t *);

#endif

/** @}
 */
