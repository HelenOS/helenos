/*
 * Copyright (c) 2019 Jiri Svoboda
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * - Redistribution1s of source code must retain the above copyright
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
 * @file Display server main
 */

#include <async.h>
#include <disp_srv.h>
#include <errno.h>
#include <gfx/context.h>
#include <str_error.h>
#include <io/log.h>
#include <io/kbd_event.h>
#include <io/pos_event.h>
#include <ipc/services.h>
#include <ipcgfx/server.h>
#include <loc.h>
#include <stdio.h>
#include <task.h>
#include "client.h"
#include "display.h"
#include "dsops.h"
#include "input.h"
#include "main.h"
#include "output.h"
#include "seat.h"
#include "window.h"

static void display_client_conn(ipc_call_t *, void *);
static void display_client_ev_pending(void *);

#ifdef CONFIG_DISP_DOUBLE_BUF
/*
 * Double buffering is one way to provide flicker-free display.
 */
static ds_display_flags_t disp_flags = df_disp_double_buf;
#else
/*
 * With double buffering disabled, wet screen flicker since front-to-back
 * rendering is not implemented.
 */
static ds_display_flags_t disp_flags = df_none;
#endif

static ds_client_cb_t display_client_cb = {
	.ev_pending = display_client_ev_pending
};

static void display_client_ev_pending(void *arg)
{
	display_srv_t *srv = (display_srv_t *) arg;

	display_srv_ev_pending(srv);
}

/** Initialize display server */
static errno_t display_srv_init(ds_output_t **routput)
{
	ds_display_t *disp = NULL;
	ds_seat_t *seat = NULL;
	ds_output_t *output = NULL;
	gfx_context_t *gc = NULL;
	errno_t rc;

	log_msg(LOG_DEFAULT, LVL_DEBUG, "display_srv_init()");

	rc = ds_display_create(NULL, disp_flags, &disp);
	if (rc != EOK)
		goto error;

	rc = ds_seat_create(disp, &seat);
	if (rc != EOK)
		goto error;

	rc = ds_output_create(&output);
	if (rc != EOK)
		goto error;

	output->def_display = disp;
	rc = ds_output_start_discovery(output);
	if (rc != EOK)
		goto error;

	rc = ds_input_open(disp);
	if (rc != EOK)
		goto error;

	async_set_fallback_port_handler(display_client_conn, disp);

	rc = loc_server_register(NAME);
	if (rc != EOK) {
		log_msg(LOG_DEFAULT, LVL_ERROR, "Failed registering server: %s.", str_error(rc));
		rc = EEXIST;
		goto error;
	}

	service_id_t sid;
	rc = loc_service_register(SERVICE_NAME_DISPLAY, &sid);
	if (rc != EOK) {
		log_msg(LOG_DEFAULT, LVL_ERROR, "Failed registering service: %s.", str_error(rc));
		rc = EEXIST;
		goto error;
	}

	*routput = output;
	return EOK;
error:
#if 0
	if (disp->input != NULL)
		ds_input_close(disp);
#endif
	if (output != NULL)
		ds_output_destroy(output);
	if (gc != NULL)
		gfx_context_delete(gc);
	if (seat != NULL)
		ds_seat_destroy(seat);
	if (disp != NULL)
		ds_display_destroy(disp);
	return rc;
}

/** Handle client connection to display server */
static void display_client_conn(ipc_call_t *icall, void *arg)
{
	display_srv_t srv;
	sysarg_t wnd_id;
	sysarg_t svc_id;
	ds_client_t *client = NULL;
	ds_window_t *wnd;
	ds_display_t *disp = (ds_display_t *) arg;
	gfx_context_t *gc;
	errno_t rc;

	log_msg(LOG_DEFAULT, LVL_DEBUG, "display_client_conn arg1=%zu arg2=%zu arg3=%zu arg4=%zu.",
	    ipc_get_arg1(icall), ipc_get_arg2(icall), ipc_get_arg3(icall),
	    ipc_get_arg4(icall));

	(void) icall;
	(void) arg;

	svc_id = ipc_get_arg2(icall);
	wnd_id = ipc_get_arg3(icall);

	if (svc_id != 0) {
		/* Create client object */
		rc = ds_client_create(disp, &display_client_cb, &srv, &client);
		if (rc != EOK) {
			async_answer_0(icall, ENOMEM);
			return;
		}

		/* Set up protocol structure */
		display_srv_initialize(&srv);
		srv.ops = &display_srv_ops;
		srv.arg = client;

		/* Handle connection */
		display_conn(icall, &srv);

		ds_client_destroy(client);
	} else {
		/* Window GC connection */

		wnd = ds_display_find_window(disp, wnd_id);
		if (wnd == NULL) {
			async_answer_0(icall, ENOENT);
			return;
		}

		/*
		 * XXX We need a way to make sure that the connection does
		 * not stay active after the window had been destroyed
		 */
		gc = ds_window_get_ctx(wnd);
		gc_conn(icall, gc);
	}
}

int main(int argc, char *argv[])
{
	errno_t rc;
	ds_output_t *output;

	printf("%s: Display server\n", NAME);

	if (log_init(NAME) != EOK) {
		printf(NAME ": Failed to initialize logging.\n");
		return 1;
	}

	rc = display_srv_init(&output);
	if (rc != EOK)
		return 1;

	printf(NAME ": Accepting connections.\n");
	task_retval(0);
	async_manager();

	return 0;
}

/** @}
 */
