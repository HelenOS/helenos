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
#include <ipc/display.h>
#include <ipc/services.h>
#include <ipcgfx/client.h>
#include <loc.h>
#include <stdlib.h>

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

	*rdisplay = display;
	return EOK;
}

/** Close display service.
 *
 * @param display Display session
 */
void display_close(display_t *display)
{
	async_hangup(display->sess);
	free(display);
}

/** Create a display window.
 *
 * @param display Display
 * @param rwindow Place to store pointer to new window
 * @return EOK on success or an error code
 */
errno_t display_window_create(display_t *display, display_window_t **rwindow)
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

/** @}
 */
