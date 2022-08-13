/*
 * SPDX-FileCopyrightText: 2022 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libui
 * @{
 */
/**
 * @file File list
 */

#ifndef _UI_PRIVATE_FILELIST_H
#define _UI_PRIVATE_FILELIST_H

#include <adt/list.h>
#include <gfx/color.h>
#include <gfx/coord.h>
#include <ipc/loc.h>
#include <ui/window.h>
#include <stdint.h>
#include <types/ui/filelist.h>

/** File list entry attributes */
struct ui_file_list_entry_attr {
	/** File name */
	const char *name;
	/** File size */
	uint64_t size;
	/** @c true iff entry is a directory */
	bool isdir;
	/** Service number for service special entries */
	service_id_t svc;
};

/** File list entry */
struct ui_file_list_entry {
	/** Containing file list */
	struct ui_file_list *flist;
	/** Link to @c flist->entries */
	link_t lentries;
	/** File name */
	char *name;
	/** File size */
	uint64_t size;
	/** @c true iff entry is a directory */
	bool isdir;
	/** Service number for service special entries */
	service_id_t svc;
};

/** File list.
 *
 * Allows browsing files and directories.
 */
typedef struct ui_file_list {
	/** Base control object */
	struct ui_control *control;

	/** Containing window */
	ui_window_t *window;

	/** Callbacks */
	struct ui_file_list_cb *cb;

	/** Callback argument */
	void *cb_arg;

	/** File list rectangle */
	gfx_rect_t rect;

	/** Scrollbar */
	struct ui_scrollbar *scrollbar;

	/** Directory-type entry color */
	gfx_color_t *dir_color;

	/** Service-type entry color */
	gfx_color_t *svc_color;

	/** File list entries (list of ui_file_list_entry_t) */
	list_t entries;

	/** Number of entries */
	size_t entries_cnt;

	/** First entry of current page */
	ui_file_list_entry_t *page;

	/** Index of first entry of current page */
	size_t page_idx;

	/** Cursor position */
	ui_file_list_entry_t *cursor;

	/** Index of entry under cursor */
	size_t cursor_idx;

	/** @c true iff the file list is active */
	bool active;

	/** Directory */
	char *dir;
} ui_file_list_t;

extern gfx_coord_t ui_file_list_entry_height(ui_file_list_t *);
extern errno_t ui_file_list_entry_paint(ui_file_list_entry_t *, size_t);
extern errno_t ui_file_list_paint(ui_file_list_t *);
extern ui_evclaim_t ui_file_list_kbd_event(ui_file_list_t *, kbd_event_t *);
extern ui_evclaim_t ui_file_list_pos_event(ui_file_list_t *, pos_event_t *);
extern unsigned ui_file_list_page_size(ui_file_list_t *);
extern void ui_file_list_inside_rect(ui_file_list_t *, gfx_rect_t *);
extern void ui_file_list_scrollbar_rect(ui_file_list_t *, gfx_rect_t *);
extern gfx_coord_t ui_file_list_scrollbar_pos(ui_file_list_t *);
extern void ui_file_list_scrollbar_update(ui_file_list_t *);
extern bool ui_file_list_is_active(ui_file_list_t *);
extern void ui_file_list_entry_delete(ui_file_list_entry_t *);
extern void ui_file_list_clear_entries(ui_file_list_t *);
extern errno_t ui_file_list_sort(ui_file_list_t *);
extern int ui_file_list_entry_ptr_cmp(const void *, const void *);
extern ui_file_list_entry_t *ui_file_list_first(ui_file_list_t *);
extern ui_file_list_entry_t *ui_file_list_last(ui_file_list_t *);
extern ui_file_list_entry_t *ui_file_list_next(ui_file_list_entry_t *);
extern ui_file_list_entry_t *ui_file_list_prev(ui_file_list_entry_t *);
extern ui_file_list_entry_t *ui_file_list_page_nth_entry(ui_file_list_t *,
    size_t, size_t *);
extern void ui_file_list_entry_attr_init(ui_file_list_entry_attr_t *);
extern errno_t ui_file_list_entry_append(ui_file_list_t *,
    ui_file_list_entry_attr_t *);
extern void ui_file_list_cursor_move(ui_file_list_t *, ui_file_list_entry_t *,
    size_t);
extern void ui_file_list_cursor_up(ui_file_list_t *);
extern void ui_file_list_cursor_down(ui_file_list_t *);
extern void ui_file_list_cursor_top(ui_file_list_t *);
extern void ui_file_list_cursor_bottom(ui_file_list_t *);
extern void ui_file_list_page_up(ui_file_list_t *);
extern void ui_file_list_page_down(ui_file_list_t *);
extern void ui_file_list_scroll_up(ui_file_list_t *);
extern void ui_file_list_scroll_down(ui_file_list_t *);
extern void ui_file_list_scroll_page_up(ui_file_list_t *);
extern void ui_file_list_scroll_page_down(ui_file_list_t *);
extern void ui_file_list_scroll_pos(ui_file_list_t *, size_t);
extern errno_t ui_file_list_open_dir(ui_file_list_t *, ui_file_list_entry_t *);
extern errno_t ui_file_list_open_file(ui_file_list_t *, ui_file_list_entry_t *);
extern void ui_file_list_activate_req(ui_file_list_t *);
extern void ui_file_list_selected(ui_file_list_t *, const char *);

#endif

/** @}
 */
