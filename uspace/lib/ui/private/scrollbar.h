/*
 * Copyright (c) 2022 Jiri Svoboda
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
