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

#include <async.h>
#include <display.h>
#include <display/event.h>
#include <errno.h>
#include <fibril_synch.h>
#include <ipc/display.h>
#include <ipc/services.h>
#include <ipcgfx/client.h>
#include <loc.h>
#include <mem.h>
#include <stdlib.h>
#include <str.h>
#include "../private/display.h"
#include "../private/params.h"

static errno_t display_callback_create(display_t *);
static void display_cb_conn(ipc_call_t *, void *);
static errno_t display_get_window(display_t *, sysarg_t, display_window_t **);

/** Open display service.
 *
 * @param dsname Display service name or @c NULL to use default display
 * @param rdisplay Place to store pointer to display session
 * @return EOK on success or an error code
 */
errno_t display_open(const char *dsname, display_t **rdisplay)
{
	service_id_t display_svc;
	display_t *display;
	errno_t rc;

	display = calloc(1, sizeof(display_t));
	if (display == NULL)
		return ENOMEM;

	fibril_mutex_initialize(&display->lock);
	fibril_condvar_initialize(&display->cv);
	list_initialize(&display->windows);

	if (dsname == NULL)
		dsname = SERVICE_NAME_DISPLAY;

	rc = loc_service_get_id(dsname, &display_svc, 0);
	if (rc != EOK) {
		free(display);
		return ENOENT;
	}

	display->sess = loc_service_connect(display_svc, INTERFACE_DISPLAY,
	    0);
	if (display->sess == NULL) {
		free(display);
		return ENOENT;
	}

	rc = display_callback_create(display);
	if (rc != EOK) {
		async_hangup(display->sess);
		free(display);
		return EIO;
	}

	*rdisplay = display;
	return EOK;
}

/** Create callback connection from display service.
 *
 * @param display Display session
 * @return EOK on success or an error code
 */
static errno_t display_callback_create(display_t *display)
{
	async_exch_t *exch = async_exchange_begin(display->sess);

	aid_t req = async_send_0(exch, DISPLAY_CALLBACK_CREATE, NULL);

	port_id_t port;
	errno_t rc = async_create_callback_port(exch, INTERFACE_DISPLAY_CB, 0, 0,
	    display_cb_conn, display, &port);

	async_exchange_end(exch);

	if (rc != EOK)
		return rc;

	errno_t retval;
	async_wait_for(req, &retval);

	return retval;
}

/** Close display service.
 *
 * @param display Display session
 */
void display_close(display_t *display)
{
	fibril_mutex_lock(&display->lock);
	async_hangup(display->sess);
	display->sess = NULL;

	/* Wait for callback handler to terminate */

	while (!display->cb_done)
		fibril_condvar_wait(&display->cv, &display->lock);
	fibril_mutex_unlock(&display->lock);

	free(display);
}

/** Initialize window parameters structure.
 *
 * Window parameters structure must always be initialized using this function
 * first.
 *
 * @param params Window parameters structure
 */
void display_wnd_params_init(display_wnd_params_t *params)
{
	memset(params, 0, sizeof(*params));
	params->caption = "";
}

/** Create a display window.
 *
 * @param display Display
 * @param params Window parameters
 * @param cb Callback functions
 * @param cb_arg Argument to callback functions
 * @param rwindow Place to store pointer to new window
 * @return EOK on success or an error code
 */
errno_t display_window_create(display_t *display, display_wnd_params_t *params,
    display_wnd_cb_t *cb, void *cb_arg, display_window_t **rwindow)
{
	display_window_t *window;
	display_wnd_params_enc_t eparams;
	async_exch_t *exch;
	aid_t req;
	ipc_call_t answer;
	errno_t rc;

	/* Encode the parameters for transport */
	eparams.rect = params->rect;
	eparams.caption_size = str_size(params->caption);
	eparams.min_size = params->min_size;
	eparams.pos = params->pos;
	eparams.flags = params->flags;
	eparams.idev_id = params->idev_id;

	window = calloc(1, sizeof(display_window_t));
	if (window == NULL)
		return ENOMEM;

	exch = async_exchange_begin(display->sess);
	req = async_send_0(exch, DISPLAY_WINDOW_CREATE, &answer);

	/* Write fixed fields */
	rc = async_data_write_start(exch, &eparams,
	    sizeof (display_wnd_params_enc_t));
	if (rc != EOK) {
		async_exchange_end(exch);
		async_forget(req);
		free(window);
		return rc;
	}

	/* Write caption */
	rc = async_data_write_start(exch, params->caption,
	    eparams.caption_size);
	async_exchange_end(exch);
	if (rc != EOK) {
		async_forget(req);
		free(window);
		return rc;
	}

	async_wait_for(req, &rc);
	if (rc != EOK) {
		free(window);
		return rc;
	}

	window->display = display;
	window->id = ipc_get_arg1(&answer);
	window->cb = cb;
	window->cb_arg = cb_arg;

	list_append(&window->lwindows, &display->windows);
	*rwindow = window;
	return EOK;
}

/** Destroy display window.
 *
 * @param window Window or @c NULL
 * @return EOK on success or an error code. In both cases @a window must
 *         not be accessed anymore
 */
errno_t display_window_destroy(display_window_t *window)
{
	async_exch_t *exch;
	errno_t rc;

	if (window == NULL)
		return EOK;

	exch = async_exchange_begin(window->display->sess);
	rc = async_req_1_0(exch, DISPLAY_WINDOW_DESTROY, window->id);

	async_exchange_end(exch);

	list_remove(&window->lwindows);
	free(window);
	return rc;
}

/** Create graphics context for drawing into a window.
 *
 * @param window Window
 * @param rgc Place to store pointer to new graphics context
 * @return EOK on success or an error code
 */
errno_t display_window_get_gc(display_window_t *window, gfx_context_t **rgc)
{
	async_sess_t *sess;
	async_exch_t *exch;
	ipc_gc_t *gc;
	errno_t rc;

	exch = async_exchange_begin(window->display->sess);
	sess = async_connect_me_to(exch, INTERFACE_GC, 0, window->id, &rc);
	if (sess == NULL) {
		async_exchange_end(exch);
		return rc;
	}

	async_exchange_end(exch);

	rc = ipc_gc_create(sess, &gc);
	if (rc != EOK) {
		async_hangup(sess);
		return ENOMEM;
	}

	*rgc = ipc_gc_get_ctx(gc);
	return EOK;
}

/** Request a window move.
 *
 * Request the display service to initiate a user window move operation
 * (i.e. let the user move the window). Used when the client detects
 * mouse press on the title bar or such.
 *
 * @param window Window
 * @param pos Position in the window where the button was pressed
 * @param pos_id Positioning device ID
 * @return EOK on success or an error code
 */
errno_t display_window_move_req(display_window_t *window, gfx_coord2_t *pos,
    sysarg_t pos_id)
{
	async_exch_t *exch;
	aid_t req;
	ipc_call_t answer;
	errno_t rc;

	exch = async_exchange_begin(window->display->sess);
	req = async_send_2(exch, DISPLAY_WINDOW_MOVE_REQ, window->id,
	    pos_id, &answer);
	rc = async_data_write_start(exch, (void *)pos, sizeof (gfx_coord2_t));
	async_exchange_end(exch);
	if (rc != EOK) {
		async_forget(req);
		return rc;
	}

	async_wait_for(req, &rc);
	if (rc != EOK)
		return rc;

	return EOK;
}

/** Move display window.
 *
 * Set new display position of a window. Display position determines where
 * the origin of the window coordinate system lies. Note that the top left
 * corner of the window need not coincide with the window's 0,0 point.
 *
 * @param window Window
 * @param dpos New display position
 * @return EOK on success or an error code
 */
errno_t display_window_move(display_window_t *window, gfx_coord2_t *dpos)
{
	async_exch_t *exch;
	aid_t req;
	ipc_call_t answer;
	errno_t rc;

	exch = async_exchange_begin(window->display->sess);
	req = async_send_1(exch, DISPLAY_WINDOW_MOVE, window->id, &answer);
	rc = async_data_write_start(exch, dpos, sizeof (gfx_coord2_t));
	async_exchange_end(exch);
	if (rc != EOK) {
		async_forget(req);
		return rc;
	}

	async_wait_for(req, &rc);
	if (rc != EOK)
		return rc;

	return EOK;
}

/** Get display window position.
 *
 * Get display window position on the display.
 *
 * @param window Window
 * @param dpos Place to store position
 * @return EOK on success or an error code
 */
errno_t display_window_get_pos(display_window_t *window, gfx_coord2_t *dpos)
{
	async_exch_t *exch;
	aid_t req;
	ipc_call_t answer;
	errno_t rc;

	exch = async_exchange_begin(window->display->sess);
	req = async_send_1(exch, DISPLAY_WINDOW_GET_POS, window->id, &answer);
	rc = async_data_read_start(exch, dpos, sizeof (gfx_coord2_t));
	async_exchange_end(exch);
	if (rc != EOK) {
		async_forget(req);
		return rc;
	}

	async_wait_for(req, &rc);
	if (rc != EOK)
		return rc;

	return EOK;
}

/** Get display window maximized rectangle.
 *
 * Get the rectangle to which a window would be maximized.
 *
 * @param window Window
 * @param rect Place to store maximized rectangle
 * @return EOK on success or an error code
 */
errno_t display_window_get_max_rect(display_window_t *window, gfx_rect_t *rect)
{
	async_exch_t *exch;
	aid_t req;
	ipc_call_t answer;
	errno_t rc;

	exch = async_exchange_begin(window->display->sess);
	req = async_send_1(exch, DISPLAY_WINDOW_GET_MAX_RECT, window->id,
	    &answer);
	rc = async_data_read_start(exch, rect, sizeof (gfx_rect_t));
	async_exchange_end(exch);
	if (rc != EOK) {
		async_forget(req);
		return rc;
	}

	async_wait_for(req, &rc);
	if (rc != EOK)
		return rc;

	return EOK;
}

/** Request a window resize.
 *
 * Request the display service to initiate a user window resize operation
 * (i.e. let the user resize the window). Used when the client detects
 * mouse press on the window frame or such.
 *
 * @param window Window
 * @param rsztype Resize type (which part of window frame is being dragged)
 * @param pos Position in the window where the button was pressed
 * @param pos_id Positioning device ID
 * @return EOK on success or an error code
 */
errno_t display_window_resize_req(display_window_t *window,
    display_wnd_rsztype_t rsztype, gfx_coord2_t *pos, sysarg_t pos_id)
{
	async_exch_t *exch;
	aid_t req;
	ipc_call_t answer;
	errno_t rc;

	exch = async_exchange_begin(window->display->sess);
	req = async_send_3(exch, DISPLAY_WINDOW_RESIZE_REQ, window->id,
	    (sysarg_t) rsztype, pos_id, &answer);
	rc = async_data_write_start(exch, (void *)pos, sizeof (gfx_coord2_t));
	async_exchange_end(exch);
	if (rc != EOK) {
		async_forget(req);
		return rc;
	}

	async_wait_for(req, &rc);
	if (rc != EOK)
		return rc;

	return EOK;
}

/** Resize display window.
 *
 * It seems resizing windows should be easy with bounding rectangles.
 * You have an old bounding rectangle and a new bounding rectangle (@a nrect).
 * Change .p0 and top-left corner moves. Change .p1 and bottom-right corner
 * moves. Piece of cake!
 *
 * There's always a catch, though. By series of resizes and moves .p0 could
 * drift outside of the range of @c gfx_coord_t. Now what? @a offs to the
 * rescue! @a offs moves the @em boundaries of the window with respect
 * to the display, while keeping the @em contents of the window in the
 * same place (with respect to the display). In other words, @a offs shifts
 * the window's internal coordinate system.
 *
 * A few examples follow:
 *
 * Enlarge window by moving bottom-right corner 1 right, 1 down:
 *
 *   bound = (0, 0, 10, 10)
 *   offs  = (0, 0)
 *   nrect = (0, 0, 11, 11)
 *
 * Enlarge window by moving top-left corner, 1 up, 1 left, allowing the
 * window-relative coordinate of the top-left corner to drift (undesirable)
 *
 *   bound = (0, 0, 10, 10)
 *   offs  = (0, 0)
 *   nrect = (-1, -1, 10, 10) <- this is the new bounding rectangle
 *
 * Enlarge window by moving top-left corner 1 up, 1 left, keeping top-left
 * corner locked to (0,0) window-relative coordinates (desirable):
 *
 *   bound = (0, 0, 10, 10)
 *   off   = (-1,-1)        <- top-left corner goes 1 up, 1 left
 *   nrect = (0, 0, 11, 11) <- window still starts at 0,0 window-relative
 *
 * @param window Window
 * @param nrect New bounding rectangle
 * @param offs
 * @return EOK on success or an error code
 */
errno_t display_window_resize(display_window_t *window, gfx_coord2_t *offs,
    gfx_rect_t *nrect)
{
	async_exch_t *exch;
	aid_t req;
	ipc_call_t answer;
	display_wnd_resize_t wresize;
	errno_t rc;

	wresize.offs = *offs;
	wresize.nrect = *nrect;

	exch = async_exchange_begin(window->display->sess);
	req = async_send_1(exch, DISPLAY_WINDOW_RESIZE, window->id, &answer);
	rc = async_data_write_start(exch, &wresize, sizeof (display_wnd_resize_t));
	async_exchange_end(exch);
	if (rc != EOK) {
		async_forget(req);
		return rc;
	}

	async_wait_for(req, &rc);
	if (rc != EOK)
		return rc;

	return EOK;
}

/** Minimize window.
 *
 * @param window Window
 * @return EOK on success or an error code
 */
errno_t display_window_minimize(display_window_t *window)
{
	async_exch_t *exch;
	errno_t rc;

	exch = async_exchange_begin(window->display->sess);
	rc = async_req_1_0(exch, DISPLAY_WINDOW_MINIMIZE, window->id);
	async_exchange_end(exch);

	return rc;
}

/** Maximize window.
 *
 * @param window Window
 * @return EOK on success or an error code
 */
errno_t display_window_maximize(display_window_t *window)
{
	async_exch_t *exch;
	errno_t rc;

	exch = async_exchange_begin(window->display->sess);
	rc = async_req_1_0(exch, DISPLAY_WINDOW_MAXIMIZE, window->id);
	async_exchange_end(exch);

	return rc;
}

/** Unmaximize window.
 *
 * @param window Window
 * @return EOK on success or an error code
 */
errno_t display_window_unmaximize(display_window_t *window)
{
	async_exch_t *exch;
	errno_t rc;

	exch = async_exchange_begin(window->display->sess);
	rc = async_req_1_0(exch, DISPLAY_WINDOW_UNMAXIMIZE, window->id);
	async_exchange_end(exch);

	return rc;
}

/** Set window cursor.
 *
 * Set cursor that is displayed when pointer is over the window. The default
 * is the arrow pointer.
 *
 * @param window Window
 * @param cursor Cursor to display
 * @return EOK on success or an error code
 */
errno_t display_window_set_cursor(display_window_t *window,
    display_stock_cursor_t cursor)
{
	async_exch_t *exch;
	errno_t rc;

	exch = async_exchange_begin(window->display->sess);
	rc = async_req_2_0(exch, DISPLAY_WINDOW_SET_CURSOR, window->id,
	    cursor);
	async_exchange_end(exch);
	return rc;
}

/** Set display window caption.
 *
 * @param window Window
 * @param caption New caption
 * @return EOK on success or an error code
 */
errno_t display_window_set_caption(display_window_t *window,
    const char *caption)
{
	async_exch_t *exch;
	aid_t req;
	ipc_call_t answer;
	size_t cap_size;
	errno_t rc;

	cap_size = str_size(caption);

	exch = async_exchange_begin(window->display->sess);
	req = async_send_1(exch, DISPLAY_WINDOW_SET_CAPTION, window->id,
	    &answer);

	/* Write caption */
	rc = async_data_write_start(exch, caption, cap_size);
	async_exchange_end(exch);
	if (rc != EOK) {
		async_forget(req);
		return rc;
	}

	async_wait_for(req, &rc);
	return rc;
}

/** Get display event.
 *
 * @param display Display
 * @param rwindow Place to store pointer to window that received event
 * @param event Place to store event
 * @return EOK on success or an error code
 */
static errno_t display_get_event(display_t *display, display_window_t **rwindow,
    display_wnd_ev_t *event)
{
	async_exch_t *exch;
	ipc_call_t answer;
	aid_t req;
	errno_t rc;
	sysarg_t wnd_id;
	display_window_t *window;

	exch = async_exchange_begin(display->sess);
	req = async_send_0(exch, DISPLAY_GET_EVENT, &answer);
	rc = async_data_read_start(exch, event, sizeof(*event));
	async_exchange_end(exch);
	if (rc != EOK) {
		async_forget(req);
		return rc;
	}

	async_wait_for(req, &rc);
	if (rc != EOK)
		return rc;

	wnd_id = ipc_get_arg1(&answer);
	rc = display_get_window(display, wnd_id, &window);
	if (rc != EOK)
		return EIO;

	*rwindow = window;
	return EOK;
}

/** Get display information.
 *
 * @param display Display
 * @param info Place to store display information
 * @return EOK on success or an error code
 */
errno_t display_get_info(display_t *display, display_info_t *info)
{
	async_exch_t *exch;
	ipc_call_t answer;
	aid_t req;
	errno_t rc;

	exch = async_exchange_begin(display->sess);
	req = async_send_0(exch, DISPLAY_GET_INFO, &answer);
	rc = async_data_read_start(exch, info, sizeof(*info));
	async_exchange_end(exch);
	if (rc != EOK) {
		async_forget(req);
		return rc;
	}

	async_wait_for(req, &rc);
	if (rc != EOK)
		return rc;

	return EOK;
}

/** Display events are pending.
 *
 * @param display Display
 * @param icall Call data
 */
static void display_ev_pending(display_t *display, ipc_call_t *icall)
{
	errno_t rc;
	display_window_t *window = NULL;
	display_wnd_ev_t event;

	while (true) {
		fibril_mutex_lock(&display->lock);

		if (display->sess != NULL)
			rc = display_get_event(display, &window, &event);
		else
			rc = ENOENT;

		fibril_mutex_unlock(&display->lock);

		if (rc != EOK)
			break;

		switch (event.etype) {
		case wev_close:
			if (window->cb != NULL && window->cb->close_event != NULL) {
				window->cb->close_event(window->cb_arg);
			}
			break;
		case wev_focus:
			if (window->cb != NULL && window->cb->focus_event != NULL) {
				window->cb->focus_event(window->cb_arg,
				    event.ev.focus.nfocus);
			}
			break;
		case wev_kbd:
			if (window->cb != NULL && window->cb->kbd_event != NULL) {
				window->cb->kbd_event(window->cb_arg,
				    &event.ev.kbd);
			}
			break;
		case wev_pos:
			if (window->cb != NULL && window->cb->pos_event != NULL) {
				window->cb->pos_event(window->cb_arg,
				    &event.ev.pos);
			}
			break;
		case wev_resize:
			if (window->cb != NULL && window->cb->resize_event != NULL) {
				window->cb->resize_event(window->cb_arg,
				    &event.ev.resize.rect);
			}
			break;
		case wev_unfocus:
			if (window->cb != NULL && window->cb->unfocus_event != NULL) {
				window->cb->unfocus_event(window->cb_arg,
				    event.ev.unfocus.nfocus);
			}
			break;
		}
	}

	async_answer_0(icall, EOK);
}

/** Callback connection handler.
 *
 * @param icall Connect call data
 * @param arg   Argument, display_t *
 */
static void display_cb_conn(ipc_call_t *icall, void *arg)
{
	display_t *display = (display_t *) arg;

	while (true) {
		ipc_call_t call;
		async_get_call(&call);

		if (!ipc_get_imethod(&call)) {
			/* Hangup */
			async_answer_0(&call, EOK);
			goto out;
		}

		switch (ipc_get_imethod(&call)) {
		case DISPLAY_EV_PENDING:
			display_ev_pending(display, &call);
			break;
		default:
			async_answer_0(&call, ENOTSUP);
			break;
		}
	}

out:
	fibril_mutex_lock(&display->lock);
	display->cb_done = true;
	fibril_mutex_unlock(&display->lock);
	fibril_condvar_broadcast(&display->cv);
}

/** Find window by ID.
 *
 * @param display Display
 * @param wnd_id Window ID
 * @param rwindow Place to store pointer to window
 * @return EOK on success, ENOENT if not found
 */
static errno_t display_get_window(display_t *display, sysarg_t wnd_id,
    display_window_t **rwindow)
{
	link_t *link;
	display_window_t *window;

	link = list_first(&display->windows);
	while (link != NULL) {
		window = list_get_instance(link, display_window_t, lwindows);
		if (window->id == wnd_id) {
			*rwindow = window;
			return EOK;
		}

		link = list_next(link, &display->windows);
	}

	return ENOENT;
}

/** @}
 */
