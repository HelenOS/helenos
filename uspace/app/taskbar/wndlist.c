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

/** @addtogroup taskbar
 * @{
 */
/** @file Taskbar window list
 */

#include <gfx/coord.h>
#include <gfx/render.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <str.h>
#include <ui/fixed.h>
#include <ui/label.h>
#include <ui/resource.h>
#include <ui/ui.h>
#include <ui/window.h>
#include "wndlist.h"

static void wndlist_wm_window_added(void *, sysarg_t);
static void wndlist_wm_window_removed(void *, sysarg_t);
static void wndlist_wm_window_changed(void *, sysarg_t);

/** Window list WM callbacks */
static wndmgt_cb_t wndlist_wndmgt_cb = {
	.window_added = wndlist_wm_window_added,
	.window_removed = wndlist_wm_window_removed,
	.window_changed = wndlist_wm_window_changed
};

static void wndlist_button_clicked(ui_pbutton_t *, void *);

/** Window list button callbacks */
static ui_pbutton_cb_t wndlist_button_cb = {
	.clicked = wndlist_button_clicked
};

enum {
	/** Min. X distance between left edges of two consecutive buttons */
	wndlist_button_pitch_min = 85,
	/** Max. X distance between left edges of two consecutive buttons (text) */
	wndlist_button_pitch_min_text = 10,
	/** Min. X distance between left edges of two consecutive buttons */
	wndlist_button_pitch_max = 165,
	/** Max. X distance between left edges of two consecutive buttons (text) */
	wndlist_button_pitch_max_text = 17,
	/** Padding between buttons */
	wndlist_button_pad = 5,
	/** Padding between buttons (text) */
	wndlist_button_pad_text = 1
};

/** Create taskbar window list.
 *
 * @param window Containing window
 * @param fixed Fixed layout to which buttons will be added
 * @param rwndlist Place to store pointer to new window list
 * @return @c EOK on success or an error code
 */
errno_t wndlist_create(ui_window_t *window, ui_fixed_t *fixed,
    wndlist_t **rwndlist)
{
	ui_resource_t *res = ui_window_get_res(window);
	wndlist_t *wndlist = NULL;
	errno_t rc;

	wndlist = calloc(1, sizeof(wndlist_t));
	if (wndlist == NULL) {
		rc = ENOMEM;
		goto error;
	}

	wndlist->window = window;
	wndlist->fixed = fixed;
	list_initialize(&wndlist->entries);

	if (ui_resource_is_textmode(res))
		wndlist->pitch = wndlist_button_pitch_max_text;
	else
		wndlist->pitch = wndlist_button_pitch_max;

	*rwndlist = wndlist;
	return EOK;
error:
	return rc;
}

/** Set window list rectangle.
 *
 * @param wndlist Window list
 * @param rect Rectangle
 */
void wndlist_set_rect(wndlist_t *wndlist, gfx_rect_t *rect)
{
	wndlist->rect = *rect;
}

/** Attach window management service to window list.
 *
 * @param wndlist Window list
 * @param wndmgt_svc Window management service name
 * @return @c EOK on success or an error code
 */
errno_t wndlist_open_wm(wndlist_t *wndlist, const char *wndmgt_svc)
{
	errno_t rc;
	wndmgt_window_list_t *wlist = NULL;
	wndmgt_window_info_t *winfo = NULL;
	sysarg_t i;

	rc = wndmgt_open(wndmgt_svc, &wndlist_wndmgt_cb, (void *)wndlist,
	    &wndlist->wndmgt);
	if (rc != EOK)
		goto error;

	rc = wndmgt_get_window_list(wndlist->wndmgt, &wlist);
	if (rc != EOK)
		goto error;

	for (i = 0; i < wlist->nwindows; i++) {
		rc = wndmgt_get_window_info(wndlist->wndmgt, wlist->windows[i],
		    &winfo);
		if (rc != EOK)
			goto error;

		if ((winfo->flags & (wndf_popup | wndf_system)) == 0) {
			rc = wndlist_append(wndlist, wlist->windows[i],
			    winfo->caption, winfo->nfocus != 0, false);
			if (rc != EOK) {
				wndmgt_free_window_info(winfo);
				goto error;
			}
		}

		wndmgt_free_window_info(winfo);
	}

	return EOK;
error:
	if (wlist != NULL)
		wndmgt_free_window_list(wlist);
	if (wndlist->wndmgt != NULL) {
		wndmgt_close(wndlist->wndmgt);
		wndlist->wndmgt = NULL;
	}
	return rc;
}

/** Destroy taskbar window list.
 *
 * @param wndlist Window list
 */
void wndlist_destroy(wndlist_t *wndlist)
{
	wndlist_entry_t *entry;

	/* Close window management service */
	if (wndlist->wndmgt)
		wndmgt_close(wndlist->wndmgt);

	/* Destroy entries */
	entry = wndlist_first(wndlist);
	while (entry != NULL) {
		(void)wndlist_remove(wndlist, entry, false);
		entry = wndlist_first(wndlist);
	}

	free(wndlist);
}

/** Append new entry to window list.
 *
 * @param wndlist Window list
 * @param wnd_id Window ID
 * @param caption Entry caption
 * @param active @c true iff window is active
 * @param paint @c true to paint immediately
 * @return @c EOK on success or an error code
 */
errno_t wndlist_append(wndlist_t *wndlist, sysarg_t wnd_id,
    const char *caption, bool active, bool paint)
{
	wndlist_entry_t *entry = NULL;
	ui_resource_t *res;
	wndlist_entry_t *e;
	errno_t rc;

	entry = calloc(1, sizeof(wndlist_entry_t));
	if (entry == NULL) {
		rc = ENOMEM;
		goto error;
	}

	entry->wnd_id = wnd_id;
	res = ui_window_get_res(wndlist->window);

	rc = ui_pbutton_create(res, caption, &entry->button);
	if (rc != EOK)
		goto error;

	entry->wndlist = wndlist;
	list_append(&entry->lentries, &wndlist->entries);

	entry->visible = false;

	ui_pbutton_set_light(entry->button, active);

	/* Set button callbacks */
	ui_pbutton_set_cb(entry->button, &wndlist_button_cb, (void *)entry);

	if (wndlist_update_pitch(wndlist)) {
		/*
		 * Update rectangles for all entries, including @a entry, adding
		 * it to the layout, if applicable.
		 */
		e = wndlist_first(wndlist);
		while (e != NULL) {
			wndlist_set_entry_rect(wndlist, e);
			e = wndlist_next(e);
		}

		if (paint)
			return wndlist_repaint(wndlist);
	} else {
		wndlist_set_entry_rect(wndlist, entry);
		if (paint)
			return wndlist_paint_entry(entry);
	}

	return EOK;
error:
	if (entry != NULL && entry->button != NULL)
		ui_pbutton_destroy(entry->button);
	if (entry != NULL)
		free(entry);
	return rc;

}

/** Remove entry from window list.
 *
 * @param wndlist Window list
 * @param entry Window list entry
 * @param paint @c true to repaint window list
 * @return @c EOK on success or an error code
 */
errno_t wndlist_remove(wndlist_t *wndlist, wndlist_entry_t *entry,
    bool paint)
{
	wndlist_entry_t *e;
	wndlist_entry_t *next;
	wndlist_entry_t *last;
	errno_t rc = EOK;

	assert(entry->wndlist == wndlist);
	next = wndlist_next(entry);

	/* Remember last entry */
	last = wndlist_last(wndlist);

	if (entry->visible)
		ui_fixed_remove(wndlist->fixed, ui_pbutton_ctl(entry->button));

	list_remove(&entry->lentries);

	if (wndlist_update_pitch(wndlist)) {
		/*
		 * Update rectangles for all entries.
		 */
		e = wndlist_first(wndlist);
		while (e != NULL) {
			wndlist_set_entry_rect(wndlist, e);
			e = wndlist_next(e);
		}

		if (paint)
			rc = wndlist_repaint(wndlist);
	} else {
		/* Unpaint the last entry */
		if (paint)
			rc = wndlist_unpaint_entry(last);

		/*
		 * Update rectangles for entries to the right
		 */

		e = NULL;
		while (next != NULL) {
			e = next;

			wndlist_set_entry_rect(wndlist, e);
			if (paint) {
				rc = wndlist_paint_entry(e);
				if (rc != EOK)
					return rc;
			}

			next = wndlist_next(e);
		}
	}

	ui_pbutton_destroy(entry->button);
	free(entry);
	return rc;
}

/** Update button pitch.
 *
 * Recalculatebutton pitch @c wndlist->pitch based on current number
 * of buttons.
 *
 * @param wndlist Window list
 * @return @c true iff pitch changed
 */
bool wndlist_update_pitch(wndlist_t *wndlist)
{
	ui_resource_t *res;
	size_t nbuttons;
	gfx_coord_t pitch;
	gfx_coord_t pitch_max;
	gfx_coord_t pitch_min;
	gfx_coord_t pad;

	res = ui_window_get_res(wndlist->window);

	if (ui_resource_is_textmode(res)) {
		pitch_max = wndlist_button_pitch_max_text;
		pitch_min = wndlist_button_pitch_min_text;
		pad = wndlist_button_pad_text;
	} else {
		pitch_max = wndlist_button_pitch_max;
		pitch_min = wndlist_button_pitch_min;
		pad = wndlist_button_pad;
	}

	/* Compute pitch that fits all buttons perfectly */
	nbuttons = wndlist_count(wndlist);
	if (nbuttons > 0)
		pitch = (wndlist->rect.p1.x - wndlist->rect.p0.x + pad) / nbuttons;
	else
		pitch = pitch_min;

	if (pitch < pitch_min)
		pitch = pitch_min;
	if (pitch > pitch_max)
		pitch = pitch_max;

	/* Did the pitch change? */
	if (pitch == wndlist->pitch)
		return false;

	wndlist->pitch = pitch;
	return true;
}

/** Update window list entry.
 *
 * @param wndlist Window list
 * @param entry Window list entry
 * @param active @c true iff the window is active
 * @return @c EOK on success or an error code
 */
errno_t wndlist_update(wndlist_t *wndlist, wndlist_entry_t *entry,
    const char *caption, bool active)
{
	errno_t rc;
	assert(entry->wndlist == wndlist);

	rc = ui_pbutton_set_caption(entry->button, caption);
	if (rc != EOK)
		return rc;

	ui_pbutton_set_light(entry->button, active);

	rc = wndlist_paint_entry(entry);
	if (rc != EOK)
		return rc;

	return EOK;
}

/** Compute and set window list entry rectangle.
 *
 * Compute rectangle for window list entry and set it.
 *
 * @param wndlist Window list
 * @param entry Window list entry
 */
void wndlist_set_entry_rect(wndlist_t *wndlist, wndlist_entry_t *entry)
{
	wndlist_entry_t *e;
	gfx_rect_t rect;
	ui_resource_t *res;
	gfx_coord_t pitch;
	gfx_coord_t pad;
	size_t idx;

	/* Determine entry index */
	idx = 0;
	e = wndlist_first(wndlist);
	while (e != entry) {
		assert(e != NULL);
		e = wndlist_next(e);
		++idx;
	}

	res = ui_window_get_res(wndlist->window);

	if (ui_resource_is_textmode(res)) {
		pad = wndlist_button_pad_text;
	} else {
		pad = wndlist_button_pad;
	}

	pitch = wndlist->pitch;

	rect.p0.x = wndlist->rect.p0.x + pitch * idx;
	rect.p0.y = wndlist->rect.p0.y;
	rect.p1.x = wndlist->rect.p0.x + pitch * (idx + 1) - pad;
	rect.p1.y = wndlist->rect.p1.y;

	/* Entry does not fit? */
	if (rect.p1.x > wndlist->rect.p1.x) {
		/* Make entry invisible */
		if (entry->visible) {
			ui_fixed_remove(wndlist->fixed,
			    ui_pbutton_ctl(entry->button));
			entry->visible = false;
		}
	} else {
		/* Make entry visible */
		if (!entry->visible) {
			ui_fixed_add(wndlist->fixed,
			    ui_pbutton_ctl(entry->button));
			entry->visible = true;
		}
	}

	ui_pbutton_set_rect(entry->button, &rect);
	entry->rect = rect;
}

/** Paint window list entry.
 *
 * @param entry Window list entry
 * @return EOK on success or an error code
 */
errno_t wndlist_paint_entry(wndlist_entry_t *entry)
{
	ui_t *ui;

	ui = ui_window_get_ui(entry->wndlist->window);
	if (ui_is_suspended(ui))
		return EOK;

	return ui_pbutton_paint(entry->button);
}

/** Unpaint window list entry.
 *
 * @param entry Window list entry
 * @return EOK on success or an error code
 */
errno_t wndlist_unpaint_entry(wndlist_entry_t *entry)
{
	errno_t rc;
	ui_t *ui;
	gfx_context_t *gc;
	ui_resource_t *res;
	gfx_color_t *color;

	ui = ui_window_get_ui(entry->wndlist->window);
	gc = ui_window_get_gc(entry->wndlist->window);
	res = ui_window_get_res(entry->wndlist->window);
	color = ui_resource_get_wnd_face_color(res);

	if (ui_is_suspended(ui))
		return EOK;

	rc = gfx_set_color(gc, color);
	if (rc != EOK)
		return rc;

	rc = gfx_fill_rect(gc, &entry->rect);
	if (rc != EOK)
		return rc;

	rc = gfx_update(gc);
	if (rc != EOK)
		return rc;

	return EOK;
}

/** Handle WM window added event.
 *
 * @param arg Argument (wndlist_t *)
 * @param wnd_id Window ID
 */
static void wndlist_wm_window_added(void *arg, sysarg_t wnd_id)
{
	wndlist_t *wndlist = (wndlist_t *)arg;
	wndmgt_window_info_t *winfo = NULL;
	ui_t *ui;
	errno_t rc;

	ui = ui_window_get_ui(wndlist->window);
	ui_lock(ui);

	rc = wndmgt_get_window_info(wndlist->wndmgt, wnd_id, &winfo);
	if (rc != EOK)
		goto error;

	if ((winfo->flags & (wndf_popup | wndf_system)) == 0) {
		rc = wndlist_append(wndlist, wnd_id, winfo->caption,
		    winfo->nfocus != 0, true);
		if (rc != EOK) {
			wndmgt_free_window_info(winfo);
			goto error;
		}
	}

	wndmgt_free_window_info(winfo);
	ui_unlock(ui);
	return;
error:
	if (winfo != NULL)
		wndmgt_free_window_info(winfo);
	ui_unlock(ui);
}

/** Handle WM window removed event.
 *
 * @param arg Argument (wndlist_t *)
 * @param wnd_id Window ID
 */
static void wndlist_wm_window_removed(void *arg, sysarg_t wnd_id)
{
	wndlist_t *wndlist = (wndlist_t *)arg;
	wndlist_entry_t *entry;
	ui_t *ui;

	ui = ui_window_get_ui(wndlist->window);
	ui_lock(ui);

	entry = wndlist_entry_by_id(wndlist, wnd_id);
	if (entry == NULL) {
		ui_unlock(ui);
		return;
	}

	(void) wndlist_remove(wndlist, entry, true);
	ui_unlock(ui);
}

/** Handle WM window changed event.
 *
 * @param arg Argument (wndlist_t *)
 * @param wnd_id Window ID
 */
static void wndlist_wm_window_changed(void *arg, sysarg_t wnd_id)
{
	wndlist_t *wndlist = (wndlist_t *)arg;
	wndmgt_window_info_t *winfo = NULL;
	wndlist_entry_t *entry;
	ui_t *ui;
	errno_t rc;

	ui = ui_window_get_ui(wndlist->window);
	ui_lock(ui);

	entry = wndlist_entry_by_id(wndlist, wnd_id);
	if (entry == NULL) {
		ui_unlock(ui);
		return;
	}

	rc = wndmgt_get_window_info(wndlist->wndmgt, wnd_id, &winfo);
	if (rc != EOK) {
		ui_unlock(ui);
		return;
	}

	(void) wndlist_update(wndlist, entry, winfo->caption,
	    winfo->nfocus != 0);
	wndmgt_free_window_info(winfo);
	ui_unlock(ui);
}

/** Find window list entry by ID.
 *
 * @param wndlist Window list
 * @param wnd_id Window ID
 * @return Window list entry on success or @c NULL if not found
 */
wndlist_entry_t *wndlist_entry_by_id(wndlist_t *wndlist, sysarg_t wnd_id)
{
	wndlist_entry_t *entry;

	entry = wndlist_first(wndlist);
	while (entry != NULL) {
		if (entry->wnd_id == wnd_id)
			return entry;

		entry = wndlist_next(entry);
	}

	return NULL;
}

/** Get first window list entry.
 *
 * @param wndlist Window list
 * @return First entry or @c NULL if the list is empty
 */
wndlist_entry_t *wndlist_first(wndlist_t *wndlist)
{
	link_t *link;

	link = list_first(&wndlist->entries);
	if (link == NULL)
		return NULL;

	return list_get_instance(link, wndlist_entry_t, lentries);
}

/** Get last window list entry.
 *
 * @param wndlist Window list
 * @return Last entry or @c NULL if the list is empty
 */
wndlist_entry_t *wndlist_last(wndlist_t *wndlist)
{
	link_t *link;

	link = list_last(&wndlist->entries);
	if (link == NULL)
		return NULL;

	return list_get_instance(link, wndlist_entry_t, lentries);
}

/** Get next window list entry.
 *
 * @param cur Current entry
 * @return Next entry or @c NULL if @a cur is the last entry
 */
wndlist_entry_t *wndlist_next(wndlist_entry_t *cur)
{
	link_t *link;

	link = list_next(&cur->lentries, &cur->wndlist->entries);
	if (link == NULL)
		return NULL;

	return list_get_instance(link, wndlist_entry_t, lentries);
}

/** Get number of window list entries.
 *
 * @param wndlist Window list
 * @return Number of entries
 */
size_t wndlist_count(wndlist_t *wndlist)
{
	return list_count(&wndlist->entries);
}

/** Repaint window list.
 *
 * @param wndlist Window list
 * @return @c EOK on success or an error code
 */
errno_t wndlist_repaint(wndlist_t *wndlist)
{
	if (ui_is_suspended(ui_window_get_ui(wndlist->window)))
		return EOK;

	return ui_window_paint(wndlist->window);
}

/** Window button was clicked.
 *
 * @param pbutton Push button
 * @param arg Argument (wndlist_entry_t *)
 */
static void wndlist_button_clicked(ui_pbutton_t *pbutton, void *arg)
{
	wndlist_entry_t *entry = (wndlist_entry_t *)arg;
	sysarg_t dev_id;

	/* ID of device that clicked the button */
	dev_id = entry->wndlist->ev_idev_id;

	(void) wndmgt_activate_window(entry->wndlist->wndmgt,
	    dev_id, entry->wnd_id);
}

/** @}
 */
