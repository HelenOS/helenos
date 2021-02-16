/*
 * Copyright (c) 2019 Jiri Svoboda
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
#include "client.h"
#include "clonegc.h"
#include "cursimg.h"
#include "cursor.h"
#include "seat.h"
#include "window.h"
#include "display.h"

static gfx_context_t *ds_display_get_unbuf_gc(ds_display_t *);
static void ds_display_invalidate_cb(void *, gfx_rect_t *);
static void ds_display_update_cb(void *);

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
	disp->next_wnd_id = 1;
	list_initialize(&disp->ddevs);
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
	assert(list_empty(&disp->clients));
	assert(list_empty(&disp->seats));
	/* XXX destroy cursors */
	gfx_color_delete(disp->bg_color);
	free(disp);
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

		if (gfx_pix_inside_rect(pos, &drect))
			return wnd;

		wnd = ds_display_next_window(wnd);
	}

	return NULL;
}

/** Add window to display.
 *
 * @param display Display
 * @param wnd Window
 */
void ds_display_add_window(ds_display_t *display, ds_window_t *wnd)
{
	assert(wnd->display == NULL);
	assert(!link_used(&wnd->ldwindows));

	wnd->display = display;
	list_prepend(&wnd->ldwindows, &display->windows);
}

/** Remove window from display.
 *
 * @param wnd Window
 */
void ds_display_remove_window(ds_window_t *wnd)
{
	list_remove(&wnd->ldwindows);
	wnd->display = NULL;
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

	// TODO Determine which seat the event belongs to
	seat = ds_display_first_seat(display);
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

	// TODO Determine which seat the event belongs to
	seat = ds_display_first_seat(display);
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

	rc = mem_gc_create(&disp->rect, &alloc,
	    ds_display_invalidate_cb, ds_display_update_cb, (void *) disp,
	    &disp->bbgc);
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

	assert(ddev->display == NULL);
	assert(!link_used(&ddev->lddevs));

	ddev->display = disp;
	list_append(&ddev->lddevs, &disp->ddevs);

	/* First display device */
	if (gfx_rect_is_empty(&disp->rect)) {
		/* Set screen dimensions */
		disp->rect = ddev->info.rect;

		/* Create cloning GC */
		rc = ds_clonegc_create(ddev->gc, &disp->fbgc);
		if (rc != EOK) {
			// XXX Remove output
			return ENOMEM;
		}

		/* Allocate backbuffer */
		rc = ds_display_alloc_backbuf(disp);
		if (rc != EOK) {
			// XXX Remove output
			// XXX Delete clone GC
			goto error;
		}
	} else {
		/* Add new output device to cloning GC */
		rc = ds_clonegc_add_output(disp->fbgc, ddev->gc);
		if (rc != EOK)
			goto error;
	}

	return EOK;
error:
	disp->rect.p0.x = 0;
	disp->rect.p0.y = 0;
	disp->rect.p1.x = 0;
	disp->rect.p1.y = 0;
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
