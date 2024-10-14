/*
 * Copyright (c) 2024 Jiri Svoboda
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
#include <dispcfg_srv.h>
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
#include <wndmgt_srv.h>
#include "cfgclient.h"
#include "cfgops.h"
#include "client.h"
#include "display.h"
#include "dsops.h"
#include "ievent.h"
#include "input.h"
#include "main.h"
#include "output.h"
#include "seat.h"
#include "window.h"
#include "wmclient.h"
#include "wmops.h"

const char *cfg_file_path = "/w/cfg/display.sif";

static void display_client_conn(ipc_call_t *, void *);
static void display_client_ev_pending(void *);
static void display_wmclient_ev_pending(void *);
static void display_cfgclient_ev_pending(void *);
static void display_gc_conn(ipc_call_t *, void *);
static void display_wndmgt_conn(ipc_call_t *, void *);
static void display_dispcfg_conn(ipc_call_t *, void *);

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

static ds_wmclient_cb_t display_wmclient_cb = {
	.ev_pending = display_wmclient_ev_pending
};

static ds_cfgclient_cb_t display_cfgclient_cb = {
	.ev_pending = display_cfgclient_ev_pending
};

static void display_client_ev_pending(void *arg)
{
	display_srv_t *srv = (display_srv_t *) arg;

	display_srv_ev_pending(srv);
}

static void display_wmclient_ev_pending(void *arg)
{
	wndmgt_srv_t *srv = (wndmgt_srv_t *) arg;

	wndmgt_srv_ev_pending(srv);
}

static void display_cfgclient_ev_pending(void *arg)
{
	dispcfg_srv_t *srv = (dispcfg_srv_t *) arg;

	dispcfg_srv_ev_pending(srv);
}

/** Initialize display server */
static errno_t display_srv_init(ds_output_t **routput)
{
	ds_display_t *disp = NULL;
	ds_seat_t *seat = NULL;
	ds_output_t *output = NULL;
	gfx_context_t *gc = NULL;
	port_id_t disp_port;
	port_id_t gc_port;
	port_id_t wm_port;
	port_id_t dc_port;
	loc_srv_t *srv = NULL;
	service_id_t sid = 0;
	errno_t rc;

	log_msg(LOG_DEFAULT, LVL_DEBUG, "display_srv_init()");

	rc = ds_display_create(NULL, disp_flags, &disp);
	if (rc != EOK)
		goto error;

	rc = ds_display_load_cfg(disp, cfg_file_path);
	if (rc != EOK) {
		log_msg(LOG_DEFAULT, LVL_NOTE,
		    "Starting with fresh configuration.");

		/* Create first seat */
		rc = ds_seat_create(disp, "Alice", &seat);
		if (rc != EOK)
			goto error;
	}

	rc = ds_output_create(&output);
	if (rc != EOK)
		goto error;

	output->def_display = disp;
	rc = ds_output_start_discovery(output);
	if (rc != EOK)
		goto error;

	rc = ds_ievent_init(disp);
	if (rc != EOK)
		goto error;

	rc = ds_input_open(disp);
	if (rc != EOK)
		goto error;

	rc = async_create_port(INTERFACE_DISPLAY, display_client_conn, disp,
	    &disp_port);
	if (rc != EOK)
		goto error;

	rc = async_create_port(INTERFACE_GC, display_gc_conn, disp, &gc_port);
	if (rc != EOK)
		goto error;

	rc = async_create_port(INTERFACE_WNDMGT, display_wndmgt_conn, disp,
	    &wm_port);
	if (rc != EOK)
		goto error;

	rc = async_create_port(INTERFACE_DISPCFG, display_dispcfg_conn, disp,
	    &dc_port);
	if (rc != EOK)
		goto error;

	rc = loc_server_register(NAME, &srv);
	if (rc != EOK) {
		log_msg(LOG_DEFAULT, LVL_ERROR, "Failed registering server: %s.", str_error(rc));
		rc = EEXIST;
		goto error;
	}

	rc = loc_service_register(srv, SERVICE_NAME_DISPLAY, &sid);
	if (rc != EOK) {
		log_msg(LOG_DEFAULT, LVL_ERROR, "Failed registering service: %s.", str_error(rc));
		rc = EEXIST;
		goto error;
	}

	*routput = output;
	return EOK;
error:
	if (sid != 0)
		loc_service_unregister(srv, sid);
	if (srv != NULL)
		loc_server_unregister(srv);
	// XXX destroy disp_port
	// XXX destroy gc_port
	// XXX destroy wm_port
	// XXX destroy dc_port
#if 0
	if (disp->input != NULL)
		ds_input_close(disp);
#endif
	ds_ievent_fini(disp);
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
	sysarg_t svc_id;
	ds_client_t *client = NULL;
	ds_display_t *disp = (ds_display_t *) arg;
	errno_t rc;

	log_msg(LOG_DEFAULT, LVL_DEBUG, "display_client_conn arg1=%zu arg2=%zu arg3=%zu arg4=%zu.",
	    ipc_get_arg1(icall), ipc_get_arg2(icall), ipc_get_arg3(icall),
	    ipc_get_arg4(icall));

	svc_id = ipc_get_arg2(icall);

	if (svc_id != 0) {
		/* Create client object */
		ds_display_lock(disp);
		rc = ds_client_create(disp, &display_client_cb, &srv, &client);
		ds_display_unlock(disp);
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

		ds_display_lock(disp);
		ds_client_destroy(client);
		ds_display_unlock(disp);
	}
}

/** Handle GC connection to display server */
static void display_gc_conn(ipc_call_t *icall, void *arg)
{
	sysarg_t wnd_id;
	ds_window_t *wnd;
	ds_display_t *disp = (ds_display_t *) arg;
	gfx_context_t *gc;

	log_msg(LOG_DEFAULT, LVL_DEBUG, "display_gc_conn arg1=%zu arg2=%zu arg3=%zu arg4=%zu.",
	    ipc_get_arg1(icall), ipc_get_arg2(icall), ipc_get_arg3(icall),
	    ipc_get_arg4(icall));

	wnd_id = ipc_get_arg3(icall);

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

/** Handle window management connection to display server */
static void display_wndmgt_conn(ipc_call_t *icall, void *arg)
{
	ds_display_t *disp = (ds_display_t *) arg;
	errno_t rc;
	wndmgt_srv_t srv;
	ds_wmclient_t *wmclient = NULL;

	/* Create WM client object */
	ds_display_lock(disp);
	rc = ds_wmclient_create(disp, &display_wmclient_cb, &srv, &wmclient);
	ds_display_unlock(disp);
	if (rc != EOK) {
		async_answer_0(icall, ENOMEM);
		return;
	}

	/* Set up protocol structure */
	wndmgt_srv_initialize(&srv);
	srv.ops = &wndmgt_srv_ops;
	srv.arg = wmclient;

	/* Handle connection */
	wndmgt_conn(icall, &srv);

	ds_display_lock(disp);
	ds_wmclient_destroy(wmclient);
	ds_display_unlock(disp);
}

/** Handle configuration connection to display server */
static void display_dispcfg_conn(ipc_call_t *icall, void *arg)
{
	ds_display_t *disp = (ds_display_t *) arg;
	errno_t rc;
	dispcfg_srv_t srv;
	ds_cfgclient_t *cfgclient = NULL;

	/* Create CFG client object */
	ds_display_lock(disp);
	rc = ds_cfgclient_create(disp, &display_cfgclient_cb, &srv, &cfgclient);
	ds_display_unlock(disp);
	if (rc != EOK) {
		async_answer_0(icall, ENOMEM);
		return;
	}

	/* Set up protocol structure */
	dispcfg_srv_initialize(&srv);
	srv.ops = &dispcfg_srv_ops;
	srv.arg = cfgclient;

	/* Handle connection */
	dispcfg_conn(icall, &srv);

	ds_display_lock(disp);
	ds_cfgclient_destroy(cfgclient);
	ds_display_unlock(disp);
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
