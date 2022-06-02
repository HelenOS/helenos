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
/** @file File list.
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
#include <ui/filelist.h>
#include <ui/paint.h>
#include <ui/resource.h>
#include <vfs/vfs.h>
#include <qsort.h>
#include "../private/filelist.h"
#include "../private/resource.h"

static void ui_file_list_ctl_destroy(void *);
static errno_t ui_file_list_ctl_paint(void *);
static ui_evclaim_t ui_file_list_ctl_kbd_event(void *, kbd_event_t *);
static ui_evclaim_t ui_file_list_ctl_pos_event(void *, pos_event_t *);

/** File list control ops */
ui_control_ops_t ui_file_list_ctl_ops = {
	.destroy = ui_file_list_ctl_destroy,
	.paint = ui_file_list_ctl_paint,
	.kbd_event = ui_file_list_ctl_kbd_event,
	.pos_event = ui_file_list_ctl_pos_event
};

enum {
	file_list_entry_hpad = 2,
	file_list_entry_vpad = 2,
	file_list_entry_hpad_text = 1,
	file_list_entry_vpad_text = 0,
};

/** Create file list.
 *
 * @param window Containing window
 * @param active @c true iff file list should be active
 * @param rflist Place to store pointer to new file list
 * @return EOK on success or an error code
 */
errno_t ui_file_list_create(ui_window_t *window, bool active,
    ui_file_list_t **rflist)
{
	ui_file_list_t *flist;
	errno_t rc;

	flist = calloc(1, sizeof(ui_file_list_t));
	if (flist == NULL)
		return ENOMEM;

	rc = ui_control_new(&ui_file_list_ctl_ops, (void *)flist,
	    &flist->control);
	if (rc != EOK) {
		free(flist);
		return rc;
	}

	rc = gfx_color_new_ega(0x0f, &flist->dir_color);
	if (rc != EOK)
		goto error;

	rc = gfx_color_new_ega(0x0a, &flist->svc_color);
	if (rc != EOK)
		goto error;

	flist->window = window;
	list_initialize(&flist->entries);
	flist->entries_cnt = 0;
	flist->active = active;

	*rflist = flist;
	return EOK;
error:
	if (flist->dir_color != NULL)
		gfx_color_delete(flist->dir_color);
	if (flist->svc_color != NULL)
		gfx_color_delete(flist->svc_color);
	ui_control_delete(flist->control);
	free(flist);
	return rc;
}

/** Destroy file list.
 *
 * @param flist File list
 */
void ui_file_list_destroy(ui_file_list_t *flist)
{
	ui_file_list_clear_entries(flist);
	ui_control_delete(flist->control);
	free(flist);
}

/** Set file list callbacks.
 *
 * @param flist File list
 * @param cb Callbacks
 * @param arg Argument to callback functions
 */
void ui_file_list_set_cb(ui_file_list_t *flist, ui_file_list_cb_t *cb, void *arg)
{
	flist->cb = cb;
	flist->cb_arg = arg;
}

/** Get height of file list entry.
 *
 * @param flist File list
 * @return Entry height in pixels
 */
gfx_coord_t ui_file_list_entry_height(ui_file_list_t *flist)
{
	ui_resource_t *res;
	gfx_font_metrics_t metrics;
	gfx_coord_t height;
	gfx_coord_t vpad;

	res = ui_window_get_res(flist->window);

	if (res->textmode) {
		vpad = file_list_entry_vpad_text;
	} else {
		vpad = file_list_entry_vpad;
	}

	/* Normal menu entry */
	gfx_font_get_metrics(res->font, &metrics);
	height = metrics.ascent + metrics.descent + 1;

	return height + 2 * vpad;
}

/** Paint file list entry.
 *
 * @param entry File list entry
 * @param entry_idx Entry index (within list of entries)
 * @return EOK on success or an error code
 */
errno_t ui_file_list_entry_paint(ui_file_list_entry_t *entry, size_t entry_idx)
{
	ui_file_list_t *flist = entry->flist;
	gfx_context_t *gc = ui_window_get_gc(flist->window);
	ui_resource_t *res = ui_window_get_res(flist->window);
	gfx_font_t *font = ui_resource_get_font(res);
	gfx_text_fmt_t fmt;
	gfx_coord2_t pos;
	gfx_rect_t rect;
	gfx_rect_t lrect;
	gfx_rect_t crect;
	gfx_color_t *bgcolor;
	char *caption;
	gfx_coord_t hpad, vpad;
	gfx_coord_t line_height;
	size_t rows;
	errno_t rc;
	int rv;

	line_height = ui_file_list_entry_height(flist);
	ui_file_list_inside_rect(entry->flist, &lrect);

	gfx_text_fmt_init(&fmt);
	fmt.font = font;
	rows = ui_file_list_page_size(flist) + 1;

	/* Do not display entry outside of current page */
	if (entry_idx < flist->page_idx ||
	    entry_idx >= flist->page_idx + rows)
		return EOK;

	if (res->textmode) {
		hpad = file_list_entry_hpad_text;
		vpad = file_list_entry_vpad_text;
	} else {
		hpad = file_list_entry_hpad;
		vpad = file_list_entry_vpad;
	}

	pos.x = lrect.p0.x;
	pos.y = lrect.p0.y + line_height * (entry_idx - flist->page_idx);

	if (entry == flist->cursor && flist->active) {
		fmt.color = res->entry_sel_text_fg_color;
		bgcolor = res->entry_sel_text_bg_color;
	} else if (entry->isdir) {
		if (res->textmode) {
			fmt.color = flist->dir_color;
			bgcolor = flist->dir_color;
		} else {
			fmt.color = res->entry_fg_color;
			bgcolor = res->entry_bg_color;
		}
	} else if (entry->svc != 0) {
		if (res->textmode) {
			fmt.color = flist->svc_color;
			bgcolor = flist->svc_color;
		} else {
			fmt.color = res->entry_fg_color;
			bgcolor = res->entry_bg_color;
		}
	} else {
		fmt.color = res->entry_fg_color;
		bgcolor = res->entry_bg_color;
	}

	/* Draw entry background */
	rect.p0 = pos;
	rect.p1.x = lrect.p1.x;
	rect.p1.y = rect.p0.y + line_height;

	/* Clip to file list interior */
	gfx_rect_clip(&rect, &lrect, &crect);

	rc = gfx_set_color(gc, bgcolor);
	if (rc != EOK)
		return rc;

	rc = gfx_fill_rect(gc, &crect);
	if (rc != EOK)
		return rc;

	/*
	 * Make sure name does not overflow the entry rectangle.
	 *
	 * XXX We probably want to measure the text width, and,
	 * if it's too long, use gfx_text_find_pos() to find where
	 * it should be cut off (and append some sort of overflow
	 * marker.
	 */
	rc = gfx_set_clip_rect(gc, &crect);
	if (rc != EOK)
		return rc;

	pos.x += hpad;
	pos.y += vpad;

	if (res->textmode || !entry->isdir) {
		rc = gfx_puttext(&pos, &fmt, entry->name);
		if (rc != EOK) {
			(void) gfx_set_clip_rect(gc, NULL);
			return rc;
		}
	} else {
		/*
		 * XXX This is mostly a hack to distinguish directories
		 * util a better solution is available. (E.g. a Size
		 * column where we can put <dir>, an icon.)
		 */
		rv = asprintf(&caption, "[%s]", entry->name);
		if (rv < 0) {
			(void) gfx_set_clip_rect(gc, NULL);
			return rc;
		}

		rc = gfx_puttext(&pos, &fmt, caption);
		if (rc != EOK) {
			free(caption);
			(void) gfx_set_clip_rect(gc, NULL);
			return rc;
		}

		free(caption);
	}

	return gfx_set_clip_rect(gc, NULL);
}

/** Paint file list.
 *
 * @param flist File list
 */
errno_t ui_file_list_paint(ui_file_list_t *flist)
{
	gfx_context_t *gc = ui_window_get_gc(flist->window);
	ui_resource_t *res = ui_window_get_res(flist->window);
	ui_file_list_entry_t *entry;
	int i, lines;
	errno_t rc;

	rc = gfx_set_color(gc, res->entry_bg_color);
	if (rc != EOK)
		return rc;

	rc = gfx_fill_rect(gc, &flist->rect);
	if (rc != EOK)
		return rc;

	if (!res->textmode) {
		rc = ui_paint_inset_frame(res, &flist->rect, NULL);
		if (rc != EOK)
			return rc;
	}

	lines = ui_file_list_page_size(flist) + 1;
	i = 0;

	entry = flist->page;
	while (entry != NULL && i < lines) {
		rc = ui_file_list_entry_paint(entry, flist->page_idx + i);
		if (rc != EOK)
			return rc;

		++i;
		entry = ui_file_list_next(entry);
	}

	rc = gfx_update(gc);
	if (rc != EOK)
		return rc;

	return EOK;
}

/** Handle file list keyboard event.
 *
 * @param flist File list
 * @param event Keyboard event
 * @return ui_claimed iff event was claimed
 */
ui_evclaim_t ui_file_list_kbd_event(ui_file_list_t *flist, kbd_event_t *event)
{
	if (!flist->active)
		return ui_unclaimed;

	if (event->type == KEY_PRESS) {
		if ((event->mods & (KM_CTRL | KM_ALT | KM_SHIFT)) == 0) {
			switch (event->key) {
			case KC_UP:
				ui_file_list_cursor_up(flist);
				break;
			case KC_DOWN:
				ui_file_list_cursor_down(flist);
				break;
			case KC_HOME:
				ui_file_list_cursor_top(flist);
				break;
			case KC_END:
				ui_file_list_cursor_bottom(flist);
				break;
			case KC_PAGE_UP:
				ui_file_list_page_up(flist);
				break;
			case KC_PAGE_DOWN:
				ui_file_list_page_down(flist);
				break;
			case KC_ENTER:
				ui_file_list_open(flist, flist->cursor);
				break;
			default:
				break;
			}
		}
	}

	return ui_claimed;
}

/** Handle file list position event.
 *
 * @param flist File list
 * @param event Position event
 * @return ui_claimed iff event was claimed
 */
ui_evclaim_t ui_file_list_pos_event(ui_file_list_t *flist, pos_event_t *event)
{
	gfx_coord2_t pos;
	gfx_rect_t irect;
	ui_file_list_entry_t *entry;
	gfx_coord_t line_height;
	size_t entry_idx;
	int n;

	line_height = ui_file_list_entry_height(flist);

	pos.x = event->hpos;
	pos.y = event->vpos;
	if (!gfx_pix_inside_rect(&pos, &flist->rect))
		return ui_unclaimed;

	if (!flist->active && event->type == POS_PRESS)
		ui_file_list_activate_req(flist);

	if (event->type == POS_PRESS || event->type == POS_DCLICK) {
		ui_file_list_inside_rect(flist, &irect);

		/* Did we click on one of the entries? */
		if (gfx_pix_inside_rect(&pos, &irect)) {
			/* Index within page */
			n = (pos.y - irect.p0.y) / line_height;

			/* Entry and its index within entire listing */
			entry = ui_file_list_page_nth_entry(flist, n, &entry_idx);
			if (entry == NULL)
				return ui_claimed;

			if (event->type == POS_PRESS) {
				/* Move to the entry found */
				ui_file_list_cursor_move(flist, entry, entry_idx);
			} else {
				/* event->type == POS_DCLICK */
				ui_file_list_open(flist, entry);
			}
		} else {
			/* It's in the border. */
			if (event->type == POS_PRESS) {
				/* Top or bottom half? */
				if (pos.y >= (irect.p0.y + irect.p1.y) / 2)
					ui_file_list_page_down(flist);
				else
					ui_file_list_page_up(flist);
			}
		}
	}

	return ui_claimed;
}

/** Get base control for file list.
 *
 * @param flist File list
 * @return Base UI control
 */
ui_control_t *ui_file_list_ctl(ui_file_list_t *flist)
{
	return flist->control;
}

/** Set file list rectangle.
 *
 * @param flist File list
 * @param rect Rectangle
 */
void ui_file_list_set_rect(ui_file_list_t *flist, gfx_rect_t *rect)
{
	flist->rect = *rect;
}

/** Get file list page size.
 *
 * @param flist File list
 * @return Number of entries that fit in flist at the same time.
 */
unsigned ui_file_list_page_size(ui_file_list_t *flist)
{
	gfx_coord_t line_height;
	gfx_rect_t irect;

	line_height = ui_file_list_entry_height(flist);
	ui_file_list_inside_rect(flist, &irect);
	return (irect.p1.y - irect.p0.y) / line_height;
}

void ui_file_list_inside_rect(ui_file_list_t *flist, gfx_rect_t *irect)
{
	ui_resource_t *res = ui_window_get_res(flist->window);

	if (res->textmode) {
		*irect = flist->rect;
	} else {
		ui_paint_get_inset_frame_inside(res, &flist->rect, irect);
	}
}

/** Determine if file list is active.
 *
 * @param flist File list
 * @return @c true iff file list is active
 */
bool ui_file_list_is_active(ui_file_list_t *flist)
{
	return flist->active;
}

/** Activate file list.
 *
 * @param flist File list
 *
 * @return EOK on success or an error code
 */
errno_t ui_file_list_activate(ui_file_list_t *flist)
{
	errno_t rc;

	if (flist->dir != NULL) {
		rc = vfs_cwd_set(flist->dir);
		if (rc != EOK)
			return rc;
	}

	flist->active = true;
	(void) ui_file_list_paint(flist);
	return EOK;
}

/** Deactivate file list.
 *
 * @param flist File list
 */
void ui_file_list_deactivate(ui_file_list_t *flist)
{
	flist->active = false;
	(void) ui_file_list_paint(flist);
}

/** Initialize file list entry attributes.
 *
 * @param attr Attributes
 */
void ui_file_list_entry_attr_init(ui_file_list_entry_attr_t *attr)
{
	memset(attr, 0, sizeof(*attr));
}

/** Destroy file list control.
 *
 * @param arg Argument (ui_file_list_t *)
 */
void ui_file_list_ctl_destroy(void *arg)
{
	ui_file_list_t *flist = (ui_file_list_t *) arg;

	ui_file_list_destroy(flist);
}

/** Paint file list control.
 *
 * @param arg Argument (ui_file_list_t *)
 * @return EOK on success or an error code
 */
errno_t ui_file_list_ctl_paint(void *arg)
{
	ui_file_list_t *flist = (ui_file_list_t *) arg;

	return ui_file_list_paint(flist);
}

/** Handle file list control keyboard event.
 *
 * @param arg Argument (ui_file_list_t *)
 * @param kbd_event Keyboard event
 * @return @c ui_claimed iff the event is claimed
 */
ui_evclaim_t ui_file_list_ctl_kbd_event(void *arg, kbd_event_t *event)
{
	ui_file_list_t *flist = (ui_file_list_t *) arg;

	return ui_file_list_kbd_event(flist, event);
}

/** Handle file list control position event.
 *
 * @param arg Argument (ui_file_list_t *)
 * @param pos_event Position event
 * @return @c ui_claimed iff the event is claimed
 */
ui_evclaim_t ui_file_list_ctl_pos_event(void *arg, pos_event_t *event)
{
	ui_file_list_t *flist = (ui_file_list_t *) arg;

	return ui_file_list_pos_event(flist, event);
}

/** Append new file list entry.
 *
 * @param flist File list
 * @param attr Entry attributes
 * @return EOK on success or an error code
 */
errno_t ui_file_list_entry_append(ui_file_list_t *flist, ui_file_list_entry_attr_t *attr)
{
	ui_file_list_entry_t *entry;

	entry = calloc(1, sizeof(ui_file_list_entry_t));
	if (entry == NULL)
		return ENOMEM;

	entry->flist = flist;
	entry->name = str_dup(attr->name);
	if (entry->name == NULL) {
		free(entry);
		return ENOMEM;
	}

	entry->size = attr->size;
	entry->isdir = attr->isdir;
	entry->svc = attr->svc;
	link_initialize(&entry->lentries);
	list_append(&entry->lentries, &flist->entries);
	++flist->entries_cnt;
	return EOK;
}

/** Delete file list entry.
 *
 * @param entry File list entry
 */
void ui_file_list_entry_delete(ui_file_list_entry_t *entry)
{
	if (entry->flist->cursor == entry)
		entry->flist->cursor = NULL;
	if (entry->flist->page == entry)
		entry->flist->page = NULL;

	list_remove(&entry->lentries);
	--entry->flist->entries_cnt;
	free((char *) entry->name);
	free(entry);
}

/** Clear file list entry list.
 *
 * @param flist File list
 */
void ui_file_list_clear_entries(ui_file_list_t *flist)
{
	ui_file_list_entry_t *entry;

	entry = ui_file_list_first(flist);
	while (entry != NULL) {
		ui_file_list_entry_delete(entry);
		entry = ui_file_list_first(flist);
	}
}

/** Read directory into file list entry list.
 *
 * @param flist File list
 * @param dirname Directory path
 * @return EOK on success or an error code
 */
errno_t ui_file_list_read_dir(ui_file_list_t *flist, const char *dirname)
{
	DIR *dir;
	struct dirent *dirent;
	vfs_stat_t finfo;
	char newdir[256];
	char *ndir = NULL;
	ui_file_list_entry_attr_t attr;
	ui_file_list_entry_t *next;
	ui_file_list_entry_t *prev;
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
		ui_file_list_entry_attr_init(&attr);
		attr.name = "..";
		attr.isdir = true;

		rc = ui_file_list_entry_append(flist, &attr);
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

		ui_file_list_entry_attr_init(&attr);
		attr.name = dirent->d_name;
		attr.size = finfo.size;
		attr.isdir = finfo.is_directory;
		attr.svc = finfo.service;

		rc = ui_file_list_entry_append(flist, &attr);
		if (rc != EOK)
			goto error;

		dirent = readdir(dir);
	}

	closedir(dir);

	rc = ui_file_list_sort(flist);
	if (rc != EOK)
		goto error;

	flist->cursor = ui_file_list_first(flist);
	flist->cursor_idx = 0;
	flist->page = ui_file_list_first(flist);
	flist->page_idx = 0;

	/* Moving up? */
	if (str_cmp(dirname, "..") == 0) {
		/* Get the last component of old path */
		olddn = str_rchr(flist->dir, '/');
		if (olddn != NULL && *olddn != '\0') {
			/* Find corresponding entry */
			++olddn;
			next = ui_file_list_next(flist->cursor);
			while (next != NULL && str_cmp(next->name, olddn) <= 0 &&
			    next->isdir) {
				flist->cursor = next;
				++flist->cursor_idx;
				next = ui_file_list_next(flist->cursor);
			}

			/* Move page so that cursor is in the center */
			flist->page = flist->cursor;
			flist->page_idx = flist->cursor_idx;

			pg_size = ui_file_list_page_size(flist);

			for (i = 0; i < pg_size / 2; i++) {
				prev = ui_file_list_prev(flist->page);
				if (prev == NULL)
					break;

				flist->page = prev;
				--flist->page_idx;
			}

			/* Make sure page is not beyond the end if possible */
			if (flist->entries_cnt > pg_size)
				max_idx = flist->entries_cnt - pg_size;
			else
				max_idx = 0;

			while (flist->page_idx > 0 && flist->page_idx > max_idx) {
				prev = ui_file_list_prev(flist->page);
				if (prev == NULL)
					break;

				flist->page = prev;
				--flist->page_idx;
			}
		}
	}

	free(flist->dir);
	flist->dir = ndir;

	return EOK;
error:
	(void) vfs_cwd_set(flist->dir);
	if (ndir != NULL)
		free(ndir);
	if (dir != NULL)
		closedir(dir);
	return rc;
}

/** Sort file list entries.
 *
 * @param flist File list
 * @return EOK on success, ENOMEM if out of memory
 */
errno_t ui_file_list_sort(ui_file_list_t *flist)
{
	ui_file_list_entry_t **emap;
	ui_file_list_entry_t *entry;
	size_t i;

	/* Create an array to hold pointer to each entry */
	emap = calloc(flist->entries_cnt, sizeof(ui_file_list_entry_t *));
	if (emap == NULL)
		return ENOMEM;

	/* Write entry pointers to array */
	entry = ui_file_list_first(flist);
	i = 0;
	while (entry != NULL) {
		assert(i < flist->entries_cnt);
		emap[i++] = entry;
		entry = ui_file_list_next(entry);
	}

	/* Sort the array of pointers */
	qsort(emap, flist->entries_cnt, sizeof(ui_file_list_entry_t *),
	    ui_file_list_entry_ptr_cmp);

	/* Unlink entries from entry list */
	entry = ui_file_list_first(flist);
	while (entry != NULL) {
		list_remove(&entry->lentries);
		entry = ui_file_list_first(flist);
	}

	/* Add entries back to entry list sorted */
	for (i = 0; i < flist->entries_cnt; i++)
		list_append(&emap[i]->lentries, &flist->entries);

	free(emap);
	return EOK;
}

/** Compare two file list entries indirectly referenced by pointers.
 *
 * @param pa Pointer to pointer to first entry
 * @param pb Pointer to pointer to second entry
 * @return <0, =0, >=0 if pa < b, pa == pb, pa > pb, resp.
 */
int ui_file_list_entry_ptr_cmp(const void *pa, const void *pb)
{
	ui_file_list_entry_t *a = *(ui_file_list_entry_t **)pa;
	ui_file_list_entry_t *b = *(ui_file_list_entry_t **)pb;
	int dcmp;

	/* Sort directories first */
	dcmp = b->isdir - a->isdir;
	if (dcmp != 0)
		return dcmp;

	return str_cmp(a->name, b->name);
}

/** Return first file list entry.
 *
 * @param flist File list
 * @return First file list entry or @c NULL if there are no entries
 */
ui_file_list_entry_t *ui_file_list_first(ui_file_list_t *flist)
{
	link_t *link;

	link = list_first(&flist->entries);
	if (link == NULL)
		return NULL;

	return list_get_instance(link, ui_file_list_entry_t, lentries);
}

/** Return last file list entry.
 *
 * @param flist File list
 * @return Last file list entry or @c NULL if there are no entries
 */
ui_file_list_entry_t *ui_file_list_last(ui_file_list_t *flist)
{
	link_t *link;

	link = list_last(&flist->entries);
	if (link == NULL)
		return NULL;

	return list_get_instance(link, ui_file_list_entry_t, lentries);
}

/** Return next file list entry.
 *
 * @param cur Current entry
 * @return Next entry or @c NULL if @a cur is the last entry
 */
ui_file_list_entry_t *ui_file_list_next(ui_file_list_entry_t *cur)
{
	link_t *link;

	link = list_next(&cur->lentries, &cur->flist->entries);
	if (link == NULL)
		return NULL;

	return list_get_instance(link, ui_file_list_entry_t, lentries);
}

/** Return previous file list entry.
 *
 * @param cur Current entry
 * @return Previous entry or @c NULL if @a cur is the first entry
 */
ui_file_list_entry_t *ui_file_list_prev(ui_file_list_entry_t *cur)
{
	link_t *link;

	link = list_prev(&cur->lentries, &cur->flist->entries);
	if (link == NULL)
		return NULL;

	return list_get_instance(link, ui_file_list_entry_t, lentries);
}

/** Find the n-th entry of the current file list page.
 *
 * @param flist File list
 * @param n Which entry to get (starting from 0)
 * @param ridx Place to store index (within listing) of the entry
 * @return n-th entry of the page
 */
ui_file_list_entry_t *ui_file_list_page_nth_entry(ui_file_list_t *flist,
    size_t n, size_t *ridx)
{
	ui_file_list_entry_t *entry;
	size_t i;
	size_t idx;

	assert(n <= ui_file_list_page_size(flist));

	entry = flist->page;
	if (entry == NULL)
		return NULL;

	idx = flist->page_idx;
	for (i = 0; i < n; i++) {
		entry = ui_file_list_next(entry);
		if (entry == NULL)
			return NULL;

		++idx;
	}

	*ridx = idx;
	return entry;
}

/** Move cursor to a new position, possibly scrolling.
 *
 * @param flist File list
 * @param entry New entry under cursor
 * @param entry_idx Index of new entry under cursor
 */
void ui_file_list_cursor_move(ui_file_list_t *flist,
    ui_file_list_entry_t *entry, size_t entry_idx)
{
	gfx_context_t *gc = ui_window_get_gc(flist->window);
	ui_file_list_entry_t *old_cursor;
	size_t old_idx;
	size_t rows;
	ui_file_list_entry_t *e;
	size_t i;

	rows = ui_file_list_page_size(flist);

	old_cursor = flist->cursor;
	old_idx = flist->cursor_idx;

	flist->cursor = entry;
	flist->cursor_idx = entry_idx;

	if (entry_idx >= flist->page_idx &&
	    entry_idx < flist->page_idx + rows) {
		/*
		 * If cursor is still on the current page, we're not
		 * scrolling. Just unpaint old cursor and paint new
		 * cursor.
		 */
		ui_file_list_entry_paint(old_cursor, old_idx);
		ui_file_list_entry_paint(flist->cursor, flist->cursor_idx);

		(void) gfx_update(gc);
	} else {
		/*
		 * Need to scroll and update all rows.
		 */

		/* Scrolling up */
		if (entry_idx < flist->page_idx) {
			flist->page = entry;
			flist->page_idx = entry_idx;
		}

		/* Scrolling down */
		if (entry_idx >= flist->page_idx + rows) {
			if (entry_idx >= rows) {
				flist->page_idx = entry_idx - rows + 1;
				/* Find first page entry (go back rows - 1) */
				e = entry;
				for (i = 0; i < rows - 1; i++) {
					e = ui_file_list_prev(e);
				}

				/* Should be valid */
				assert(e != NULL);
				flist->page = e;
			} else {
				flist->page = ui_file_list_first(flist);
				flist->page_idx = 0;
			}
		}

		(void) ui_file_list_paint(flist);
	}
}

/** Move cursor one entry up.
 *
 * @param flist File list
 */
void ui_file_list_cursor_up(ui_file_list_t *flist)
{
	ui_file_list_entry_t *prev;
	size_t prev_idx;

	prev = ui_file_list_prev(flist->cursor);
	prev_idx = flist->cursor_idx - 1;
	if (prev != NULL)
		ui_file_list_cursor_move(flist, prev, prev_idx);
}

/** Move cursor one entry down.
 *
 * @param flist File list
 */
void ui_file_list_cursor_down(ui_file_list_t *flist)
{
	ui_file_list_entry_t *next;
	size_t next_idx;

	next = ui_file_list_next(flist->cursor);
	next_idx = flist->cursor_idx + 1;
	if (next != NULL)
		ui_file_list_cursor_move(flist, next, next_idx);
}

/** Move cursor to top.
 *
 * @param flist File list
 */
void ui_file_list_cursor_top(ui_file_list_t *flist)
{
	ui_file_list_cursor_move(flist, ui_file_list_first(flist), 0);
}

/** Move cursor to bottom.
 *
 * @param flist File list
 */
void ui_file_list_cursor_bottom(ui_file_list_t *flist)
{
	ui_file_list_cursor_move(flist, ui_file_list_last(flist),
	    flist->entries_cnt - 1);
}

/** Move one page up.
 *
 * @param flist File list
 */
void ui_file_list_page_up(ui_file_list_t *flist)
{
	gfx_context_t *gc = ui_window_get_gc(flist->window);
	ui_file_list_entry_t *old_page;
	ui_file_list_entry_t *old_cursor;
	size_t old_idx;
	size_t rows;
	ui_file_list_entry_t *entry;
	size_t i;

	rows = ui_file_list_page_size(flist);

	old_page = flist->page;
	old_cursor = flist->cursor;
	old_idx = flist->cursor_idx;

	/* Move page by rows entries up (if possible) */
	for (i = 0; i < rows; i++) {
		entry = ui_file_list_prev(flist->page);
		if (entry != NULL) {
			flist->page = entry;
			--flist->page_idx;
		}
	}

	/* Move cursor by rows entries up (if possible) */

	for (i = 0; i < rows; i++) {
		entry = ui_file_list_prev(flist->cursor);
		if (entry != NULL) {
			flist->cursor = entry;
			--flist->cursor_idx;
		}
	}

	if (flist->page != old_page) {
		/* We have scrolled. Need to repaint all entries */
		(void) ui_file_list_paint(flist);
	} else if (flist->cursor != old_cursor) {
		/* No scrolling, but cursor has moved */
		ui_file_list_entry_paint(old_cursor, old_idx);
		ui_file_list_entry_paint(flist->cursor, flist->cursor_idx);

		(void) gfx_update(gc);
	}
}

/** Move one page down.
 *
 * @param flist File list
 */
void ui_file_list_page_down(ui_file_list_t *flist)
{
	gfx_context_t *gc = ui_window_get_gc(flist->window);
	ui_file_list_entry_t *old_page;
	ui_file_list_entry_t *old_cursor;
	size_t old_idx;
	size_t max_idx;
	size_t rows;
	ui_file_list_entry_t *entry;
	size_t i;

	rows = ui_file_list_page_size(flist);

	old_page = flist->page;
	old_cursor = flist->cursor;
	old_idx = flist->cursor_idx;

	if (flist->entries_cnt > rows)
		max_idx = flist->entries_cnt - rows;
	else
		max_idx = 0;

	/* Move page by rows entries down (if possible) */
	for (i = 0; i < rows; i++) {
		entry = ui_file_list_next(flist->page);
		/* Do not scroll that results in a short page */
		if (entry != NULL && flist->page_idx < max_idx) {
			flist->page = entry;
			++flist->page_idx;
		}
	}

	/* Move cursor by rows entries down (if possible) */

	for (i = 0; i < rows; i++) {
		entry = ui_file_list_next(flist->cursor);
		if (entry != NULL) {
			flist->cursor = entry;
			++flist->cursor_idx;
		}
	}

	if (flist->page != old_page) {
		/* We have scrolled. Need to repaint all entries */
		(void) ui_file_list_paint(flist);
	} else if (flist->cursor != old_cursor) {
		/* No scrolling, but cursor has moved */
		ui_file_list_entry_paint(old_cursor, old_idx);
		ui_file_list_entry_paint(flist->cursor, flist->cursor_idx);

		(void) gfx_update(gc);
	}
}

/** Open file list entry.
 *
 * Perform Open action on a file list entry (e.g. switch to a subdirectory).
 *
 * @param flist File list
 * @param entry File list entry
 *
 * @return EOK on success or an error code
 */
errno_t ui_file_list_open(ui_file_list_t *flist, ui_file_list_entry_t *entry)
{
	if (entry->isdir)
		return ui_file_list_open_dir(flist, entry);
	else if (entry->svc == 0)
		return ui_file_list_open_file(flist, entry);
	else
		return EOK;
}

/** Open file list directory entry.
 *
 * Perform Open action on a directory entry (i.e. switch to the directory).
 *
 * @param flist File list
 * @param entry File list entry (which is a directory)
 *
 * @return EOK on success or an error code
 */
errno_t ui_file_list_open_dir(ui_file_list_t *flist,
    ui_file_list_entry_t *entry)
{
	gfx_context_t *gc = ui_window_get_gc(flist->window);
	char *dirname;
	errno_t rc;

	assert(entry->isdir);

	/*
	 * Need to copy out name before we free the entry below
	 * via ui_file_list_clear_entries().
	 */
	dirname = str_dup(entry->name);
	if (dirname == NULL)
		return ENOMEM;

	ui_file_list_clear_entries(flist);

	rc = ui_file_list_read_dir(flist, dirname);
	if (rc != EOK) {
		free(dirname);
		return rc;
	}

	free(dirname);

	rc = ui_file_list_paint(flist);
	if (rc != EOK)
		return rc;

	return gfx_update(gc);
}

/** Open file list file entry.
 *
 * Perform Open action on a file entry (i.e. try running it).
 *
 * @param flist File list
 * @param entry File list entry (which is a file)
 *
 * @return EOK on success or an error code
 */
errno_t ui_file_list_open_file(ui_file_list_t *flist,
    ui_file_list_entry_t *entry)
{
	ui_file_list_selected(flist, entry->name);
	return EOK;
}

/** Request file list activation.
 *
 * Call back to request file list activation.
 *
 * @param flist File list
 */
void ui_file_list_activate_req(ui_file_list_t *flist)
{
	if (flist->cb != NULL && flist->cb->activate_req != NULL)
		flist->cb->activate_req(flist, flist->cb_arg);
}

/** Call back when a file is selected.
 *
 * @param flist File list
 * @param fname File name
 */
void ui_file_list_selected(ui_file_list_t *flist, const char *fname)
{
	if (flist->cb != NULL && flist->cb->selected != NULL)
		flist->cb->selected(flist, flist->cb_arg, fname);
}

/** @}
 */
