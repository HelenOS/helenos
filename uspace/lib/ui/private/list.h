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
 * @file UI list
 */

#ifndef _UI_PRIVATE_LIST_H
#define _UI_PRIVATE_LIST_H

#include <adt/list.h>
#include <gfx/color.h>
#include <gfx/coord.h>
#include <ipc/loc.h>
#include <ui/window.h>
#include <stdint.h>
#include <types/ui/list.h>

/** List entry */
struct ui_list_entry {
	/** Containing list */
	struct ui_list *list;
	/** Link to @c flist->entries */
	link_t lentries;
	/** Entry caption */
	char *caption;
	/** User argument */
	void *arg;
	/** Custom color or @c NULL to use default color */
	gfx_color_t *color;
	/** Custom background color or @c NULL to use default color */
	gfx_color_t *bgcolor;
};

/** UI list.
 *
 * Displays a scrollable list of entries.
 */
typedef struct ui_list {
	/** Base control object */
	struct ui_control *control;

	/** Containing window */
	ui_window_t *window;

	/** Callbacks */
	struct ui_list_cb *cb;

	/** Callback argument */
	void *cb_arg;

	/** List rectangle */
	gfx_rect_t rect;

	/** Scrollbar */
	struct ui_scrollbar *scrollbar;

	/** List entries (list of ui_list_entry_t) */
	list_t entries;

	/** Number of entries */
	size_t entries_cnt;

	/** First entry of current page */
	ui_list_entry_t *page;

	/** Index of first entry of current page */
	size_t page_idx;

	/** Cursor position */
	ui_list_entry_t *cursor;

	/** Index of entry under cursor */
	size_t cursor_idx;

	/** @c true iff the list is active */
	bool active;
} ui_list_t;

extern gfx_coord_t ui_list_entry_height(ui_list_t *);
extern errno_t ui_list_entry_paint(ui_list_entry_t *, size_t);
extern errno_t ui_list_paint(ui_list_t *);
extern ui_evclaim_t ui_list_kbd_event(ui_list_t *, kbd_event_t *);
extern ui_evclaim_t ui_list_pos_event(ui_list_t *, pos_event_t *);
extern unsigned ui_list_page_size(ui_list_t *);
extern void ui_list_inside_rect(ui_list_t *, gfx_rect_t *);
extern void ui_list_scrollbar_rect(ui_list_t *, gfx_rect_t *);
extern gfx_coord_t ui_list_scrollbar_pos(ui_list_t *);
extern void ui_list_scrollbar_update(ui_list_t *);
extern void ui_list_clear_entries(ui_list_t *);
extern ui_list_entry_t *ui_list_page_nth_entry(ui_list_t *,
    size_t, size_t *);
extern void ui_list_cursor_move(ui_list_t *, ui_list_entry_t *,
    size_t);
extern void ui_list_cursor_up(ui_list_t *);
extern void ui_list_cursor_down(ui_list_t *);
extern void ui_list_cursor_top(ui_list_t *);
extern void ui_list_cursor_bottom(ui_list_t *);
extern void ui_list_page_up(ui_list_t *);
extern void ui_list_page_down(ui_list_t *);
extern void ui_list_scroll_up(ui_list_t *);
extern void ui_list_scroll_down(ui_list_t *);
extern void ui_list_scroll_page_up(ui_list_t *);
extern void ui_list_scroll_page_down(ui_list_t *);
extern void ui_list_scroll_pos(ui_list_t *, size_t);
extern void ui_list_activate_req(ui_list_t *);
extern void ui_list_selected(ui_list_entry_t *);
extern int ui_list_entry_ptr_cmp(const void *, const void *);
extern size_t ui_list_entry_get_idx(ui_list_entry_t *);
extern void ui_list_entry_destroy(ui_list_entry_t *);

#endif

/** @}
 */
