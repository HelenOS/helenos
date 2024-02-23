/*
 * Copyright (c) 2024 Jiri Svoboda
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
/** @file List.
 *
 * Simple list control.
 */

#include <errno.h>
#include <gfx/render.h>
#include <gfx/text.h>
#include <stdlib.h>
#include <str.h>
#include <ui/control.h>
#include <ui/list.h>
#include <ui/paint.h>
#include <ui/resource.h>
#include <ui/scrollbar.h>
#include "../private/list.h"
#include "../private/resource.h"

static void ui_list_ctl_destroy(void *);
static errno_t ui_list_ctl_paint(void *);
static ui_evclaim_t ui_list_ctl_kbd_event(void *, kbd_event_t *);
static ui_evclaim_t ui_list_ctl_pos_event(void *, pos_event_t *);

/** List control ops */
ui_control_ops_t ui_list_ctl_ops = {
	.destroy = ui_list_ctl_destroy,
	.paint = ui_list_ctl_paint,
	.kbd_event = ui_list_ctl_kbd_event,
	.pos_event = ui_list_ctl_pos_event
};

enum {
	list_entry_hpad = 2,
	list_entry_vpad = 2,
	list_entry_hpad_text = 1,
	list_entry_vpad_text = 0,
};

static void ui_list_scrollbar_up(ui_scrollbar_t *, void *);
static void ui_list_scrollbar_down(ui_scrollbar_t *, void *);
static void ui_list_scrollbar_page_up(ui_scrollbar_t *, void *);
static void ui_list_scrollbar_page_down(ui_scrollbar_t *, void *);
static void ui_list_scrollbar_moved(ui_scrollbar_t *, void *, gfx_coord_t);

/** List scrollbar callbacks */
static ui_scrollbar_cb_t ui_list_scrollbar_cb = {
	.up = ui_list_scrollbar_up,
	.down = ui_list_scrollbar_down,
	.page_up = ui_list_scrollbar_page_up,
	.page_down = ui_list_scrollbar_page_down,
	.moved = ui_list_scrollbar_moved
};

/** Create UI list.
 *
 * @param window Containing window
 * @param active @c true iff list should be active
 * @param rlist Place to store pointer to new list
 * @return EOK on success or an error code
 */
errno_t ui_list_create(ui_window_t *window, bool active,
    ui_list_t **rlist)
{
	ui_list_t *list;
	errno_t rc;

	list = calloc(1, sizeof(ui_list_t));
	if (list == NULL)
		return ENOMEM;

	rc = ui_control_new(&ui_list_ctl_ops, (void *)list,
	    &list->control);
	if (rc != EOK) {
		free(list);
		return rc;
	}

	rc = ui_scrollbar_create(ui_window_get_ui(window), window,
	    ui_sbd_vert, &list->scrollbar);
	if (rc != EOK)
		goto error;

	ui_scrollbar_set_cb(list->scrollbar, &ui_list_scrollbar_cb,
	    (void *) list);

	list->window = window;
	list_initialize(&list->entries);
	list->entries_cnt = 0;
	list->active = active;

	*rlist = list;
	return EOK;
error:
	ui_control_delete(list->control);
	free(list);
	return rc;
}

/** Destroy UI list.
 *
 * @param list UI list
 */
void ui_list_destroy(ui_list_t *list)
{
	ui_list_clear_entries(list);
	ui_control_delete(list->control);
	free(list);
}

/** Set UI list callbacks.
 *
 * @param list UI list
 * @param cb Callbacks
 * @param arg Argument to callback functions
 */
void ui_list_set_cb(ui_list_t *list, ui_list_cb_t *cb, void *arg)
{
	list->cb = cb;
	list->cb_arg = arg;
}

/** Get UI list callback argument.
 *
 * @param list UI list
 * @return Callback argument
 */
void *ui_list_get_cb_arg(ui_list_t *list)
{
	return list->cb_arg;
}

/** Get height of list entry.
 *
 * @param list UI list
 * @return Entry height in pixels
 */
gfx_coord_t ui_list_entry_height(ui_list_t *list)
{
	ui_resource_t *res;
	gfx_font_metrics_t metrics;
	gfx_coord_t height;
	gfx_coord_t vpad;

	res = ui_window_get_res(list->window);

	if (res->textmode) {
		vpad = list_entry_vpad_text;
	} else {
		vpad = list_entry_vpad;
	}

	/* Normal menu entry */
	gfx_font_get_metrics(res->font, &metrics);
	height = metrics.ascent + metrics.descent + 1;

	return height + 2 * vpad;
}

/** Paint list entry.
 *
 * @param entry List entry
 * @param entry_idx Entry index (within list of entries)
 * @return EOK on success or an error code
 */
errno_t ui_list_entry_paint(ui_list_entry_t *entry, size_t entry_idx)
{
	ui_list_t *list = entry->list;
	gfx_context_t *gc = ui_window_get_gc(list->window);
	ui_resource_t *res = ui_window_get_res(list->window);
	gfx_font_t *font = ui_resource_get_font(res);
	gfx_text_fmt_t fmt;
	gfx_coord2_t pos;
	gfx_rect_t rect;
	gfx_rect_t lrect;
	gfx_rect_t crect;
	gfx_color_t *bgcolor;
	gfx_coord_t hpad, vpad;
	gfx_coord_t line_height;
	size_t rows;
	errno_t rc;

	line_height = ui_list_entry_height(list);
	ui_list_inside_rect(entry->list, &lrect);

	gfx_text_fmt_init(&fmt);
	fmt.font = font;
	rows = ui_list_page_size(list) + 1;

	/* Do not display entry outside of current page */
	if (entry_idx < list->page_idx ||
	    entry_idx >= list->page_idx + rows)
		return EOK;

	if (res->textmode) {
		hpad = list_entry_hpad_text;
		vpad = list_entry_vpad_text;
	} else {
		hpad = list_entry_hpad;
		vpad = list_entry_vpad;
	}

	pos.x = lrect.p0.x;
	pos.y = lrect.p0.y + line_height * (entry_idx - list->page_idx);

	if (entry == list->cursor && list->active) {
		fmt.color = res->entry_sel_text_fg_color;
		bgcolor = res->entry_sel_text_bg_color;
	} else {
		if (entry->color != NULL)
			fmt.color = entry->color;
		else
			fmt.color = res->entry_fg_color;

		if (entry->bgcolor != NULL)
			bgcolor = entry->bgcolor;
		else
			bgcolor = res->entry_bg_color;
	}

	/* Draw entry background */
	rect.p0 = pos;
	rect.p1.x = lrect.p1.x;
	rect.p1.y = rect.p0.y + line_height;

	/* Clip to list interior */
	gfx_rect_clip(&rect, &lrect, &crect);

	rc = gfx_set_color(gc, bgcolor);
	if (rc != EOK)
		return rc;

	rc = gfx_fill_rect(gc, &crect);
	if (rc != EOK)
		return rc;

	/*
	 * Make sure caption does not overflow the entry rectangle.
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

	rc = gfx_puttext(&pos, &fmt, entry->caption);
	if (rc != EOK) {
		(void) gfx_set_clip_rect(gc, NULL);
		return rc;
	}

	return gfx_set_clip_rect(gc, NULL);
}

/** Paint UI list.
 *
 * @param list UI list
 */
errno_t ui_list_paint(ui_list_t *list)
{
	gfx_context_t *gc = ui_window_get_gc(list->window);
	ui_resource_t *res = ui_window_get_res(list->window);
	ui_list_entry_t *entry;
	int i, lines;
	errno_t rc;

	rc = gfx_set_color(gc, res->entry_bg_color);
	if (rc != EOK)
		return rc;

	rc = gfx_fill_rect(gc, &list->rect);
	if (rc != EOK)
		return rc;

	if (!res->textmode) {
		rc = ui_paint_inset_frame(res, &list->rect, NULL);
		if (rc != EOK)
			return rc;
	}

	lines = ui_list_page_size(list) + 1;
	i = 0;

	entry = list->page;
	while (entry != NULL && i < lines) {
		rc = ui_list_entry_paint(entry, list->page_idx + i);
		if (rc != EOK)
			return rc;

		++i;
		entry = ui_list_next(entry);
	}

	rc = ui_scrollbar_paint(list->scrollbar);
	if (rc != EOK)
		return rc;

	rc = gfx_update(gc);
	if (rc != EOK)
		return rc;

	return EOK;
}

/** Handle list keyboard event.
 *
 * @param list UI list
 * @param event Keyboard event
 * @return ui_claimed iff event was claimed
 */
ui_evclaim_t ui_list_kbd_event(ui_list_t *list, kbd_event_t *event)
{
	if (!list->active)
		return ui_unclaimed;

	if (event->type == KEY_PRESS) {
		if ((event->mods & (KM_CTRL | KM_ALT | KM_SHIFT)) == 0) {
			switch (event->key) {
			case KC_UP:
				ui_list_cursor_up(list);
				break;
			case KC_DOWN:
				ui_list_cursor_down(list);
				break;
			case KC_HOME:
				ui_list_cursor_top(list);
				break;
			case KC_END:
				ui_list_cursor_bottom(list);
				break;
			case KC_PAGE_UP:
				ui_list_page_up(list);
				break;
			case KC_PAGE_DOWN:
				ui_list_page_down(list);
				break;
			case KC_ENTER:
				ui_list_selected(list->cursor);
				break;
			default:
				break;
			}
		}
	}

	return ui_claimed;
}

/** Handle UI list position event.
 *
 * @param list UI list
 * @param event Position event
 * @return ui_claimed iff event was claimed
 */
ui_evclaim_t ui_list_pos_event(ui_list_t *list, pos_event_t *event)
{
	gfx_coord2_t pos;
	gfx_rect_t irect;
	ui_list_entry_t *entry;
	gfx_coord_t line_height;
	size_t entry_idx;
	ui_evclaim_t claim;
	int n;

	claim = ui_scrollbar_pos_event(list->scrollbar, event);
	if (claim == ui_claimed)
		return ui_claimed;

	line_height = ui_list_entry_height(list);

	pos.x = event->hpos;
	pos.y = event->vpos;
	if (!gfx_pix_inside_rect(&pos, &list->rect))
		return ui_unclaimed;

	if (event->type == POS_PRESS || event->type == POS_DCLICK) {
		ui_list_inside_rect(list, &irect);

		/* Did we click on one of the entries? */
		if (gfx_pix_inside_rect(&pos, &irect)) {
			/* Index within page */
			n = (pos.y - irect.p0.y) / line_height;

			/* Entry and its index within entire listing */
			entry = ui_list_page_nth_entry(list, n, &entry_idx);
			if (entry == NULL)
				return ui_claimed;

			if (event->type == POS_PRESS) {
				/* Move to the entry found */
				ui_list_cursor_move(list, entry, entry_idx);
			} else {
				/* event->type == POS_DCLICK */
				ui_list_selected(entry);
			}
		} else {
			/* It's in the border. */
			if (event->type == POS_PRESS) {
				/* Top or bottom half? */
				if (pos.y >= (irect.p0.y + irect.p1.y) / 2)
					ui_list_page_down(list);
				else
					ui_list_page_up(list);
			}
		}
	}

	if (!list->active && event->type == POS_PRESS)
		ui_list_activate_req(list);

	return ui_claimed;
}

/** Get base control for UI list.
 *
 * @param list UI list
 * @return Base UI control
 */
ui_control_t *ui_list_ctl(ui_list_t *list)
{
	return list->control;
}

/** Set UI list rectangle.
 *
 * @param list UI list
 * @param rect Rectangle
 */
void ui_list_set_rect(ui_list_t *list, gfx_rect_t *rect)
{
	gfx_rect_t srect;

	list->rect = *rect;

	ui_list_scrollbar_rect(list, &srect);
	ui_scrollbar_set_rect(list->scrollbar, &srect);
}

/** Get UI list page size.
 *
 * @param list UI list
 * @return Number of entries that fit in list at the same time.
 */
unsigned ui_list_page_size(ui_list_t *list)
{
	gfx_coord_t line_height;
	gfx_rect_t irect;

	line_height = ui_list_entry_height(list);
	ui_list_inside_rect(list, &irect);
	return (irect.p1.y - irect.p0.y) / line_height;
}

/** Get UI list interior rectangle.
 *
 * @param list UI list
 * @param irect Place to store interior rectangle
 */
void ui_list_inside_rect(ui_list_t *list, gfx_rect_t *irect)
{
	ui_resource_t *res = ui_window_get_res(list->window);
	gfx_rect_t rect;
	gfx_coord_t width;

	if (res->textmode) {
		rect = list->rect;
	} else {
		ui_paint_get_inset_frame_inside(res, &list->rect, &rect);
	}

	if (res->textmode) {
		width = 1;
	} else {
		width = 23;
	}

	irect->p0 = rect.p0;
	irect->p1.x = rect.p1.x - width;
	irect->p1.y = rect.p1.y;
}

/** Get UI list scrollbar rectangle.
 *
 * @param list UI list
 * @param irect Place to store interior rectangle
 */
void ui_list_scrollbar_rect(ui_list_t *list, gfx_rect_t *srect)
{
	ui_resource_t *res = ui_window_get_res(list->window);
	gfx_rect_t rect;
	gfx_coord_t width;

	if (res->textmode) {
		rect = list->rect;
	} else {
		ui_paint_get_inset_frame_inside(res, &list->rect, &rect);
	}

	if (res->textmode) {
		width = 1;
	} else {
		width = 23;
	}

	srect->p0.x = rect.p1.x - width;
	srect->p0.y = rect.p0.y;
	srect->p1 = rect.p1;
}

/** Compute new position for UI list scrollbar thumb.
 *
 * @param list UI list
 * @return New position
 */
gfx_coord_t ui_list_scrollbar_pos(ui_list_t *list)
{
	size_t entries;
	size_t pglen;
	size_t sbar_len;

	entries = list_count(&list->entries);
	pglen = ui_list_page_size(list);
	sbar_len = ui_scrollbar_move_length(list->scrollbar);

	if (entries > pglen)
		return sbar_len * list->page_idx / (entries - pglen);
	else
		return 0;
}

/** Update UI list scrollbar position.
 *
 * @param list UI list
 */
void ui_list_scrollbar_update(ui_list_t *list)
{
	ui_scrollbar_set_pos(list->scrollbar,
	    ui_list_scrollbar_pos(list));
}

/** Determine if UI list is active.
 *
 * @param list UI list
 * @return @c true iff UI list is active
 */
bool ui_list_is_active(ui_list_t *list)
{
	return list->active;
}

/** Activate UI list.
 *
 * @param list UI list
 *
 * @return EOK on success or an error code
 */
errno_t ui_list_activate(ui_list_t *list)
{
	list->active = true;
	(void) ui_list_paint(list);
	return EOK;
}

/** Deactivate UI list.
 *
 * @param list UI list
 */
void ui_list_deactivate(ui_list_t *list)
{
	list->active = false;
	(void) ui_list_paint(list);
}

/** Initialize UI list entry attributes.
 *
 * @param attr Attributes
 */
void ui_list_entry_attr_init(ui_list_entry_attr_t *attr)
{
	memset(attr, 0, sizeof(*attr));
}

/** Destroy UI list control.
 *
 * @param arg Argument (ui_list_t *)
 */
void ui_list_ctl_destroy(void *arg)
{
	ui_list_t *list = (ui_list_t *) arg;

	ui_list_destroy(list);
}

/** Paint UI list control.
 *
 * @param arg Argument (ui_list_t *)
 * @return EOK on success or an error code
 */
errno_t ui_list_ctl_paint(void *arg)
{
	ui_list_t *list = (ui_list_t *) arg;

	return ui_list_paint(list);
}

/** Handle UI list control keyboard event.
 *
 * @param arg Argument (ui_list_t *)
 * @param kbd_event Keyboard event
 * @return @c ui_claimed iff the event is claimed
 */
ui_evclaim_t ui_list_ctl_kbd_event(void *arg, kbd_event_t *event)
{
	ui_list_t *list = (ui_list_t *) arg;

	return ui_list_kbd_event(list, event);
}

/** Handle UI list control position event.
 *
 * @param arg Argument (ui_list_t *)
 * @param pos_event Position event
 * @return @c ui_claimed iff the event is claimed
 */
ui_evclaim_t ui_list_ctl_pos_event(void *arg, pos_event_t *event)
{
	ui_list_t *list = (ui_list_t *) arg;

	return ui_list_pos_event(list, event);
}

/** Append new UI list entry.
 *
 * @param list UI list
 * @param attr Entry attributes
 * @param rentry Place to store pointer to new entry or @c NULL if not
 *               interested
 * @return EOK on success or an error code
 */
errno_t ui_list_entry_append(ui_list_t *list, ui_list_entry_attr_t *attr,
    ui_list_entry_t **rentry)
{
	ui_list_entry_t *entry;

	entry = calloc(1, sizeof(ui_list_entry_t));
	if (entry == NULL)
		return ENOMEM;

	entry->list = list;
	entry->caption = str_dup(attr->caption);
	if (entry->caption == NULL) {
		free(entry);
		return ENOMEM;
	}

	entry->arg = attr->arg;
	entry->color = attr->color;
	entry->bgcolor = attr->bgcolor;
	link_initialize(&entry->lentries);
	list_append(&entry->lentries, &list->entries);

	if (list->entries_cnt == 0) {
		/* Adding first entry - need to set the cursor */
		list->cursor = entry;
		list->cursor_idx = 0;
		list->page = entry;
		list->page_idx = 0;
	}

	++list->entries_cnt;

	if (rentry != NULL)
		*rentry = entry;
	return EOK;
}

/** Move UI list entry one position up.
 *
 * @parm entry UI list entry
 */
void ui_list_entry_move_up(ui_list_entry_t *entry)
{
	ui_list_t *list = entry->list;
	ui_list_entry_t *prev;

	prev = ui_list_prev(entry);
	if (prev == NULL) {
		/* Entry is already on first position, nothing to do. */
		return;
	}

	list_remove(&entry->lentries);
	list_insert_before(&entry->lentries, &prev->lentries);

	/* Make sure page stays on the same position/idx as it was before */
	if (list->page == entry) {
		list->page = prev;
	} else if (list->page == prev) {
		list->page = entry;
	}

	/*
	 * Return cursor to the same position/idx as it was before,
	 * but then move it using ui_list_cursor_move() to the new
	 * position (this ensures scrolling when needed).
	 */
	if (list->cursor == entry) {
		list->cursor = prev;
		ui_list_cursor_move(list, entry, list->cursor_idx - 1);
	} else if (list->cursor == prev) {
		list->cursor = entry;
		ui_list_cursor_move(list, prev, list->cursor_idx + 1);
	}
}

/** Move UI list entry one position down.
 *
 * @parm entry UI list entry
 */
void ui_list_entry_move_down(ui_list_entry_t *entry)
{
	ui_list_t *list = entry->list;
	ui_list_entry_t *next;

	next = ui_list_next(entry);
	if (next == NULL) {
		/* Entry is already on last position, nothing to do. */
		return;
	}

	list_remove(&entry->lentries);
	list_insert_after(&entry->lentries, &next->lentries);

	/* Make sure page stays on the same position/idx as it was before */
	if (list->page == entry) {
		list->page = next;
	} else if (list->page == next) {
		list->page = entry;
	}

	/*
	 * Return cursor to the same position/idx as it was before,
	 * but then move it using ui_list_cursor_move() to the new
	 * position (this ensures scrolling when needed).
	 */
	if (list->cursor == entry) {
		list->cursor = next;
		ui_list_cursor_move(list, entry, list->cursor_idx + 1);
	} else if (list->cursor == next) {
		list->cursor = entry;
		ui_list_cursor_move(list, next, list->cursor_idx - 1);
	}
}

/** Destroy UI list entry.
 *
 * This is the quick way, but does not update cursor or page position.
 *
 * @param entry UI list entry
 */
void ui_list_entry_destroy(ui_list_entry_t *entry)
{
	if (entry->list->cursor == entry)
		entry->list->cursor = NULL;
	if (entry->list->page == entry)
		entry->list->page = NULL;

	list_remove(&entry->lentries);
	--entry->list->entries_cnt;
	free((char *) entry->caption);
	free(entry);
}

/** Delete UI list entry.
 *
 * If required, update cursor and page position and repaint.
 *
 * @param entry UI list entry
 */
void ui_list_entry_delete(ui_list_entry_t *entry)
{
	ui_list_t *list = entry->list;

	/* Try to make sure entry does not disappear under cursor or page */
	if (entry->list->cursor == entry)
		ui_list_cursor_up(entry->list);
	if (entry->list->cursor == entry)
		ui_list_cursor_down(entry->list);
	if (entry->list->page == entry)
		ui_list_scroll_up(entry->list);
	if (entry->list->page == entry)
		ui_list_scroll_down(entry->list);

	ui_list_entry_destroy(entry);

	/*
	 * But it could still happen if there are not enough entries.
	 * In that case just move page and/or cursor to the first
	 * entry.
	 */
	if (list->page == NULL) {
		list->page = ui_list_first(list);
		list->page_idx = 0;
	} else {
		/*
		 * Entry index might have changed if earlier entry
		 * was deleted.
		 */
		list->page_idx = ui_list_entry_get_idx(list->page);
	}

	if (list->cursor == NULL) {
		list->cursor = ui_list_first(list);
		list->cursor_idx = 0;
	} else {
		/*
		 * Entry index might have changed if earlier entry
		 * was deleted.
		 */
		list->cursor_idx = ui_list_entry_get_idx(list->cursor);
	}
}

/** Get entry argument.
 *
 * @param entry UI list entry
 * @return User argument
 */
void *ui_list_entry_get_arg(ui_list_entry_t *entry)
{
	return entry->arg;
}

/** Get containing list.
 *
 * @param entry UI list entry
 * @return Containing list
 */
ui_list_t *ui_list_entry_get_list(ui_list_entry_t *entry)
{
	return entry->list;
}

/** Change list entry caption.
 *
 * @param entry UI list entry
 * @param caption New caption
 *
 * @return EOK on success, ENOMEM if out of memory
 */
errno_t ui_list_entry_set_caption(ui_list_entry_t *entry, const char *caption)
{
	char *dcaption;

	dcaption = str_dup(caption);
	if (dcaption == NULL)
		return ENOMEM;

	free(entry->caption);
	entry->caption = dcaption;

	(void)ui_list_entry_paint(entry, ui_list_entry_get_idx(entry));
	return EOK;
}

/** Clear UI list entry list.
 *
 * @param list UI list
 */
void ui_list_clear_entries(ui_list_t *list)
{
	ui_list_entry_t *entry;

	entry = ui_list_first(list);
	while (entry != NULL) {
		ui_list_entry_destroy(entry);
		entry = ui_list_first(list);
	}
}

/** Get number of UI list entries.
 *
 * @param list UI list
 * @return Number of entries
 */
size_t ui_list_entries_cnt(ui_list_t *list)
{
	return list->entries_cnt;
}

/** Return first UI list entry.
 *
 * @param list UI list
 * @return First UI list entry or @c NULL if there are no entries
 */
ui_list_entry_t *ui_list_first(ui_list_t *list)
{
	link_t *link;

	link = list_first(&list->entries);
	if (link == NULL)
		return NULL;

	return list_get_instance(link, ui_list_entry_t, lentries);
}

/** Return last UI list entry.
 *
 * @param list UI list
 * @return Last UI list entry or @c NULL if there are no entries
 */
ui_list_entry_t *ui_list_last(ui_list_t *list)
{
	link_t *link;

	link = list_last(&list->entries);
	if (link == NULL)
		return NULL;

	return list_get_instance(link, ui_list_entry_t, lentries);
}

/** Return next UI list entry.
 *
 * @param cur Current entry
 * @return Next entry or @c NULL if @a cur is the last entry
 */
ui_list_entry_t *ui_list_next(ui_list_entry_t *cur)
{
	link_t *link;

	link = list_next(&cur->lentries, &cur->list->entries);
	if (link == NULL)
		return NULL;

	return list_get_instance(link, ui_list_entry_t, lentries);
}

/** Return previous UI list entry.
 *
 * @param cur Current entry
 * @return Previous entry or @c NULL if @a cur is the first entry
 */
ui_list_entry_t *ui_list_prev(ui_list_entry_t *cur)
{
	link_t *link;

	link = list_prev(&cur->lentries, &cur->list->entries);
	if (link == NULL)
		return NULL;

	return list_get_instance(link, ui_list_entry_t, lentries);
}

/** Find the n-th entry of the current UI list page.
 *
 * @param list UI list
 * @param n Which entry to get (starting from 0)
 * @param ridx Place to store index (within listing) of the entry
 * @return n-th entry of the page
 */
ui_list_entry_t *ui_list_page_nth_entry(ui_list_t *list,
    size_t n, size_t *ridx)
{
	ui_list_entry_t *entry;
	size_t i;
	size_t idx;

	assert(n <= ui_list_page_size(list));

	entry = list->page;
	if (entry == NULL)
		return NULL;

	idx = list->page_idx;
	for (i = 0; i < n; i++) {
		entry = ui_list_next(entry);
		if (entry == NULL)
			return NULL;

		++idx;
	}

	*ridx = idx;
	return entry;
}

/** Get entry under cursor.
 *
 * @param list UI list
 * @return Current cursor
 */
ui_list_entry_t *ui_list_get_cursor(ui_list_t *list)
{
	return list->cursor;
}

/** Set new cursor position.
 *
 * O(N) in list size, use with caution.
 *
 * @param list UI list
 * @param entry New cursor position
 */
void ui_list_set_cursor(ui_list_t *list, ui_list_entry_t *entry)
{
	size_t idx;

	idx = ui_list_entry_get_idx(entry);
	ui_list_cursor_move(list, entry, idx);
}

/** Move cursor to a new position, possibly scrolling.
 *
 * @param list UI list
 * @param entry New entry under cursor
 * @param entry_idx Index of new entry under cursor
 */
void ui_list_cursor_move(ui_list_t *list,
    ui_list_entry_t *entry, size_t entry_idx)
{
	gfx_context_t *gc = ui_window_get_gc(list->window);
	ui_list_entry_t *old_cursor;
	size_t old_idx;
	size_t rows;
	ui_list_entry_t *e;
	size_t i;

	rows = ui_list_page_size(list);

	old_cursor = list->cursor;
	old_idx = list->cursor_idx;

	list->cursor = entry;
	list->cursor_idx = entry_idx;

	if (entry_idx >= list->page_idx &&
	    entry_idx < list->page_idx + rows) {
		/*
		 * If cursor is still on the current page, we're not
		 * scrolling. Just unpaint old cursor and paint new
		 * cursor.
		 */
		ui_list_entry_paint(old_cursor, old_idx);
		ui_list_entry_paint(list->cursor, list->cursor_idx);

		(void) gfx_update(gc);
	} else {
		/*
		 * Need to scroll and update all rows.
		 */

		/* Scrolling up */
		if (entry_idx < list->page_idx) {
			list->page = entry;
			list->page_idx = entry_idx;
		}

		/* Scrolling down */
		if (entry_idx >= list->page_idx + rows) {
			if (entry_idx >= rows) {
				list->page_idx = entry_idx - rows + 1;
				/* Find first page entry (go back rows - 1) */
				e = entry;
				for (i = 0; i + 1 < rows; i++) {
					e = ui_list_prev(e);
				}

				/* Should be valid */
				assert(e != NULL);
				list->page = e;
			} else {
				list->page = ui_list_first(list);
				list->page_idx = 0;
			}
		}

		ui_list_scrollbar_update(list);
		(void) ui_list_paint(list);
	}
}

/** Move cursor one entry up.
 *
 * @param list UI list
 */
void ui_list_cursor_up(ui_list_t *list)
{
	ui_list_entry_t *prev;
	size_t prev_idx;

	prev = ui_list_prev(list->cursor);
	prev_idx = list->cursor_idx - 1;
	if (prev != NULL)
		ui_list_cursor_move(list, prev, prev_idx);
}

/** Move cursor one entry down.
 *
 * @param list UI list
 */
void ui_list_cursor_down(ui_list_t *list)
{
	ui_list_entry_t *next;
	size_t next_idx;

	next = ui_list_next(list->cursor);
	next_idx = list->cursor_idx + 1;
	if (next != NULL)
		ui_list_cursor_move(list, next, next_idx);
}

/** Move cursor to top.
 *
 * @param list UI list
 */
void ui_list_cursor_top(ui_list_t *list)
{
	ui_list_cursor_move(list, ui_list_first(list), 0);
}

/** Move cursor to bottom.
 *
 * @param list UI list
 */
void ui_list_cursor_bottom(ui_list_t *list)
{
	ui_list_cursor_move(list, ui_list_last(list),
	    list->entries_cnt - 1);
}

/** Move cursor one page up.
 *
 * @param list UI list
 */
void ui_list_page_up(ui_list_t *list)
{
	gfx_context_t *gc = ui_window_get_gc(list->window);
	ui_list_entry_t *old_page;
	ui_list_entry_t *old_cursor;
	size_t old_idx;
	size_t rows;
	ui_list_entry_t *entry;
	size_t i;

	rows = ui_list_page_size(list);

	old_page = list->page;
	old_cursor = list->cursor;
	old_idx = list->cursor_idx;

	/* Move page by rows entries up (if possible) */
	for (i = 0; i < rows; i++) {
		entry = ui_list_prev(list->page);
		if (entry != NULL) {
			list->page = entry;
			--list->page_idx;
		}
	}

	/* Move cursor by rows entries up (if possible) */

	for (i = 0; i < rows; i++) {
		entry = ui_list_prev(list->cursor);
		if (entry != NULL) {
			list->cursor = entry;
			--list->cursor_idx;
		}
	}

	if (list->page != old_page) {
		/* We have scrolled. Need to repaint all entries */
		ui_list_scrollbar_update(list);
		(void) ui_list_paint(list);
	} else if (list->cursor != old_cursor) {
		/* No scrolling, but cursor has moved */
		ui_list_entry_paint(old_cursor, old_idx);
		ui_list_entry_paint(list->cursor, list->cursor_idx);

		(void) gfx_update(gc);
	}
}

/** Move cursor one page down.
 *
 * @param list UI list
 */
void ui_list_page_down(ui_list_t *list)
{
	gfx_context_t *gc = ui_window_get_gc(list->window);
	ui_list_entry_t *old_page;
	ui_list_entry_t *old_cursor;
	size_t old_idx;
	size_t max_idx;
	size_t rows;
	ui_list_entry_t *entry;
	size_t i;

	rows = ui_list_page_size(list);

	old_page = list->page;
	old_cursor = list->cursor;
	old_idx = list->cursor_idx;

	if (list->entries_cnt > rows)
		max_idx = list->entries_cnt - rows;
	else
		max_idx = 0;

	/* Move page by rows entries down (if possible) */
	for (i = 0; i < rows; i++) {
		entry = ui_list_next(list->page);
		/* Do not scroll that results in a short page */
		if (entry != NULL && list->page_idx < max_idx) {
			list->page = entry;
			++list->page_idx;
		}
	}

	/* Move cursor by rows entries down (if possible) */

	for (i = 0; i < rows; i++) {
		entry = ui_list_next(list->cursor);
		if (entry != NULL) {
			list->cursor = entry;
			++list->cursor_idx;
		}
	}

	if (list->page != old_page) {
		/* We have scrolled. Need to repaint all entries */
		ui_list_scrollbar_update(list);
		(void) ui_list_paint(list);
	} else if (list->cursor != old_cursor) {
		/* No scrolling, but cursor has moved */
		ui_list_entry_paint(old_cursor, old_idx);
		ui_list_entry_paint(list->cursor, list->cursor_idx);

		(void) gfx_update(gc);
	}
}

/** Scroll one entry up.
 *
 * @param list UI list
 */
void ui_list_scroll_up(ui_list_t *list)
{
	ui_list_entry_t *prev;

	if (list->page == NULL)
		return;

	prev = ui_list_prev(list->page);
	if (prev == NULL)
		return;

	list->page = prev;
	assert(list->page_idx > 0);
	--list->page_idx;

	ui_list_scrollbar_update(list);
	(void) ui_list_paint(list);
}

/** Scroll one entry down.
 *
 * @param list UI list
 */
void ui_list_scroll_down(ui_list_t *list)
{
	ui_list_entry_t *next;
	ui_list_entry_t *pgend;
	size_t i;
	size_t rows;

	if (list->page == NULL)
		return;

	next = ui_list_next(list->page);
	if (next == NULL)
		return;

	rows = ui_list_page_size(list);

	/* Find last page entry */
	pgend = list->page;
	for (i = 0; i < rows && pgend != NULL; i++) {
		pgend = ui_list_next(pgend);
	}

	/* Scroll down by one entry, if the page remains full */
	if (pgend != NULL) {
		list->page = next;
		++list->page_idx;
	}

	ui_list_scrollbar_update(list);
	(void) ui_list_paint(list);
}

/** Scroll one page up.
 *
 * @param list UI list
 */
void ui_list_scroll_page_up(ui_list_t *list)
{
	ui_list_entry_t *prev;
	size_t i;
	size_t rows;

	prev = ui_list_prev(list->page);
	if (prev == NULL)
		return;

	rows = ui_list_page_size(list);

	for (i = 0; i < rows && prev != NULL; i++) {
		list->page = prev;
		assert(list->page_idx > 0);
		--list->page_idx;
		prev = ui_list_prev(prev);
	}

	ui_list_scrollbar_update(list);
	(void) ui_list_paint(list);
}

/** Scroll one page down.
 *
 * @param list UI list
 */
void ui_list_scroll_page_down(ui_list_t *list)
{
	ui_list_entry_t *next;
	ui_list_entry_t *pgend;
	size_t i;
	size_t rows;

	next = ui_list_next(list->page);
	if (next == NULL)
		return;

	rows = ui_list_page_size(list);

	/* Find last page entry */
	pgend = list->page;
	for (i = 0; i < rows && pgend != NULL; i++) {
		pgend = ui_list_next(pgend);
	}

	/* Scroll by up to 'rows' entries, keeping the page full */
	for (i = 0; i < rows && pgend != NULL; i++) {
		list->page = next;
		++list->page_idx;
		next = ui_list_next(next);
		pgend = ui_list_next(pgend);
	}

	ui_list_scrollbar_update(list);
	(void) ui_list_paint(list);
}

/** Scroll to a specific entry
 *
 * @param list UI list
 * @param page_idx New index of first entry on the page
 */
void ui_list_scroll_pos(ui_list_t *list, size_t page_idx)
{
	ui_list_entry_t *entry;
	size_t i;

	entry = ui_list_first(list);
	for (i = 0; i < page_idx; i++) {
		entry = ui_list_next(entry);
		assert(entry != NULL);
	}

	list->page = entry;
	list->page_idx = page_idx;

	(void) ui_list_paint(list);
}

/** Request UI list activation.
 *
 * Call back to request UI list activation.
 *
 * @param list UI list
 */
void ui_list_activate_req(ui_list_t *list)
{
	if (list->cb != NULL && list->cb->activate_req != NULL) {
		list->cb->activate_req(list, list->cb_arg);
	} else {
		/*
		 * If there is no callback for activation request,
		 * just activate the list.
		 */
		ui_list_activate(list);
	}
}

/** Sort list entries.
 *
 * @param list UI list
 * @return EOK on success, ENOMEM if out of memory
 */
errno_t ui_list_sort(ui_list_t *list)
{
	ui_list_entry_t **emap;
	ui_list_entry_t *entry;
	size_t i;

	/* Create an array to hold pointer to each entry */
	emap = calloc(list->entries_cnt, sizeof(ui_list_entry_t *));
	if (emap == NULL)
		return ENOMEM;

	/* Write entry pointers to array */
	entry = ui_list_first(list);
	i = 0;
	while (entry != NULL) {
		assert(i < list->entries_cnt);
		emap[i++] = entry;
		entry = ui_list_next(entry);
	}

	/* Sort the array of pointers */
	qsort(emap, list->entries_cnt, sizeof(ui_list_entry_t *),
	    ui_list_entry_ptr_cmp);

	/* Unlink entries from entry list */
	entry = ui_list_first(list);
	while (entry != NULL) {
		list_remove(&entry->lentries);
		entry = ui_list_first(list);
	}

	/* Add entries back to entry list sorted */
	for (i = 0; i < list->entries_cnt; i++)
		list_append(&emap[i]->lentries, &list->entries);

	free(emap);

	list->page = ui_list_first(list);
	list->page_idx = 0;
	list->cursor = ui_list_first(list);
	list->cursor_idx = 0;
	return EOK;
}

/** Determine list entry index.
 *
 * @param entry List entry
 * @return List entry index
 */
size_t ui_list_entry_get_idx(ui_list_entry_t *entry)
{
	ui_list_entry_t *ep;
	size_t idx;

	idx = 0;
	ep = ui_list_prev(entry);
	while (ep != NULL) {
		++idx;
		ep = ui_list_prev(ep);
	}

	return idx;
}

/** Center list cursor on entry.
 *
 * @param list UI list
 * @param entry Entry
 */
void ui_list_cursor_center(ui_list_t *list, ui_list_entry_t *entry)
{
	ui_list_entry_t *prev;
	size_t idx;
	size_t max_idx;
	size_t pg_size;
	size_t i;

	idx = ui_list_entry_get_idx(entry);
	list->cursor = entry;
	list->cursor_idx = idx;

	/* Move page so that cursor is in the center */
	list->page = list->cursor;
	list->page_idx = list->cursor_idx;

	pg_size = ui_list_page_size(list);

	for (i = 0; i < pg_size / 2; i++) {
		prev = ui_list_prev(list->page);
		if (prev == NULL)
			break;

		list->page = prev;
		--list->page_idx;
	}

	/* Make sure page is not beyond the end if possible */
	if (list->entries_cnt > pg_size)
		max_idx = list->entries_cnt - pg_size;
	else
		max_idx = 0;

	while (list->page_idx > 0 && list->page_idx > max_idx) {
		prev = ui_list_prev(list->page);
		if (prev == NULL)
			break;

		list->page = prev;
		--list->page_idx;
	}
}

/** Call back when an entry is selected.
 *
 * @param entry UI list entry
 * @param fname File name
 */
void ui_list_selected(ui_list_entry_t *entry)
{
	if (entry->list->cb != NULL && entry->list->cb->selected != NULL)
		entry->list->cb->selected(entry, entry->arg);
}

/** UI list scrollbar up button pressed.
 *
 * @param scrollbar Scrollbar
 * @param arg Argument (ui_list_t *)
 */
static void ui_list_scrollbar_up(ui_scrollbar_t *scrollbar, void *arg)
{
	ui_list_t *list = (ui_list_t *)arg;
	ui_list_scroll_up(list);
}

/** UI list scrollbar down button pressed.
 *
 * @param scrollbar Scrollbar
 * @param arg Argument (ui_list_t *)
 */
static void ui_list_scrollbar_down(ui_scrollbar_t *scrollbar, void *arg)
{
	ui_list_t *list = (ui_list_t *)arg;
	ui_list_scroll_down(list);
}

/** UI list scrollbar page up pressed.
 *
 * @param scrollbar Scrollbar
 * @param arg Argument (ui_list_t *)
 */
static void ui_list_scrollbar_page_up(ui_scrollbar_t *scrollbar, void *arg)
{
	ui_list_t *list = (ui_list_t *)arg;
	ui_list_scroll_page_up(list);
}

/** UI list scrollbar page down pressed.
 *
 * @param scrollbar Scrollbar
 * @param arg Argument (ui_list_t *)
 */
static void ui_list_scrollbar_page_down(ui_scrollbar_t *scrollbar,
    void *arg)
{
	ui_list_t *list = (ui_list_t *)arg;
	ui_list_scroll_page_down(list);
}

/** UI list scrollbar moved.
 *
 * @param scrollbar Scrollbar
 * @param arg Argument (ui_list_t *)
 */
static void ui_list_scrollbar_moved(ui_scrollbar_t *scrollbar, void *arg,
    gfx_coord_t pos)
{
	ui_list_t *list = (ui_list_t *)arg;
	size_t entries;
	size_t pglen;
	size_t sbar_len;
	size_t pgstart;

	entries = list_count(&list->entries);
	pglen = ui_list_page_size(list);
	sbar_len = ui_scrollbar_move_length(list->scrollbar);

	if (entries > pglen)
		pgstart = (entries - pglen) * pos / (sbar_len - 1);
	else
		pgstart = 0;

	ui_list_scroll_pos(list, pgstart);
}

/** Compare two list entries indirectly referenced by pointers.
 *
 * @param pa Pointer to pointer to first entry
 * @param pb Pointer to pointer to second entry
 * @return <0, =0, >=0 if pa < b, pa == pb, pa > pb, resp.
 */
int ui_list_entry_ptr_cmp(const void *pa, const void *pb)
{
	ui_list_entry_t *a = *(ui_list_entry_t **)pa;
	ui_list_entry_t *b = *(ui_list_entry_t **)pb;

	return a->list->cb->compare(a, b);
}

/** @}
 */
