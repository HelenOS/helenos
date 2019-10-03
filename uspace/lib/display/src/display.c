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

#include <async.h>
#include <display.h>
#include <errno.h>
#include <fibril_synch.h>
#include <ipc/display.h>
#include <ipc/services.h>
#include <ipcgfx/client.h>
#include <loc.h>
#include <stdlib.h>

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

	list_initialize(&display->windows);

	if (dsname == NULL)
		dsname = SERVICE_NAME_DISPLAY;

	rc = loc_service_get_id(dsname, &display_svc, IPC_FLAG_BLOCKING);
	if (rc != EOK) {
		free(display);
		return ENOENT;
	}

	display->sess = loc_service_connect(display_svc, INTERFACE_DISPLAY,
	    IPC_FLAG_BLOCKING);
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
	async_hangup(display->sess);

	/* Wait for callback handler to terminate */

	fibril_mutex_lock(&display->lock);
	while (!display->cb_done)
		fibril_condvar_wait(&display->cv, &display->lock);
	fibril_mutex_unlock(&display->lock);

	free(display);
}

/** Create a display window.
 *
 * @param display Display
 * @param cb Callback functions
 * @param cb_arg Argument to callback functions
 * @param rwindow Place to store pointer to new window
 * @return EOK on success or an error code
 */
errno_t display_window_create(display_t *display, display_wnd_cb_t *cb,
    void *cb_arg, display_window_t **rwindow)
{
	display_window_t *window;
	async_exch_t *exch;
	sysarg_t wnd_id;
	errno_t rc;

	window = calloc(1, sizeof(display_window_t));
	if (window == NULL)
		return ENOMEM;

	exch = async_exchange_begin(display->sess);
	rc = async_req_0_1(exch, DISPLAY_WINDOW_CREATE, &wnd_id);

	async_exchange_end(exch);

	if (rc != EOK) {
		free(window);
		return rc;
	}

	window->display = display;
	window->id = wnd_id;
	window->cb = cb;
	window->cb_arg = cb_arg;

	list_append(&window->lwindows, &display->windows);
	*rwindow = window;
	return EOK;
}

/** Destroy display window.
 *
 * @param window Window
 * @return EOK on success or an error code. In both cases @a window must
 *         not be accessed anymore
 */
errno_t display_window_destroy(display_window_t *window)
{
	async_exch_t *exch;
	errno_t rc;

	exch = async_exchange_begin(window->display->sess);
	rc = async_req_1_0(exch, DISPLAY_WINDOW_DESTROY, window->id);

	async_exchange_end(exch);

	free(window);
	return rc;
}

/** Create graphics context for drawing into a window.
 *
 * @param window Window
 * @param rgc Place to store pointer to new graphics context
 */
errno_t display_window_get_gc(display_window_t *window, gfx_context_t **rgc)
{
	async_sess_t *sess;
	async_exch_t *exch;
	ipc_gc_t *gc;
	errno_t rc;

	exch = async_exchange_begin(window->display->sess);
	sess = async_connect_me_to(exch, INTERFACE_GC, 0, window->id);
	if (sess == NULL) {
		async_exchange_end(exch);
		return EIO;
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

/** Get display event.
 *
 * @param display Display
 * @param rwindow Place to store pointe to window that received event
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
	if (rc != EOK) {
		async_forget(req);
		return rc;
	}

	async_exchange_end(exch);

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
		rc = display_get_event(display, &window, &event);
		if (rc != EOK)
			break;

		if (window->cb->kbd_event != NULL)
			window->cb->kbd_event(window->cb_arg, &event.kbd_event);
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
