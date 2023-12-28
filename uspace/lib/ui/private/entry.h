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
