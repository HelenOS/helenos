/*
 * Copyright (c) 2012 Vojtech Horky
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

/** @addtogroup remcons
 * @{
 */
/** @file
 */

#include <async.h>
#include <errno.h>
#include <io/con_srv.h>
#include <stdio.h>
#include <stdlib.h>
#include <str_error.h>
#include <loc.h>
#include <io/keycode.h>
#include <align.h>
#include <fibril_synch.h>
#include <task.h>
#include <inet/addr.h>
#include <inet/endpoint.h>
#include <inet/tcp.h>
#include <io/console.h>
#include <inttypes.h>
#include <str.h>
#include "telnet.h"
#include "user.h"

#define APP_GETTERM  "/app/getterm"
#define APP_SHELL "/app/bdsh"

/** Telnet commands to force character mode
 * (redundant to be on the safe side).
 * See
 * http://stackoverflow.com/questions/273261/force-telnet-user-into-character-mode
 * for discussion.
 */
static const telnet_cmd_t telnet_force_character_mode_command[] = {
	TELNET_IAC, TELNET_WILL, TELNET_ECHO,
	TELNET_IAC, TELNET_WILL, TELNET_SUPPRESS_GO_AHEAD,
	TELNET_IAC, TELNET_WONT, TELNET_LINEMODE
};

static const size_t telnet_force_character_mode_command_count =
    sizeof(telnet_force_character_mode_command) / sizeof(telnet_cmd_t);

static errno_t remcons_open(con_srvs_t *, con_srv_t *);
static errno_t remcons_close(con_srv_t *);
static errno_t remcons_write(con_srv_t *, void *, size_t, size_t *);
static void remcons_sync(con_srv_t *);
static void remcons_clear(con_srv_t *);
static void remcons_set_pos(con_srv_t *, sysarg_t col, sysarg_t row);
static errno_t remcons_get_pos(con_srv_t *, sysarg_t *, sysarg_t *);
static errno_t remcons_get_size(con_srv_t *, sysarg_t *, sysarg_t *);
static errno_t remcons_get_color_cap(con_srv_t *, console_caps_t *);
static errno_t remcons_get_event(con_srv_t *, cons_event_t *);

static con_ops_t con_ops = {
	.open = remcons_open,
	.close = remcons_close,
	.read = NULL,
	.write = remcons_write,
	.sync = remcons_sync,
	.clear = remcons_clear,
	.set_pos = remcons_set_pos,
	.get_pos = remcons_get_pos,
	.get_size = remcons_get_size,
	.get_color_cap = remcons_get_color_cap,
	.set_style = NULL,
	.set_color = NULL,
	.set_rgb_color = NULL,
	.set_cursor_visibility = NULL,
	.get_event = remcons_get_event
};

static void remcons_new_conn(tcp_listener_t *lst, tcp_conn_t *conn);

static tcp_listen_cb_t listen_cb = {
	.new_conn = remcons_new_conn
};

static tcp_cb_t conn_cb = {
	.connected = NULL
};

static telnet_user_t *srv_to_user(con_srv_t *srv)
{
	return srv->srvs->sarg;
}

static errno_t remcons_open(con_srvs_t *srvs, con_srv_t *srv)
{
	telnet_user_t *user = srv_to_user(srv);

	telnet_user_log(user, "New client connected (%p).", srv);

	/* Force character mode. */
	(void) tcp_conn_send(user->conn, (void *)telnet_force_character_mode_command,
	    telnet_force_character_mode_command_count);

	return EOK;
}

static errno_t remcons_close(con_srv_t *srv)
{
	telnet_user_t *user = srv_to_user(srv);

	telnet_user_notify_client_disconnected(user);
	telnet_user_log(user, "Client disconnected (%p).", srv);

	return EOK;
}

static errno_t remcons_write(con_srv_t *srv, void *data, size_t size, size_t *nwritten)
{
	telnet_user_t *user = srv_to_user(srv);
	errno_t rc;

	rc = telnet_user_send_data(user, data, size);
	if (rc != EOK)
		return rc;

	*nwritten = size;
	return EOK;
}

static void remcons_sync(con_srv_t *srv)
{
	(void) srv;
}

static void remcons_clear(con_srv_t *srv)
{
	(void) srv;
}

static void remcons_set_pos(con_srv_t *srv, sysarg_t col, sysarg_t row)
{
	telnet_user_t *user = srv_to_user(srv);

	telnet_user_update_cursor_x(user, col);
}

static errno_t remcons_get_pos(con_srv_t *srv, sysarg_t *col, sysarg_t *row)
{
	telnet_user_t *user = srv_to_user(srv);

	*col = user->cursor_x;
	*row = 0;

	return EOK;
}

static errno_t remcons_get_size(con_srv_t *srv, sysarg_t *cols, sysarg_t *rows)
{
	(void) srv;

	*cols = 100;
	*rows = 1;

	return EOK;
}

static errno_t remcons_get_color_cap(con_srv_t *srv, console_caps_t *ccaps)
{
	(void) srv;
	*ccaps = CONSOLE_CAP_NONE;

	return EOK;
}

static errno_t remcons_get_event(con_srv_t *srv, cons_event_t *event)
{
	telnet_user_t *user = srv_to_user(srv);
	kbd_event_t kevent;
	errno_t rc;

	rc = telnet_user_get_next_keyboard_event(user, &kevent);
	if (rc != EOK) {
		/* XXX What? */
		memset(event, 0, sizeof(*event));
		return EOK;
	}

	event->type = CEV_KEY;
	event->ev.key = kevent;

	return EOK;
}

/** Callback when client connects to a telnet terminal. */
static void client_connection(ipc_callid_t iid, ipc_call_t *icall, void *arg)
{
	/* Find the user. */
	telnet_user_t *user = telnet_user_get_for_client_connection(IPC_GET_ARG2(*icall));
	if (user == NULL) {
		async_answer_0(iid, ENOENT);
		return;
	}

	/* Handle messages. */
	con_conn(iid, icall, &user->srvs);
}

/** Fibril for spawning the task running after user connects.
 *
 * @param arg Corresponding @c telnet_user_t structure.
 */
static errno_t spawn_task_fibril(void *arg)
{
	telnet_user_t *user = arg;

	task_id_t task;
	task_wait_t wait;
	errno_t rc = task_spawnl(&task, &wait, APP_GETTERM, APP_GETTERM, user->service_name,
	    "/loc", "--msg", "--", APP_SHELL, NULL);
	if (rc != EOK) {
		telnet_user_error(user, "Spawning `%s %s /loc --msg -- %s' "
		    "failed: %s.", APP_GETTERM, user->service_name, APP_SHELL,
		    str_error(rc));
		fibril_mutex_lock(&user->guard);
		user->task_finished = true;
		user->srvs.aborted = true;
		fibril_condvar_signal(&user->refcount_cv);
		fibril_mutex_unlock(&user->guard);
		return EOK;
	}

	fibril_mutex_lock(&user->guard);
	user->task_id = task;
	fibril_mutex_unlock(&user->guard);

	task_exit_t task_exit;
	int task_retval;
	task_wait(&wait, &task_exit, &task_retval);
	telnet_user_log(user, "%s terminated %s, exit code %d.", APP_GETTERM,
	    task_exit == TASK_EXIT_NORMAL ? "normally" : "unexpectedly",
	    task_retval);

	/* Announce destruction. */
	fibril_mutex_lock(&user->guard);
	user->task_finished = true;
	user->srvs.aborted = true;
	fibril_condvar_signal(&user->refcount_cv);
	fibril_mutex_unlock(&user->guard);

	return EOK;
}

/** Tell whether given user can be destroyed (has no active clients).
 *
 * @param user The telnet user in question.
 */
static bool user_can_be_destroyed_no_lock(telnet_user_t *user)
{
	return user->task_finished && user->socket_closed &&
	    (user->locsrv_connection_count == 0);
}

/** Handle network connection.
 *
 * @param lst  Listener
 * @param conn Connection
 */
static void remcons_new_conn(tcp_listener_t *lst, tcp_conn_t *conn)
{
	telnet_user_t *user = telnet_user_create(conn);
	assert(user);

	con_srvs_init(&user->srvs);
	user->srvs.ops = &con_ops;
	user->srvs.sarg = user;
	user->srvs.abort_timeout = 1000;

	telnet_user_add(user);

	errno_t rc = loc_service_register(user->service_name, &user->service_id);
	if (rc != EOK) {
		telnet_user_error(user, "Unable to register %s with loc: %s.",
		    user->service_name, str_error(rc));
		return;
	}

	telnet_user_log(user, "Service %s registerd with id %" PRIun ".",
	    user->service_name, user->service_id);

	fid_t spawn_fibril = fibril_create(spawn_task_fibril, user);
	assert(spawn_fibril);
	fibril_add_ready(spawn_fibril);

	/* Wait for all clients to exit. */
	fibril_mutex_lock(&user->guard);
	while (!user_can_be_destroyed_no_lock(user)) {
		if (user->task_finished) {
			user->conn = NULL;
			user->socket_closed = true;
			user->srvs.aborted = true;
			continue;
		} else if (user->socket_closed) {
			if (user->task_id != 0) {
				task_kill(user->task_id);
			}
		}
		fibril_condvar_wait_timeout(&user->refcount_cv, &user->guard, 1000);
	}
	fibril_mutex_unlock(&user->guard);

	rc = loc_service_unregister(user->service_id);
	if (rc != EOK) {
		telnet_user_error(user,
		    "Unable to unregister %s from loc: %s (ignored).",
		    user->service_name, str_error(rc));
	}

	telnet_user_log(user, "Destroying...");
	telnet_user_destroy(user);
}

int main(int argc, char *argv[])
{
	errno_t rc;
	tcp_listener_t *lst;
	tcp_t *tcp;
	inet_ep_t ep;

	async_set_fallback_port_handler(client_connection, NULL);
	rc = loc_server_register(NAME);
	if (rc != EOK) {
		fprintf(stderr, "%s: Unable to register server\n", NAME);
		return rc;
	}

	rc = tcp_create(&tcp);
	if (rc != EOK) {
		fprintf(stderr, "%s: Error initializing TCP.\n", NAME);
		return rc;
	}

	inet_ep_init(&ep);
	ep.port = 2223;

	rc = tcp_listener_create(tcp, &ep, &listen_cb, NULL, &conn_cb, NULL,
	    &lst);
	if (rc != EOK) {
		fprintf(stderr, "%s: Error creating listener.\n", NAME);
		return rc;
	}

	printf("%s: HelenOS Remote console service\n", NAME);
	task_retval(0);
	async_manager();

	/* Not reached */
	return 0;
}

/** @}
 */
