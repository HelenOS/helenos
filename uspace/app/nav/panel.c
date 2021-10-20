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
#include <task.h>
#include <ui/control.h>
#include <ui/paint.h>
#include <ui/resource.h>
#include <vfs/vfs.h>
#include <qsort.h>
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
 * @param active @c true iff panel should be active
 * @param rpanel Place to store pointer to new panel
 * @return EOK on success or an error code
 */
errno_t panel_create(ui_window_t *window, bool active, panel_t **rpanel)
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

	rc = gfx_color_new_ega(0x0f, &panel->act_border_color);
	if (rc != EOK)
		goto error;

	rc = gfx_color_new_ega(0x0f, &panel->dir_color);
	if (rc != EOK)
		goto error;

	rc = gfx_color_new_ega(0x0a, &panel->svc_color);
	if (rc != EOK)
		goto error;

	panel->window = window;
	list_initialize(&panel->entries);
	panel->entries_cnt = 0;
	panel->active = active;
	*rpanel = panel;
	return EOK;
error:
	if (panel->color != NULL)
		gfx_color_delete(panel->color);
	if (panel->curs_color != NULL)
		gfx_color_delete(panel->curs_color);
	if (panel->act_border_color != NULL)
		gfx_color_delete(panel->act_border_color);
	if (panel->dir_color != NULL)
		gfx_color_delete(panel->dir_color);
	if (panel->svc_color != NULL)
		gfx_color_delete(panel->svc_color);
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
	gfx_color_delete(panel->act_border_color);
	gfx_color_delete(panel->dir_color);
	gfx_color_delete(panel->svc_color);
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

	if (entry == panel->cursor && panel->active)
		fmt.color = panel->curs_color;
	else if (entry->isdir)
		fmt.color = panel->dir_color;
	else if (entry->svc != 0)
		fmt.color = panel->svc_color;
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
	ui_box_style_t bstyle;
	gfx_color_t *bcolor;
	int i, lines;
	errno_t rc;

	gfx_text_fmt_init(&fmt);

	rc = gfx_set_color(gc, panel->color);
	if (rc != EOK)
		return rc;

	rc = gfx_fill_rect(gc, &panel->rect);
	if (rc != EOK)
		return rc;

	if (panel->active) {
		bstyle = ui_box_double;
		bcolor = panel->act_border_color;
	} else {
		bstyle = ui_box_single;
		bcolor = panel->color;
	}

	rc = ui_paint_text_box(res, &panel->rect, bstyle, bcolor);
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
	if (!panel->active)
		return ui_unclaimed;

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
			case KC_ENTER:
				panel_open(panel, panel->cursor);
				break;
			default:
				break;
			}
		}
	}

	return ui_claimed;
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

/** Determine if panel is active.
 *
 * @param panel Panel
 * @return @c true iff panel is active
 */
bool panel_is_active(panel_t *panel)
{
	return panel->active;
}

/** Activate panel.
 *
 * @param panel Panel
 *
 * @return EOK on success or an error code
 */
errno_t panel_activate(panel_t *panel)
{
	errno_t rc;

	if (panel->dir != NULL) {
		rc = vfs_cwd_set(panel->dir);
		if (rc != EOK)
			return rc;
	}

	panel->active = true;
	(void) panel_paint(panel);
	return EOK;
}

/** Deactivate panel.
 *
 * @param panel Panel
 */
void panel_deactivate(panel_t *panel)
{
	panel->active = false;
	(void) panel_paint(panel);
}

/** Initialize panel entry attributes.
 *
 * @param attr Attributes
 */
void panel_entry_attr_init(panel_entry_attr_t *attr)
{
	memset(attr, 0, sizeof(*attr));
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
 * @param attr Entry attributes
 * @return EOK on success or an error code
 */
errno_t panel_entry_append(panel_t *panel, panel_entry_attr_t *attr)
{
	panel_entry_t *entry;

	entry = calloc(1, sizeof(panel_entry_t));
	if (entry == NULL)
		return ENOMEM;

	entry->panel = panel;
	entry->name = str_dup(attr->name);
	if (entry->name == NULL) {
		free(entry);
		return ENOMEM;
	}

	entry->size = attr->size;
	entry->isdir = attr->isdir;
	entry->svc = attr->svc;
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
	free((char *) entry->name);
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
	vfs_stat_t finfo;
	char newdir[256];
	char *ndir = NULL;
	panel_entry_attr_t attr;
	panel_entry_t *next;
	panel_entry_t *prev;
	char *olddn;
	size_t pg_size;
	size_t max_idx;
	size_t i;
	errno_t rc;

	rc = vfs_cwd_set(dirname);
	if (rc != EOK)
		return rc;

	rc = vfs_cwd_get(newdir, sizeof(newdir));
	if (rc != EOK)
		return rc;

	ndir = str_dup(newdir);
	if (ndir == NULL)
		return ENOMEM;

	dir = opendir(".");
	if (dir == NULL) {
		rc = errno;
		goto error;
	}

	if (str_cmp(ndir, "/") != 0) {
		/* Need to add a synthetic up-dir entry */
		panel_entry_attr_init(&attr);
		attr.name = "..";
		attr.isdir = true;

		rc = panel_entry_append(panel, &attr);
		if (rc != EOK)
			goto error;
	}

	dirent = readdir(dir);
	while (dirent != NULL) {
		rc = vfs_stat_path(dirent->d_name, &finfo);
		if (rc != EOK) {
			/* Possibly a stale entry */
			dirent = readdir(dir);
			continue;
		}

		panel_entry_attr_init(&attr);
		attr.name = dirent->d_name;
		attr.size = finfo.size;
		attr.isdir = finfo.is_directory;
		attr.svc = finfo.service;

		rc = panel_entry_append(panel, &attr);
		if (rc != EOK)
			goto error;

		dirent = readdir(dir);
	}

	closedir(dir);

	rc = panel_sort(panel);
	if (rc != EOK)
		goto error;

	panel->cursor = panel_first(panel);
	panel->cursor_idx = 0;
	panel->page = panel_first(panel);
	panel->page_idx = 0;

	/* Moving up? */
	if (str_cmp(dirname, "..") == 0) {
		/* Get the last component of old path */
		olddn = str_rchr(panel->dir, '/');
		if (olddn != NULL && *olddn != '\0') {
			/* Find corresponding entry */
			++olddn;
			next = panel_next(panel->cursor);
			while (next != NULL && str_cmp(next->name, olddn) <= 0 &&
			    next->isdir) {
				panel->cursor = next;
				++panel->cursor_idx;
				next = panel_next(panel->cursor);
			}

			/* Move page so that cursor is in the center */
			panel->page = panel->cursor;
			panel->page_idx = panel->cursor_idx;

			pg_size = panel_page_size(panel);

			for (i = 0; i < pg_size / 2; i++) {
				prev = panel_prev(panel->page);
				if (prev == NULL)
					break;

				panel->page = prev;
				--panel->page_idx;
			}

			/* Make sure page is not beyond the end if possible */
			if (panel->entries_cnt > pg_size)
				max_idx = panel->entries_cnt - pg_size;
			else
				max_idx = 0;

			while (panel->page_idx > 0 && panel->page_idx > max_idx) {
				prev = panel_prev(panel->page);
				if (prev == NULL)
					break;

				panel->page = prev;
				--panel->page_idx;
			}
		}
	}

	free(panel->dir);
	panel->dir = ndir;

	return EOK;
error:
	(void) vfs_cwd_set(panel->dir);
	if (ndir != NULL)
		free(ndir);
	if (dir != NULL)
		closedir(dir);
	return rc;
}

/** Sort panel entries.
 *
 * @param panel Panel
 * @return EOK on success, ENOMEM if out of memory
 */
errno_t panel_sort(panel_t *panel)
{
	panel_entry_t **emap;
	panel_entry_t *entry;
	size_t i;

	/* Create an array to hold pointer to each entry */
	emap = calloc(panel->entries_cnt, sizeof(panel_entry_t *));
	if (emap == NULL)
		return ENOMEM;

	/* Write entry pointers to array */
	entry = panel_first(panel);
	i = 0;
	while (entry != NULL) {
		assert(i < panel->entries_cnt);
		emap[i++] = entry;
		entry = panel_next(entry);
	}

	/* Sort the array of pointers */
	qsort(emap, panel->entries_cnt, sizeof(panel_entry_t *),
	    panel_entry_ptr_cmp);

	/* Unlink entries from entry list */
	entry = panel_first(panel);
	while (entry != NULL) {
		list_remove(&entry->lentries);
		entry = panel_first(panel);
	}

	/* Add entries back to entry list sorted */
	for (i = 0; i < panel->entries_cnt; i++)
		list_append(&emap[i]->lentries, &panel->entries);

	free(emap);
	return EOK;
}

/** Compare two panel entries indirectly referenced by pointers.
 *
 * @param pa Pointer to pointer to first entry
 * @param pb Pointer to pointer to second entry
 * @return <0, =0, >=0 if pa < b, pa == pb, pa > pb, resp.
 */
int panel_entry_ptr_cmp(const void *pa, const void *pb)
{
	panel_entry_t *a = *(panel_entry_t **)pa;
	panel_entry_t *b = *(panel_entry_t **)pb;
	int dcmp;

	/* Sort directories first */
	dcmp = b->isdir - a->isdir;
	if (dcmp != 0)
		return dcmp;

	return str_cmp(a->name, b->name);
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

	if (panel->entries_cnt > rows)
		max_idx = panel->entries_cnt - rows;
	else
		max_idx = 0;

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

/** Open panel entry.
 *
 * Perform Open action on a panel entry (e.g. switch to a subdirectory).
 *
 * @param panel Panel
 * @param entry Panel entry
 *
 * @return EOK on success or an error code
 */
errno_t panel_open(panel_t *panel, panel_entry_t *entry)
{
	if (entry->isdir)
		return panel_open_dir(panel, entry);
	else if (entry->svc == 0)
		return panel_open_file(panel, entry);
	else
		return EOK;
}

/** Open panel directory entry.
 *
 * Perform Open action on a directory entry (i.e. switch to the directory).
 *
 * @param panel Panel
 * @param entry Panel entry (which is a directory)
 *
 * @return EOK on success or an error code
 */
errno_t panel_open_dir(panel_t *panel, panel_entry_t *entry)
{
	gfx_context_t *gc = ui_window_get_gc(panel->window);
	char *dirname;
	errno_t rc;

	assert(entry->isdir);

	/*
	 * Need to copy out name before we free the entry below
	 * via panel_clear_entries().
	 */
	dirname = str_dup(entry->name);
	if (dirname == NULL)
		return ENOMEM;

	panel_clear_entries(panel);

	rc = panel_read_dir(panel, dirname);
	if (rc != EOK) {
		free(dirname);
		return rc;
	}

	free(dirname);

	rc = panel_paint(panel);
	if (rc != EOK)
		return rc;

	return gfx_update(gc);
}

/** Open panel file entry.
 *
 * Perform Open action on a file entry (i.e. try running it).
 *
 * @param panel Panel
 * @param entry Panel entry (which is a file)
 *
 * @return EOK on success or an error code
 */
errno_t panel_open_file(panel_t *panel, panel_entry_t *entry)
{
	task_id_t id;
	task_wait_t wait;
	task_exit_t texit;
	int retval;
	errno_t rc;
	ui_t *ui;

	/* It's not a directory */
	assert(!entry->isdir);
	/* It's not a service-special file */
	assert(entry->svc == 0);

	ui = ui_window_get_ui(panel->window);

	/* Free up and clean console for the child task. */
	rc = ui_suspend(ui);
	if (rc != EOK)
		return rc;

	rc = task_spawnl(&id, &wait, entry->name, entry->name, NULL);
	if (rc != EOK)
		goto error;

	rc = task_wait(&wait, &texit, &retval);
	if ((rc != EOK) || (texit != TASK_EXIT_NORMAL))
		goto error;

	/* Resume UI operation */
	rc = ui_resume(ui);
	if (rc != EOK)
		return rc;

	(void) ui_paint(ui_window_get_ui(panel->window));
	return EOK;
error:
	(void) ui_resume(ui);
	(void) ui_paint(ui_window_get_ui(panel->window));
	return rc;
}

/** @}
 */
