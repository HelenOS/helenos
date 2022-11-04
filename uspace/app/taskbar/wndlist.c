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

/** @addtogroup taskbar
 * @{
 */
/** @file Task bar window list
 */

#include <gfx/coord.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <str.h>
#include <ui/fixed.h>
#include <ui/label.h>
#include <ui/resource.h>
#include <ui/ui.h>
#include <ui/window.h>
#include "clock.h"
#include "wndlist.h"

static void wndlist_wm_window_added(void *, sysarg_t);
static void wndlist_wm_window_removed(void *, sysarg_t);
static void wndlist_wm_window_changed(void *, sysarg_t);

static wndmgt_cb_t wndlist_wndmgt_cb = {
	.window_added = wndlist_wm_window_added,
	.window_removed = wndlist_wm_window_removed,
	.window_changed = wndlist_wm_window_changed
};

/** Create task bar window list.
 *
 * @param window Containing window
 * @param fixed Fixed layout to which buttons will be added
 * @param wndmgt Window management service
 * @param rwndlist Place to store pointer to new window list
 * @return @c EOK on success or an error code
 */
errno_t wndlist_create(ui_window_t *window, ui_fixed_t *fixed,
    wndlist_t **rwndlist)
{
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
	*rwndlist = wndlist;
	return EOK;
error:
	return rc;
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

		rc = wndlist_append(wndlist, wlist->windows[i], winfo->caption,
		    false);
		if (rc != EOK) {
			wndmgt_free_window_info(winfo);
			goto error;
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

/** Destroy task bar window list. */
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
 * @param paint @c true to paint immediately
 * @return @c EOK on success or an error code
 */
errno_t wndlist_append(wndlist_t *wndlist, sysarg_t wnd_id,
    const char *caption, bool paint)
{
	wndlist_entry_t *entry = NULL;
	ui_resource_t *res;
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

	/* Set the button rectangle */
	wndlist_set_entry_rect(wndlist, entry);

	rc = ui_fixed_add(wndlist->fixed, ui_pbutton_ctl(entry->button));
	if (rc != EOK)
		goto error;

	if (paint) {
		rc = ui_pbutton_paint(entry->button);
		if (rc != EOK)
			goto error;
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
	wndlist_entry_t *next;
	assert(entry->wndlist == wndlist);

	next = wndlist_next(entry);

	ui_fixed_remove(wndlist->fixed, ui_pbutton_ctl(entry->button));
	ui_pbutton_destroy(entry->button);
	list_remove(&entry->lentries);
	free(entry);

	/* Update positions of the remaining entries */
	while (next != NULL) {
		wndlist_set_entry_rect(wndlist, next);
		next = wndlist_next(next);
	}

	if (!paint)
		return EOK;

	return wndlist_repaint(wndlist);
}

/** Update window list entry.
 *
 * @param wndlist Window list
 * @param entry Window list entry
 * @return @c EOK on success or an error code
 */
errno_t wndlist_update(wndlist_t *wndlist, wndlist_entry_t *entry,
    const char *caption)
{
	errno_t rc;
	assert(entry->wndlist == wndlist);

	rc = ui_pbutton_set_caption(entry->button, caption);
	if (rc != EOK)
		return rc;

	rc = ui_pbutton_paint(entry->button);
	if (rc != EOK)
		return rc;

	return wndlist_repaint(wndlist);
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
		rect.p0.x = 17 * idx + 9;
		rect.p0.y = 0;
		rect.p1.x = 17 * idx + 25;
		rect.p1.y = 1;
	} else {
		rect.p0.x = 145 * idx + 90;
		rect.p0.y = 3;
		rect.p1.x = 145 * idx + 230;
		rect.p1.y = 29;
	}

	ui_pbutton_set_rect(entry->button, &rect);
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
	errno_t rc;

	rc = wndmgt_get_window_info(wndlist->wndmgt, wnd_id, &winfo);
	if (rc != EOK)
		goto error;

	rc = wndlist_append(wndlist, wnd_id, winfo->caption, true);
	if (rc != EOK) {
		wndmgt_free_window_info(winfo);
		goto error;
	}

	wndmgt_free_window_info(winfo);
	return;
error:
	if (winfo != NULL)
		wndmgt_free_window_info(winfo);
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

	printf("wm_window_removed: wndlist=%p wnd_id=%zu\n",
	    (void *)wndlist, wnd_id);

	entry = wndlist_entry_by_id(wndlist, wnd_id);
	if (entry == NULL)
		return;

	(void) wndlist_remove(wndlist, entry, true);
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
	errno_t rc;

	printf("wm_window_changed: wndlist=%p wnd_id=%zu\n",
	    (void *)wndlist, wnd_id);

	entry = wndlist_entry_by_id(wndlist, wnd_id);
	if (entry == NULL)
		return;

	rc = wndmgt_get_window_info(wndlist->wndmgt, wnd_id, &winfo);
	if (rc != EOK)
		return;

	(void) wndlist_update(wndlist, entry, winfo->caption);
	wndmgt_free_window_info(winfo);
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

/** Repaint window list.
 *
 * @param wndlist Window list
 * @return @c EOK on success or an error code
 */
errno_t wndlist_repaint(wndlist_t *wndlist)
{
	return ui_window_paint(wndlist->window);
}

/** @}
 */
