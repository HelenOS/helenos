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

/** @addtogroup nav
 * @{
 */
/** @file Navigator panel.
 *
 * Displays a file listing.
 */

#include <dirent.h>
#include <errno.h>
#include <gfx/render.h>
#include <gfx/text.h>
#include <stdlib.h>
#include <str.h>
#include <ui/control.h>
#include <ui/paint.h>
#include <ui/resource.h>
#include "panel.h"
#include "nav.h"

static void panel_ctl_destroy(void *);
static errno_t panel_ctl_paint(void *);
static ui_evclaim_t panel_ctl_kbd_event(void *, kbd_event_t *);
static ui_evclaim_t panel_ctl_pos_event(void *, pos_event_t *);

/** Panel control ops */
ui_control_ops_t panel_ctl_ops = {
	.destroy = panel_ctl_destroy,
	.paint = panel_ctl_paint,
	.kbd_event = panel_ctl_kbd_event,
	.pos_event = panel_ctl_pos_event
};

/** Create panel.
 *
 * @param window Containing window
 * @param rpanel Place to store pointer to new panel
 * @return EOK on success or an error code
 */
errno_t panel_create(ui_window_t *window, panel_t **rpanel)
{
	panel_t *panel;
	errno_t rc;

	panel = calloc(1, sizeof(panel_t));
	if (panel == NULL)
		return ENOMEM;

	rc = ui_control_new(&panel_ctl_ops, (void *)panel,
	    &panel->control);
	if (rc != EOK) {
		free(panel);
		return rc;
	}

	rc = gfx_color_new_ega(0x07, &panel->color);
	if (rc != EOK)
		goto error;

	rc = gfx_color_new_ega(0x30, &panel->curs_color);
	if (rc != EOK)
		goto error;

	panel->window = window;
	list_initialize(&panel->entries);
	panel->entries_cnt = 0;
	*rpanel = panel;
	return EOK;
error:
	if (panel->color != NULL)
		gfx_color_delete(panel->color);
	if (panel->curs_color != NULL)
		gfx_color_delete(panel->curs_color);
	ui_control_delete(panel->control);
	free(panel);
	return rc;
}

/** Destroy panel.
 *
 * @param panel Panel
 */
void panel_destroy(panel_t *panel)
{
	gfx_color_delete(panel->color);
	gfx_color_delete(panel->curs_color);
	panel_clear_entries(panel);
	ui_control_delete(panel->control);
	free(panel);
}

/** Paint panel entry.
 *
 * @param entry Panel entry
 * @param entry_idx Entry index (within list of entries)
 * @return EOK on success or an error code
 */
errno_t panel_entry_paint(panel_entry_t *entry, size_t entry_idx)
{
	panel_t *panel = entry->panel;
	gfx_context_t *gc = ui_window_get_gc(panel->window);
	ui_resource_t *res = ui_window_get_res(panel->window);
	gfx_font_t *font = ui_resource_get_font(res);
	gfx_text_fmt_t fmt;
	gfx_coord2_t pos;
	gfx_rect_t rect;
	size_t rows;
	errno_t rc;

	gfx_text_fmt_init(&fmt);
	rows = panel_page_size(panel);

	/* Do not display entry outside of current page */
	if (entry_idx < panel->page_idx ||
	    entry_idx >= panel->page_idx + rows)
		return EOK;

	pos.x = panel->rect.p0.x + 1;
	pos.y = panel->rect.p0.y + 1 + entry_idx - panel->page_idx;

	if (entry == panel->cursor)
		fmt.color = panel->curs_color;
	else
		fmt.color = panel->color;

	/* Draw entry background */
	rect.p0 = pos;
	rect.p1.x = panel->rect.p1.x - 1;
	rect.p1.y = rect.p0.y + 1;

	rc = gfx_set_color(gc, fmt.color);
	if (rc != EOK)
		return rc;

	rc = gfx_fill_rect(gc, &rect);
	if (rc != EOK)
		return rc;

	rc = gfx_puttext(font, &pos, &fmt, entry->name);
	if (rc != EOK)
		return rc;

	return EOK;
}

/** Paint panel.
 *
 * @param panel Panel
 */
errno_t panel_paint(panel_t *panel)
{
	gfx_context_t *gc = ui_window_get_gc(panel->window);
	ui_resource_t *res = ui_window_get_res(panel->window);
	gfx_text_fmt_t fmt;
	panel_entry_t *entry;
	int i, lines;
	errno_t rc;

	gfx_text_fmt_init(&fmt);

	rc = gfx_set_color(gc, panel->color);
	if (rc != EOK)
		return rc;

	rc = gfx_fill_rect(gc, &panel->rect);
	if (rc != EOK)
		return rc;

	rc = ui_paint_text_box(res, &panel->rect, ui_box_single,
	    panel->color);
	if (rc != EOK)
		return rc;

	lines = panel_page_size(panel);
	i = 0;

	entry = panel->page;
	while (entry != NULL && i < lines) {
		rc = panel_entry_paint(entry, panel->page_idx + i);
		if (rc != EOK)
			return rc;

		++i;
		entry = panel_next(entry);
	}

	rc = gfx_update(gc);
	if (rc != EOK)
		return rc;

	return EOK;
}

/** Handle panel keyboard event.
 *
 * @param panel Panel
 * @param event Keyboard event
 * @return ui_claimed iff event was claimed
 */
ui_evclaim_t panel_kbd_event(panel_t *panel, kbd_event_t *event)
{
	if (event->type == KEY_PRESS) {
		if ((event->mods & (KM_CTRL | KM_ALT | KM_SHIFT)) == 0) {
			switch (event->key) {
			case KC_UP:
				panel_cursor_up(panel);
				break;
			case KC_DOWN:
				panel_cursor_down(panel);
				break;
			case KC_HOME:
				panel_cursor_top(panel);
				break;
			case KC_END:
				panel_cursor_bottom(panel);
				break;
			case KC_PAGE_UP:
				panel_page_up(panel);
				break;
			case KC_PAGE_DOWN:
				panel_page_down(panel);
				break;
			default:
				break;
			}
		}
	}

	return ui_unclaimed;
}

/** Handle panel position event.
 *
 * @param panel Panel
 * @param event Position event
 * @return ui_claimed iff event was claimed
 */
ui_evclaim_t panel_pos_event(panel_t *panel, pos_event_t *event)
{
	return ui_unclaimed;
}

/** Get base control for panel.
 *
 * @param panel Panel
 * @return Base UI control
 */
ui_control_t *panel_ctl(panel_t *panel)
{
	return panel->control;
}

/** Set panel rectangle.
 *
 * @param panel Panel
 * @param rect Rectangle
 */
void panel_set_rect(panel_t *panel, gfx_rect_t *rect)
{
	panel->rect = *rect;
}

/** Get panel page size.
 *
 * @param panel Panel
 * @return Number of entries that fit in panel at the same time.
 */
unsigned panel_page_size(panel_t *panel)
{
	return panel->rect.p1.y - panel->rect.p0.y - 2;
}

/** Destroy panel control.
 *
 * @param arg Argument (panel_t *)
 */
void panel_ctl_destroy(void *arg)
{
	panel_t *panel = (panel_t *) arg;

	panel_destroy(panel);
}

/** Paint panel control.
 *
 * @param arg Argument (panel_t *)
 * @return EOK on success or an error code
 */
errno_t panel_ctl_paint(void *arg)
{
	panel_t *panel = (panel_t *) arg;

	return panel_paint(panel);
}

/** Handle panel control keyboard event.
 *
 * @param arg Argument (panel_t *)
 * @param kbd_event Keyboard event
 * @return @c ui_claimed iff the event is claimed
 */
ui_evclaim_t panel_ctl_kbd_event(void *arg, kbd_event_t *event)
{
	panel_t *panel = (panel_t *) arg;

	return panel_kbd_event(panel, event);
}

/** Handle panel control position event.
 *
 * @param arg Argument (panel_t *)
 * @param pos_event Position event
 * @return @c ui_claimed iff the event is claimed
 */
ui_evclaim_t panel_ctl_pos_event(void *arg, pos_event_t *event)
{
	panel_t *panel = (panel_t *) arg;

	return panel_pos_event(panel, event);
}

/** Append new panel entry.
 *
 * @param panel Panel
 * @param name File name
 * @param size File size;
 * @return EOK on success or an error code
 */
errno_t panel_entry_append(panel_t *panel, const char *name, uint64_t size)
{
	panel_entry_t *entry;

	entry = calloc(1, sizeof(panel_entry_t));
	if (entry == NULL)
		return ENOMEM;

	entry->panel = panel;
	entry->name = str_dup(name);
	if (entry->name == NULL) {
		free(entry);
		return ENOMEM;
	}

	entry->size = size;
	link_initialize(&entry->lentries);
	list_append(&entry->lentries, &panel->entries);
	++panel->entries_cnt;
	return EOK;
}

/** Delete panel entry.
 *
 * @param entry Panel entry
 */
void panel_entry_delete(panel_entry_t *entry)
{
	if (entry->panel->cursor == entry)
		entry->panel->cursor = NULL;
	if (entry->panel->page == entry)
		entry->panel->page = NULL;

	list_remove(&entry->lentries);
	--entry->panel->entries_cnt;
	free(entry->name);
	free(entry);
}

/** Clear panel entry list.
 *
 * @param panel Panel
 */
void panel_clear_entries(panel_t *panel)
{
	panel_entry_t *entry;

	entry = panel_first(panel);
	while (entry != NULL) {
		panel_entry_delete(entry);
		entry = panel_first(panel);
	}
}

/** Read directory into panel entry list.
 *
 * @param panel Panel
 * @param dirname Directory path
 * @return EOK on success or an error code
 */
errno_t panel_read_dir(panel_t *panel, const char *dirname)
{
	DIR *dir;
	struct dirent *dirent;
	errno_t rc;

	dir = opendir(dirname);
	if (dir == NULL)
		return errno;

	dirent = readdir(dir);
	while (dirent != NULL) {
		rc = panel_entry_append(panel, dirent->d_name, 1);
		if (rc != EOK)
			goto error;
		dirent = readdir(dir);
	}

	closedir(dir);

	panel->cursor = panel_first(panel);
	panel->cursor_idx = 0;
	panel->page = panel_first(panel);
	panel->page_idx = 0;
	return EOK;
error:
	closedir(dir);
	return rc;
}

/** Return first panel entry.
 *
 * @panel Panel
 * @return First panel entry or @c NULL if there are no entries
 */
panel_entry_t *panel_first(panel_t *panel)
{
	link_t *link;

	link = list_first(&panel->entries);
	if (link == NULL)
		return NULL;

	return list_get_instance(link, panel_entry_t, lentries);
}

/** Return last panel entry.
 *
 * @panel Panel
 * @return Last panel entry or @c NULL if there are no entries
 */
panel_entry_t *panel_last(panel_t *panel)
{
	link_t *link;

	link = list_last(&panel->entries);
	if (link == NULL)
		return NULL;

	return list_get_instance(link, panel_entry_t, lentries);
}

/** Return next panel entry.
 *
 * @param cur Current entry
 * @return Next entry or @c NULL if @a cur is the last entry
 */
panel_entry_t *panel_next(panel_entry_t *cur)
{
	link_t *link;

	link = list_next(&cur->lentries, &cur->panel->entries);
	if (link == NULL)
		return NULL;

	return list_get_instance(link, panel_entry_t, lentries);
}

/** Return previous panel entry.
 *
 * @param cur Current entry
 * @return Previous entry or @c NULL if @a cur is the first entry
 */
panel_entry_t *panel_prev(panel_entry_t *cur)
{
	link_t *link;

	link = list_prev(&cur->lentries, &cur->panel->entries);
	if (link == NULL)
		return NULL;

	return list_get_instance(link, panel_entry_t, lentries);
}

/** Move cursor to a new position, possibly scrolling.
 *
 * @param panel Panel
 * @param entry New entry under cursor
 * @param entry_idx Index of new entry under cursor
 */
void panel_cursor_move(panel_t *panel, panel_entry_t *entry, size_t entry_idx)
{
	gfx_context_t *gc = ui_window_get_gc(panel->window);
	panel_entry_t *old_cursor;
	size_t old_idx;
	size_t rows;
	panel_entry_t *e;
	size_t i;

	rows = panel_page_size(panel);

	old_cursor = panel->cursor;
	old_idx = panel->cursor_idx;

	panel->cursor = entry;
	panel->cursor_idx = entry_idx;

	if (entry_idx >= panel->page_idx &&
	    entry_idx < panel->page_idx + rows) {
		/*
		 * If cursor is still on the current page, we're not
		 * scrolling. Just unpaint old cursor and paint new
		 * cursor.
		 */
		panel_entry_paint(old_cursor, old_idx);
		panel_entry_paint(panel->cursor, panel->cursor_idx);

		(void) gfx_update(gc);
	} else {
		/*
		 * Need to scroll and update all rows.
		 */

		/* Scrolling up */
		if (entry_idx < panel->page_idx) {
			panel->page = entry;
			panel->page_idx = entry_idx;
		}

		/* Scrolling down */
		if (entry_idx >= panel->page_idx + rows) {
			if (entry_idx >= rows) {
				panel->page_idx = entry_idx - rows + 1;
				/* Find first page entry (go back rows - 1) */
				e = entry;
				for (i = 0; i < rows - 1; i++) {
					e = panel_prev(e);
				}

				/* Should be valid */
				assert(e != NULL);
				panel->page = e;
			} else {
				panel->page = panel_first(panel);
				panel->page_idx = 0;
			}
		}

		(void) panel_paint(panel);
	}
}

/** Move cursor one entry up.
 *
 * @param panel Panel
 */
void panel_cursor_up(panel_t *panel)
{
	panel_entry_t *prev;
	size_t prev_idx;

	prev = panel_prev(panel->cursor);
	prev_idx = panel->cursor_idx - 1;
	if (prev != NULL)
		panel_cursor_move(panel, prev, prev_idx);
}

/** Move cursor one entry down.
 *
 * @param panel Panel
 */
void panel_cursor_down(panel_t *panel)
{
	panel_entry_t *next;
	size_t next_idx;

	next = panel_next(panel->cursor);
	next_idx = panel->cursor_idx + 1;
	if (next != NULL)
		panel_cursor_move(panel, next, next_idx);
}

/** Move cursor to top.
 *
 * @param panel Panel
 */
void panel_cursor_top(panel_t *panel)
{
	panel_cursor_move(panel, panel_first(panel), 0);
}

/** Move cursor to bottom.
 *
 * @param panel Panel
 */
void panel_cursor_bottom(panel_t *panel)
{
	panel_cursor_move(panel, panel_last(panel), panel->entries_cnt - 1);
}

/** Move one page up.
 *
 * @param panel Panel
 */
void panel_page_up(panel_t *panel)
{
	gfx_context_t *gc = ui_window_get_gc(panel->window);
	panel_entry_t *old_page;
	panel_entry_t *old_cursor;
	size_t old_idx;
	size_t rows;
	panel_entry_t *entry;
	size_t i;

	rows = panel_page_size(panel);

	old_page = panel->page;
	old_cursor = panel->cursor;
	old_idx = panel->cursor_idx;

	/* Move page by rows entries up (if possible) */
	for (i = 0; i < rows; i++) {
		entry = panel_prev(panel->page);
		if (entry != NULL) {
			panel->page = entry;
			--panel->page_idx;
		}
	}

	/* Move cursor by rows entries up (if possible) */

	for (i = 0; i < rows; i++) {
		entry = panel_prev(panel->cursor);
		if (entry != NULL) {
			panel->cursor = entry;
			--panel->cursor_idx;
		}
	}

	if (panel->page != old_page) {
		/* We have scrolled. Need to repaint all entries */
		(void) panel_paint(panel);
	} else if (panel->cursor != old_cursor) {
		/* No scrolling, but cursor has moved */
		panel_entry_paint(old_cursor, old_idx);
		panel_entry_paint(panel->cursor, panel->cursor_idx);

		(void) gfx_update(gc);
	}
}

/** Move one page down.
 *
 * @param panel Panel
 */
void panel_page_down(panel_t *panel)
{
	gfx_context_t *gc = ui_window_get_gc(panel->window);
	panel_entry_t *old_page;
	panel_entry_t *old_cursor;
	size_t old_idx;
	size_t max_idx;
	size_t rows;
	panel_entry_t *entry;
	size_t i;

	rows = panel_page_size(panel);

	old_page = panel->page;
	old_cursor = panel->cursor;
	old_idx = panel->cursor_idx;
	max_idx = panel->entries_cnt - rows;

	/* Move page by rows entries down (if possible) */
	for (i = 0; i < rows; i++) {
		entry = panel_next(panel->page);
		/* Do not scroll that results in a short page */
		if (entry != NULL && panel->page_idx < max_idx) {
			panel->page = entry;
			++panel->page_idx;
		}
	}

	/* Move cursor by rows entries down (if possible) */

	for (i = 0; i < rows; i++) {
		entry = panel_next(panel->cursor);
		if (entry != NULL) {
			panel->cursor = entry;
			++panel->cursor_idx;
		}
	}

	if (panel->page != old_page) {
		/* We have scrolled. Need to repaint all entries */
		(void) panel_paint(panel);
	} else if (panel->cursor != old_cursor) {
		/* No scrolling, but cursor has moved */
		panel_entry_paint(old_cursor, old_idx);
		panel_entry_paint(panel->cursor, panel->cursor_idx);

		(void) gfx_update(gc);
	}
}

/** @}
 */
