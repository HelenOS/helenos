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

/** @addtogroup display
 * @{
 */
/**
 * @file Display server display
 */

#include <errno.h>
#include <gfx/bitmap.h>
#include <gfx/context.h>
#include <gfx/render.h>
#include <io/log.h>
#include <memgfx/memgc.h>
#include <stdlib.h>
#include <str.h>
#include "client.h"
#include "clonegc.h"
#include "cursimg.h"
#include "cursor.h"
#include "display.h"
#include "idevcfg.h"
#include "seat.h"
#include "window.h"
#include "wmclient.h"

static gfx_context_t *ds_display_get_unbuf_gc(ds_display_t *);
static void ds_display_invalidate_cb(void *, gfx_rect_t *);
static void ds_display_update_cb(void *);

static mem_gc_cb_t ds_display_mem_gc_cb = {
	.invalidate = ds_display_invalidate_cb,
	.update = ds_display_update_cb
};

/** Create display.
 *
 * @param gc Graphics context for displaying output
 * @param flags Display flags
 * @param rdisp Place to store pointer to new display.
 * @return EOK on success, ENOMEM if out of memory
 */
errno_t ds_display_create(gfx_context_t *gc, ds_display_flags_t flags,
    ds_display_t **rdisp)
{
	ds_display_t *disp;
	ds_cursor_t *cursor;
	int i;
	errno_t rc;

	disp = calloc(1, sizeof(ds_display_t));
	if (disp == NULL)
		return ENOMEM;

	rc = gfx_color_new_rgb_i16(0x8000, 0xc800, 0xffff, &disp->bg_color);
	if (rc != EOK) {
		free(disp);
		return ENOMEM;
	}

	list_initialize(&disp->cursors);

	for (i = 0; i < dcurs_limit; i++) {
		rc = ds_cursor_create(disp, &ds_cursimg[i].rect,
		    ds_cursimg[i].image, &cursor);
		if (rc != EOK)
			goto error;

		disp->cursor[i] = cursor;
	}

	fibril_mutex_initialize(&disp->lock);
	list_initialize(&disp->clients);
	list_initialize(&disp->wmclients);
	list_initialize(&disp->cfgclients);
	disp->next_wnd_id = 1;
	disp->next_seat_id = 1;
	list_initialize(&disp->ddevs);
	list_initialize(&disp->idevcfgs);
	list_initialize(&disp->ievents);
	fibril_condvar_initialize(&disp->ievent_cv);
	list_initialize(&disp->seats);
	list_initialize(&disp->windows);
	disp->flags = flags;
	*rdisp = disp;
	return EOK;
error:
	ds_display_destroy(disp);
	return rc;
}

/** Destroy display.
 *
 * @param disp Display
 */
void ds_display_destroy(ds_display_t *disp)
{
	int i;

	assert(list_empty(&disp->clients));
	assert(list_empty(&disp->wmclients));
	assert(list_empty(&disp->cfgclients));
	assert(list_empty(&disp->seats));
	assert(list_empty(&disp->ddevs));
	assert(list_empty(&disp->idevcfgs));
	assert(list_empty(&disp->ievents));
	assert(list_empty(&disp->seats));
	assert(list_empty(&disp->windows));

	/* Destroy cursors */
	for (i = 0; i < dcurs_limit; i++) {
		ds_cursor_destroy(disp->cursor[i]);
		disp->cursor[i] = NULL;
	}

	gfx_color_delete(disp->bg_color);
	free(disp);
}

/** Load display configuration from SIF file.
 *
 * @param display Display
 * @param cfgpath Configuration file path
 *
 * @return EOK on success or an error code
 */
errno_t ds_display_load_cfg(ds_display_t *display, const char *cfgpath)
{
	sif_doc_t *doc = NULL;
	sif_node_t *rnode;
	sif_node_t *ndisplay;
	sif_node_t *nseats;
	sif_node_t *nseat;
	ds_seat_t *seat;
	sif_node_t *nidevcfgs;
	sif_node_t *nidevcfg;
	const char *ntype;
	ds_idevcfg_t *idevcfg;
	errno_t rc;

	rc = sif_load(cfgpath, &doc);
	if (rc != EOK)
		goto error;

	rnode = sif_get_root(doc);
	ndisplay = sif_node_first_child(rnode);
	ntype = sif_node_get_type(ndisplay);
	if (str_cmp(ntype, "display") != 0) {
		rc = EIO;
		goto error;
	}

	nseats = sif_node_first_child(ndisplay);
	ntype = sif_node_get_type(nseats);
	if (str_cmp(ntype, "seats") != 0) {
		rc = EIO;
		goto error;
	}

	/* Load individual seats */
	nseat = sif_node_first_child(nseats);
	while (nseat != NULL) {
		ntype = sif_node_get_type(nseat);
		if (str_cmp(ntype, "seat") != 0) {
			rc = EIO;
			goto error;
		}

		rc = ds_seat_load(display, nseat, &seat);
		if (rc != EOK)
			goto error;

		(void)seat;
		nseat = sif_node_next_child(nseat);
	}

	nidevcfgs = sif_node_next_child(nseats);
	ntype = sif_node_get_type(nidevcfgs);
	if (str_cmp(ntype, "idevcfgs") != 0) {
		rc = EIO;
		goto error;
	}

	/* Load individual input device configuration entries */
	nidevcfg = sif_node_first_child(nidevcfgs);
	while (nidevcfg != NULL) {
		ntype = sif_node_get_type(nidevcfg);
		if (str_cmp(ntype, "idevcfg") != 0) {
			rc = EIO;
			goto error;
		}

		/*
		 * Load device configuration entry. If the device
		 * is not currently connected (ENOENT), skip it.
		 */
		rc = ds_idevcfg_load(display, nidevcfg, &idevcfg);
		if (rc != EOK && rc != ENOENT)
			goto error;

		(void)idevcfg;
		nidevcfg = sif_node_next_child(nidevcfg);
	}

	sif_delete(doc);
	return EOK;
error:
	if (doc != NULL)
		sif_delete(doc);

	seat = ds_display_first_seat(display);
	while (seat != NULL) {
		ds_seat_destroy(seat);
		seat = ds_display_first_seat(display);
	}
	return rc;
}

/** Save display configuration to SIF file.
 *
 * @param display Display
 * @param cfgpath Configuration file path
 *
 * @return EOK on success or an error code
 */
errno_t ds_display_save_cfg(ds_display_t *display, const char *cfgpath)
{
	sif_doc_t *doc = NULL;
	sif_node_t *rnode;
	sif_node_t *ndisplay;
	sif_node_t *nseats;
	sif_node_t *nseat;
	ds_seat_t *seat;
	sif_node_t *nidevcfgs;
	sif_node_t *nidevcfg;
	ds_idevcfg_t *idevcfg;
	errno_t rc;

	rc = sif_new(&doc);
	if (rc != EOK)
		goto error;

	rnode = sif_get_root(doc);
	rc = sif_node_append_child(rnode, "display", &ndisplay);
	if (rc != EOK)
		goto error;

	rc = sif_node_append_child(ndisplay, "seats", &nseats);
	if (rc != EOK)
		goto error;

	/* Save individual seats */
	seat = ds_display_first_seat(display);
	while (seat != NULL) {
		rc = sif_node_append_child(nseats, "seat", &nseat);
		if (rc != EOK)
			goto error;

		rc = ds_seat_save(seat, nseat);
		if (rc != EOK)
			goto error;

		seat = ds_display_next_seat(seat);
	}

	rc = sif_node_append_child(ndisplay, "idevcfgs", &nidevcfgs);
	if (rc != EOK)
		goto error;

	/* Save individual input device configuration entries */
	idevcfg = ds_display_first_idevcfg(display);
	while (idevcfg != NULL) {
		rc = sif_node_append_child(nidevcfgs, "idevcfg", &nidevcfg);
		if (rc != EOK)
			goto error;

		rc = ds_idevcfg_save(idevcfg, nidevcfg);
		if (rc != EOK)
			goto error;

		idevcfg = ds_display_next_idevcfg(idevcfg);
	}

	rc = sif_save(doc, cfgpath);
	if (rc != EOK)
		goto error;

	sif_delete(doc);
	return EOK;
error:
	if (doc != NULL)
		sif_delete(doc);
	return rc;
}

/** Lock display.
 *
 * This should be called in any thread that wishes to access the display
 * or its child objects (e.g. windows).
 *
 * @param disp Display
 */
void ds_display_lock(ds_display_t *disp)
{
	fibril_mutex_lock(&disp->lock);
}

/** Unlock display.
 *
 * @param disp Display
 */
void ds_display_unlock(ds_display_t *disp)
{
	fibril_mutex_unlock(&disp->lock);
}

/** Get display information.
 *
 * @param disp Display
 */
void ds_display_get_info(ds_display_t *disp, display_info_t *info)
{
	info->rect = disp->rect;
}

/** Add client to display.
 *
 * @param disp Display
 * @param client Client
 */
void ds_display_add_client(ds_display_t *disp, ds_client_t *client)
{
	assert(client->display == NULL);
	assert(!link_used(&client->lclients));

	client->display = disp;
	list_append(&client->lclients, &disp->clients);
}

/** Remove client from display.
 *
 * @param client Client
 */
void ds_display_remove_client(ds_client_t *client)
{
	list_remove(&client->lclients);
	client->display = NULL;
}

/** Get first client in display.
 *
 * @param disp Display
 * @return First client or @c NULL if there is none
 */
ds_client_t *ds_display_first_client(ds_display_t *disp)
{
	link_t *link = list_first(&disp->clients);

	if (link == NULL)
		return NULL;

	return list_get_instance(link, ds_client_t, lclients);
}

/** Get next client in display.
 *
 * @param client Current client
 * @return Next client or @c NULL if there is none
 */
ds_client_t *ds_display_next_client(ds_client_t *client)
{
	link_t *link = list_next(&client->lclients, &client->display->clients);

	if (link == NULL)
		return NULL;

	return list_get_instance(link, ds_client_t, lclients);
}

/** Add WM client to display.
 *
 * @param disp Display
 * @param wmclient WM client
 */
void ds_display_add_wmclient(ds_display_t *disp, ds_wmclient_t *wmclient)
{
	assert(wmclient->display == NULL);
	assert(!link_used(&wmclient->lwmclients));

	wmclient->display = disp;
	list_append(&wmclient->lwmclients, &disp->wmclients);
}

/** Remove WM client from display.
 *
 * @param wmclient WM client
 */
void ds_display_remove_wmclient(ds_wmclient_t *wmclient)
{
	list_remove(&wmclient->lwmclients);
	wmclient->display = NULL;
}

/** Add CFG client to display.
 *
 * @param disp Display
 * @param cfgclient CFG client
 */
void ds_display_add_cfgclient(ds_display_t *disp, ds_cfgclient_t *cfgclient)
{
	assert(cfgclient->display == NULL);
	assert(!link_used(&cfgclient->lcfgclients));

	cfgclient->display = disp;
	list_append(&cfgclient->lcfgclients, &disp->cfgclients);
}

/** Remove CFG client from display.
 *
 * @param cfgclient CFG client
 */
void ds_display_remove_cfgclient(ds_cfgclient_t *cfgclient)
{
	list_remove(&cfgclient->lcfgclients);
	cfgclient->display = NULL;
}

/** Get first WM client in display.
 *
 * @param disp Display
 * @return First WM client or @c NULL if there is none
 */
ds_wmclient_t *ds_display_first_wmclient(ds_display_t *disp)
{
	link_t *link = list_first(&disp->wmclients);

	if (link == NULL)
		return NULL;

	return list_get_instance(link, ds_wmclient_t, lwmclients);
}

/** Get next WM client in display.
 *
 * @param wmclient Current WM client
 * @return Next WM client or @c NULL if there is none
 */
ds_wmclient_t *ds_display_next_wmclient(ds_wmclient_t *wmclient)
{
	link_t *link = list_next(&wmclient->lwmclients,
	    &wmclient->display->wmclients);

	if (link == NULL)
		return NULL;

	return list_get_instance(link, ds_wmclient_t, lwmclients);
}

/** Find window in all clients by ID.
 *
 * XXX This is just a hack needed to match GC connection to a window,
 * as we don't have a good safe way to pass the GC endpoint to our client
 * on demand.
 *
 * @param display Display
 * @param id Window ID
 */
ds_window_t *ds_display_find_window(ds_display_t *display, ds_wnd_id_t id)
{
	ds_client_t *client;
	ds_window_t *wnd;

	client = ds_display_first_client(display);
	while (client != NULL) {
		wnd = ds_client_find_window(client, id);
		if (wnd != NULL)
			return wnd;

		client = ds_display_next_client(client);
	}

	return NULL;
}

/** Find window by display position.
 *
 * @param display Display
 * @param pos Display position
 */
ds_window_t *ds_display_window_by_pos(ds_display_t *display, gfx_coord2_t *pos)
{
	ds_window_t *wnd;
	gfx_rect_t drect;

	wnd = ds_display_first_window(display);
	while (wnd != NULL) {
		/* Window bounding rectangle on display */
		gfx_rect_translate(&wnd->dpos, &wnd->rect, &drect);

		if (gfx_pix_inside_rect(pos, &drect) &&
		    ds_window_is_visible(wnd))
			return wnd;

		wnd = ds_display_next_window(wnd);
	}

	return NULL;
}

/** Add window to window list.
 *
 * Topmost windows are enlisted before any other window. Non-topmost
 * windows are enlisted before any other non-topmost window.
 *
 * @param display Display
 * @param wnd Window
 */
void ds_display_enlist_window(ds_display_t *display, ds_window_t *wnd)
{
	ds_window_t *w;

	assert(wnd->display == display);
	assert(!link_used(&wnd->ldwindows));

	if ((wnd->flags & wndf_topmost) == 0) {
		/* Find the first non-topmost window */
		w = ds_display_first_window(display);
		while (w != NULL && (w->flags & wndf_topmost) != 0)
			w = ds_display_next_window(w);

		if (w != NULL)
			list_insert_before(&wnd->ldwindows, &w->ldwindows);
		else
			list_append(&wnd->ldwindows, &display->windows);
	} else {
		/* Insert at the beginning */
		list_prepend(&wnd->ldwindows, &display->windows);
	}

}

/** Add window to display.
 *
 * @param display Display
 * @param wnd Window
 */
void ds_display_add_window(ds_display_t *display, ds_window_t *wnd)
{
	ds_wmclient_t *wmclient;

	assert(wnd->display == NULL);
	assert(!link_used(&wnd->ldwindows));

	wnd->display = display;
	ds_display_enlist_window(display, wnd);

	/* Notify window managers about the new window */
	wmclient = ds_display_first_wmclient(display);
	while (wmclient != NULL) {
		ds_wmclient_post_wnd_added_event(wmclient, wnd->id);
		wmclient = ds_display_next_wmclient(wmclient);
	}
}

/** Remove window from display.
 *
 * @param wnd Window
 */
void ds_display_remove_window(ds_window_t *wnd)
{
	ds_wmclient_t *wmclient;
	ds_display_t *display;

	display = wnd->display;

	list_remove(&wnd->ldwindows);
	wnd->display = NULL;

	/* Notify window managers about the removed window */
	wmclient = ds_display_first_wmclient(display);
	while (wmclient != NULL) {
		ds_wmclient_post_wnd_removed_event(wmclient, wnd->id);
		wmclient = ds_display_next_wmclient(wmclient);
	}
}

/** Move window to top.
 *
 * @param display Display
 * @param wnd Window
 */
void ds_display_window_to_top(ds_window_t *wnd)
{
	assert(wnd->display != NULL);
	assert(link_used(&wnd->ldwindows));

	list_remove(&wnd->ldwindows);
	ds_display_enlist_window(wnd->display, wnd);
}

/** Get first window in display.
 *
 * @param display Display
 * @return First window or @c NULL if there is none
 */
ds_window_t *ds_display_first_window(ds_display_t *display)
{
	link_t *link = list_first(&display->windows);

	if (link == NULL)
		return NULL;

	return list_get_instance(link, ds_window_t, ldwindows);
}

/** Get last window in display.
 *
 * @param display Display
 * @return Last window or @c NULL if there is none
 */
ds_window_t *ds_display_last_window(ds_display_t *display)
{
	link_t *link = list_last(&display->windows);

	if (link == NULL)
		return NULL;

	return list_get_instance(link, ds_window_t, ldwindows);
}

/** Get next window in client.
 *
 * @param wnd Current window
 * @return Next window or @c NULL if there is none
 */
ds_window_t *ds_display_next_window(ds_window_t *wnd)
{
	link_t *link = list_next(&wnd->ldwindows, &wnd->display->windows);

	if (link == NULL)
		return NULL;

	return list_get_instance(link, ds_window_t, ldwindows);
}

/** Get previous window in client.
 *
 * @param wnd Current window
 * @return Previous window or @c NULL if there is none
 */
ds_window_t *ds_display_prev_window(ds_window_t *wnd)
{
	link_t *link = list_prev(&wnd->ldwindows, &wnd->display->windows);

	if (link == NULL)
		return NULL;

	return list_get_instance(link, ds_window_t, ldwindows);
}

/** Post keyboard event to a display.
 *
 * The event is routed to the correct window by first determining the
 * seat the keyboard device belongs to and then the event is sent to the
 * window focused by that seat.
 *
 * @param display Display
 * @param event Event
 */
errno_t ds_display_post_kbd_event(ds_display_t *display, kbd_event_t *event)
{
	ds_seat_t *seat;

	/* Determine which seat the event belongs to */
	seat = ds_display_seat_by_idev(display, event->kbd_id);
	if (seat == NULL)
		return EOK;

	return ds_seat_post_kbd_event(seat, event);
}

/** Post position event to a display.
 *
 * @param display Display
 * @param event Event
 */
errno_t ds_display_post_ptd_event(ds_display_t *display, ptd_event_t *event)
{
	ds_seat_t *seat;

	/* Determine which seat the event belongs to */
	seat = ds_display_seat_by_idev(display, event->pos_id);
	if (seat == NULL)
		return EOK;

	return ds_seat_post_ptd_event(seat, event);
}

/** Add seat to display.
 *
 * @param disp Display
 * @param seat Seat
 */
void ds_display_add_seat(ds_display_t *disp, ds_seat_t *seat)
{
	assert(seat->display == NULL);
	assert(!link_used(&seat->lseats));

	seat->display = disp;
	seat->id = disp->next_seat_id++;
	list_append(&seat->lseats, &disp->seats);
}

/** Remove seat from display.
 *
 * @param seat Seat
 */
void ds_display_remove_seat(ds_seat_t *seat)
{
	list_remove(&seat->lseats);
	seat->display = NULL;
}

/** Get first seat in display.
 *
 * @param disp Display
 * @return First seat or @c NULL if there is none
 */
ds_seat_t *ds_display_first_seat(ds_display_t *disp)
{
	link_t *link = list_first(&disp->seats);

	if (link == NULL)
		return NULL;

	return list_get_instance(link, ds_seat_t, lseats);
}

/** Get next seat in display.
 *
 * @param seat Current seat
 * @return Next seat or @c NULL if there is none
 */
ds_seat_t *ds_display_next_seat(ds_seat_t *seat)
{
	link_t *link = list_next(&seat->lseats, &seat->display->seats);

	if (link == NULL)
		return NULL;

	return list_get_instance(link, ds_seat_t, lseats);
}

/** Get default seat in display.
 *
 * @param disp Display
 * @return First seat or @c NULL if there is none
 */
ds_seat_t *ds_display_default_seat(ds_display_t *disp)
{
	/* XXX Probably not the best solution */
	return ds_display_first_seat(disp);
}

/** Find seat by ID.
 *
 * @param display Display
 * @param id Seat ID
 */
ds_seat_t *ds_display_find_seat(ds_display_t *display, ds_seat_id_t id)
{
	ds_seat_t *seat;

	seat = ds_display_first_seat(display);
	while (seat != NULL) {
		if (seat->id == id)
			return seat;

		seat = ds_display_next_seat(seat);
	}

	return NULL;
}

/** Get seat which owns the specified input device.
 *
 * @param disp Display
 * @param idev_id Input device ID
 * @return Seat which owns device with ID @a idev_id or @c NULL if not found
 */
ds_seat_t *ds_display_seat_by_idev(ds_display_t *disp, ds_idev_id_t idev_id)
{
	ds_idevcfg_t *idevcfg;

	/*
	 * Find input device configuration entry that maps this input device
	 * to a seat.
	 */
	idevcfg = ds_display_first_idevcfg(disp);
	while (idevcfg != NULL) {
		if (idevcfg->svc_id == idev_id)
			return idevcfg->seat;

		idevcfg = ds_display_next_idevcfg(idevcfg);
	}

	/* If none was found, return the default seat */
	return ds_display_default_seat(disp);
}

/** Allocate back buffer for display.
 *
 * @param disp Display
 * @return EOK on success or if no back buffer is required, otherwise
 *         an error code.
 */
static errno_t ds_display_alloc_backbuf(ds_display_t *disp)
{
	gfx_context_t *ugc;
	gfx_bitmap_params_t params;
	gfx_bitmap_alloc_t alloc;
	errno_t rc;

	/* Allocate backbuffer */
	if ((disp->flags & df_disp_double_buf) == 0) {
		/* Not double buffering. Nothing to do. */
		return EOK;
	}

	ugc = ds_display_get_unbuf_gc(disp);

	gfx_bitmap_params_init(&params);
	params.rect = disp->rect;

	rc = gfx_bitmap_create(ugc, &params, NULL,
	    &disp->backbuf);
	if (rc != EOK)
		goto error;

	rc = gfx_bitmap_get_alloc(disp->backbuf, &alloc);
	if (rc != EOK)
		goto error;

	rc = mem_gc_create(&disp->rect, &alloc, &ds_display_mem_gc_cb,
	    (void *) disp, &disp->bbgc);
	if (rc != EOK)
		goto error;

	disp->dirty_rect.p0.x = 0;
	disp->dirty_rect.p0.y = 0;
	disp->dirty_rect.p1.x = 0;
	disp->dirty_rect.p1.y = 0;

	return EOK;
error:
	if (disp->backbuf != NULL) {
		gfx_bitmap_destroy(disp->backbuf);
		disp->backbuf = NULL;
	}

	return rc;
}

/** Add display device to display.
 *
 * @param disp Display
 * @param ddev Display device
 * @return EOK on success, or an error code
 */
errno_t ds_display_add_ddev(ds_display_t *disp, ds_ddev_t *ddev)
{
	errno_t rc;
	gfx_rect_t old_disp_rect;

	assert(ddev->display == NULL);
	assert(!link_used(&ddev->lddevs));

	old_disp_rect = disp->rect;

	ddev->display = disp;
	list_append(&ddev->lddevs, &disp->ddevs);

	/* First display device */
	if (gfx_rect_is_empty(&disp->rect)) {
		/* Set screen dimensions */
		disp->rect = ddev->info.rect;

		/* Create cloning GC */
		rc = ds_clonegc_create(ddev->gc, &disp->fbgc);
		if (rc != EOK)
			goto error;

		/* Allocate backbuffer */
		rc = ds_display_alloc_backbuf(disp);
		if (rc != EOK) {
			ds_clonegc_delete(disp->fbgc);
			disp->fbgc = NULL;
			goto error;
		}
	} else {
		/* Add new output device to cloning GC */
		rc = ds_clonegc_add_output(disp->fbgc, ddev->gc);
		if (rc != EOK)
			goto error;
	}

	ds_display_update_max_rect(disp);

	return EOK;
error:
	disp->rect = old_disp_rect;
	list_remove(&ddev->lddevs);
	return rc;
}

/** Remove display device from display.
 *
 * @param ddev Display device
 */
void ds_display_remove_ddev(ds_ddev_t *ddev)
{
	list_remove(&ddev->lddevs);
	ddev->display = NULL;
}

/** Get first display device in display.
 *
 * @param disp Display
 * @return First display device or @c NULL if there is none
 */
ds_ddev_t *ds_display_first_ddev(ds_display_t *disp)
{
	link_t *link = list_first(&disp->ddevs);

	if (link == NULL)
		return NULL;

	return list_get_instance(link, ds_ddev_t, lddevs);
}

/** Get next display device in display.
 *
 * @param ddev Current display device
 * @return Next display device or @c NULL if there is none
 */
ds_ddev_t *ds_display_next_ddev(ds_ddev_t *ddev)
{
	link_t *link = list_next(&ddev->lddevs, &ddev->display->ddevs);

	if (link == NULL)
		return NULL;

	return list_get_instance(link, ds_ddev_t, lddevs);
}

/** Add input device configuration entry to display.
 *
 * @param disp Display
 * @param idevcfg Input device configuration
 */
void ds_display_add_idevcfg(ds_display_t *disp, ds_idevcfg_t *idevcfg)
{
	assert(idevcfg->display == NULL);
	assert(!link_used(&idevcfg->ldispidcfgs));

	idevcfg->display = disp;
	list_append(&idevcfg->ldispidcfgs, &disp->idevcfgs);
}

/** Remove input device configuration entry from display.
 *
 * @param idevcfg Input device configuration entry
 */
void ds_display_remove_idevcfg(ds_idevcfg_t *idevcfg)
{
	list_remove(&idevcfg->ldispidcfgs);
	idevcfg->display = NULL;
}

/** Get first input device configuration entry in display.
 *
 * @param disp Display
 * @return First input device configuration entry or @c NULL if there is none
 */
ds_idevcfg_t *ds_display_first_idevcfg(ds_display_t *disp)
{
	link_t *link = list_first(&disp->idevcfgs);

	if (link == NULL)
		return NULL;

	return list_get_instance(link, ds_idevcfg_t, ldispidcfgs);
}

/** Get next input device configuration entry in display.
 *
 * @param idevcfg Current input device configuration entry
 * @return Next input device configuration entry or @c NULL if there is none
 */
ds_idevcfg_t *ds_display_next_idevcfg(ds_idevcfg_t *idevcfg)
{
	link_t *link = list_next(&idevcfg->ldispidcfgs, &idevcfg->display->idevcfgs);

	if (link == NULL)
		return NULL;

	return list_get_instance(link, ds_idevcfg_t, ldispidcfgs);
}

/** Add cursor to display.
 *
 * @param display Display
 * @param cursor Cursor
 */
void ds_display_add_cursor(ds_display_t *display, ds_cursor_t *cursor)
{
	assert(cursor->display == NULL);
	assert(!link_used(&cursor->ldisplay));

	cursor->display = display;
	list_prepend(&cursor->ldisplay, &display->cursors);
}

/** Remove cursor from display.
 *
 * @param cursor Cursor
 */
void ds_display_remove_cursor(ds_cursor_t *cursor)
{
	list_remove(&cursor->ldisplay);
	cursor->display = NULL;
}

/** Update display maximize rectangle.
 *
 * Recalculate the maximize rectangle (the rectangle used for maximized
 * windows).
 *
 * @param display Display
 */
void ds_display_update_max_rect(ds_display_t *display)
{
	ds_window_t *wnd;
	gfx_rect_t max_rect;
	gfx_rect_t drect;

	/* Start with the entire display */
	max_rect = display->rect;

	wnd = ds_display_first_window(display);
	while (wnd != NULL) {
		/* Should maximized windows avoid this window? */
		if ((wnd->flags & wndf_avoid) != 0) {
			/* Window bounding rectangle on display */
			gfx_rect_translate(&wnd->dpos, &wnd->rect, &drect);

			/* Crop maximized rectangle */
			ds_display_crop_max_rect(&drect, &max_rect);
		}

		wnd = ds_display_next_window(wnd);
	}

	/* Update the maximize rectangle */
	display->max_rect = max_rect;
}

/** Crop maximize rectangle.
 *
 * Use the avoid rectangle @a arect to crop off maximization rectangle
 * @a mrect. If @a arect covers the top, bottom, left or right part
 * of @a mrect, it will be cropped off. Otherwise there will be
 * no effect.
 *
 * @param arect Avoid rectangle
 * @param mrect Maximize rectangle to be modified
 */
void ds_display_crop_max_rect(gfx_rect_t *arect, gfx_rect_t *mrect)
{
	if (arect->p0.x == mrect->p0.x && arect->p0.y == mrect->p0.y &&
	    arect->p1.x == mrect->p1.x) {
		/* Cropp off top part */
		mrect->p0.y = arect->p1.y;
	} else if (arect->p0.x == mrect->p0.x && arect->p1.x == mrect->p1.x &&
	    arect->p1.y == mrect->p1.y) {
		/* Cropp off bottom part */
		mrect->p1.y = arect->p0.y;
	} else if (arect->p0.x == mrect->p0.x && arect->p0.y == mrect->p0.y &&
	    arect->p1.y == mrect->p1.y) {
		/* Cropp off left part */
		mrect->p0.x = arect->p1.x;
	} else if (arect->p0.y == mrect->p0.y && arect->p1.x == mrect->p1.x &&
	    arect->p1.y == mrect->p1.y) {
		/* Cropp off right part */
		mrect->p1.x = arect->p0.x;
	}
}

/** Get unbuffered GC.
 *
 * Get the display's (unbuffered) graphic context. If the display
 * is double-buffered, this returns GC of the front buffer. If the display
 * is unbuffered, this is the same as @c ds_display_get_gc().
 *
 * @param display Display
 * @return Unbuffered GC
 */
static gfx_context_t *ds_display_get_unbuf_gc(ds_display_t *display)
{
	/* In case of unit tests */
	if (display->fbgc == NULL)
		return NULL;

	return ds_clonegc_get_ctx(display->fbgc);
}

/** Get display GC.
 *
 * Get the graphic context used to paint the display. This is to be used
 * for all display server paint operations.
 *
 * @param display Display
 * @return Graphic context for painting to the display
 */
gfx_context_t *ds_display_get_gc(ds_display_t *display)
{
	if ((display->flags & df_disp_double_buf) != 0)
		return mem_gc_get_ctx(display->bbgc);
	else
		return ds_display_get_unbuf_gc(display);
}

/** Paint display background.
 *
 * @param display Display
 * @param rect Bounding rectangle or @c NULL to repaint entire display
 */
errno_t ds_display_paint_bg(ds_display_t *disp, gfx_rect_t *rect)
{
	gfx_rect_t crect;
	gfx_context_t *gc;
	errno_t rc;

	if (rect != NULL)
		gfx_rect_clip(&disp->rect, rect, &crect);
	else
		crect = disp->rect;

	gc = ds_display_get_gc(disp);
	if (gc == NULL)
		return EOK;

	rc = gfx_set_color(gc, disp->bg_color);
	if (rc != EOK)
		return rc;

	return gfx_fill_rect(gc, &crect);
}

/** Update front buffer from back buffer.
 *
 * If the display is not double-buffered, no action is taken.
 *
 * @param disp Display
 * @return EOK on success, or an error code
 */
static errno_t ds_display_update(ds_display_t *disp)
{
	errno_t rc;

	if (disp->backbuf == NULL) {
		/* Not double-buffered, nothing to do. */
		return EOK;
	}

	rc = gfx_bitmap_render(disp->backbuf, &disp->dirty_rect, NULL);
	if (rc != EOK)
		return rc;

	disp->dirty_rect.p0.x = 0;
	disp->dirty_rect.p0.y = 0;
	disp->dirty_rect.p1.x = 0;
	disp->dirty_rect.p1.y = 0;

	return EOK;
}

/** Paint display.
 *
 * @param display Display
 * @param rect Bounding rectangle or @c NULL to repaint entire display
 */
errno_t ds_display_paint(ds_display_t *disp, gfx_rect_t *rect)
{
	errno_t rc;
	ds_window_t *wnd;
	ds_seat_t *seat;

	/* Paint background */
	rc = ds_display_paint_bg(disp, rect);
	if (rc != EOK)
		return rc;

	/* Paint windows bottom to top */
	wnd = ds_display_last_window(disp);
	while (wnd != NULL) {
		rc = ds_window_paint(wnd, rect);
		if (rc != EOK)
			return rc;

		wnd = ds_display_prev_window(wnd);
	}

	/* Paint window previews for windows being resized or moved */
	wnd = ds_display_last_window(disp);
	while (wnd != NULL) {
		rc = ds_window_paint_preview(wnd, rect);
		if (rc != EOK)
			return rc;

		wnd = ds_display_prev_window(wnd);
	}

	/* Paint pointers */
	seat = ds_display_first_seat(disp);
	while (seat != NULL) {
		rc = ds_seat_paint_pointer(seat, rect);
		if (rc != EOK)
			return rc;

		seat = ds_display_next_seat(seat);
	}

	return ds_display_update(disp);
}

/** Display invalidate callback.
 *
 * Called by backbuffer memory GC when something is rendered into it.
 * Updates the display's dirty rectangle.
 *
 * @param arg Argument (display cast as void *)
 * @param rect Rectangle to update
 */
static void ds_display_invalidate_cb(void *arg, gfx_rect_t *rect)
{
	ds_display_t *disp = (ds_display_t *) arg;
	gfx_rect_t env;

	gfx_rect_envelope(&disp->dirty_rect, rect, &env);
	disp->dirty_rect = env;
}

/** Display update callback.
 *
 * @param arg Argument (display cast as void *)
 */
static void ds_display_update_cb(void *arg)
{
	ds_display_t *disp = (ds_display_t *) arg;

	(void) disp;
}

/** @}
 */
