/*
 * Copyright (c) 2024 Jiri Svoboda
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

#include <adt/list.h>
#include <as.h>
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
#include <vt/vt100.h>
#include "telnet.h"
#include "user.h"
#include "remcons.h"

#define APP_GETTERM  "/app/getterm"
#define APP_SHELL "/app/bdsh"

#define DEF_PORT 2223

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
static errno_t remcons_read(con_srv_t *, void *, size_t, size_t *);
static errno_t remcons_write(con_srv_t *, void *, size_t, size_t *);
static void remcons_sync(con_srv_t *);
static void remcons_clear(con_srv_t *);
static void remcons_set_pos(con_srv_t *, sysarg_t col, sysarg_t row);
static errno_t remcons_get_pos(con_srv_t *, sysarg_t *, sysarg_t *);
static errno_t remcons_get_size(con_srv_t *, sysarg_t *, sysarg_t *);
static errno_t remcons_get_color_cap(con_srv_t *, console_caps_t *);
static void remcons_set_style(con_srv_t *, console_style_t);
static void remcons_set_color(con_srv_t *, console_color_t,
    console_color_t, console_color_attr_t);
static void remcons_set_color(con_srv_t *, console_color_t,
    console_color_t, console_color_attr_t);
static void remcons_set_rgb_color(con_srv_t *, pixel_t, pixel_t);
static void remcons_cursor_visibility(con_srv_t *, bool);
static errno_t remcons_set_caption(con_srv_t *, const char *);
static errno_t remcons_get_event(con_srv_t *, cons_event_t *);
static errno_t remcons_map(con_srv_t *, sysarg_t, sysarg_t, charfield_t **);
static void remcons_unmap(con_srv_t *);
static void remcons_update(con_srv_t *, sysarg_t, sysarg_t, sysarg_t,
    sysarg_t);

static con_ops_t con_ops = {
	.open = remcons_open,
	.close = remcons_close,
	.read = remcons_read,
	.write = remcons_write,
	.sync = remcons_sync,
	.clear = remcons_clear,
	.set_pos = remcons_set_pos,
	.get_pos = remcons_get_pos,
	.get_size = remcons_get_size,
	.get_color_cap = remcons_get_color_cap,
	.set_style = remcons_set_style,
	.set_color = remcons_set_color,
	.set_rgb_color = remcons_set_rgb_color,
	.set_cursor_visibility = remcons_cursor_visibility,
	.set_caption = remcons_set_caption,
	.get_event = remcons_get_event,
	.map = remcons_map,
	.unmap = remcons_unmap,
	.update = remcons_update
};

static void remcons_vt_putchar(void *, char32_t);
static void remcons_vt_cputs(void *, const char *);
static void remcons_vt_flush(void *);
static void remcons_vt_key(void *, keymod_t, keycode_t, char);
static void remcons_vt_pos_event(void *, pos_event_t *);

static vt100_cb_t remcons_vt_cb = {
	.putuchar = remcons_vt_putchar,
	.control_puts = remcons_vt_cputs,
	.flush = remcons_vt_flush,
	.key = remcons_vt_key,
	.pos_event = remcons_vt_pos_event
};

static void remcons_new_conn(tcp_listener_t *lst, tcp_conn_t *conn);

static tcp_listen_cb_t listen_cb = {
	.new_conn = remcons_new_conn
};

static tcp_cb_t conn_cb = {
	.connected = NULL
};

static void remcons_telnet_ws_update(void *, unsigned, unsigned);

static telnet_cb_t remcons_telnet_cb = {
	.ws_update = remcons_telnet_ws_update
};

static loc_srv_t *remcons_srv;
static bool no_ctl;
static bool no_rgb;

static telnet_user_t *srv_to_user(con_srv_t *srv)
{
	remcons_t *remcons = (remcons_t *)srv->srvs->sarg;
	return remcons->user;
}

static remcons_t *srv_to_remcons(con_srv_t *srv)
{
	remcons_t *remcons = (remcons_t *)srv->srvs->sarg;
	return remcons;
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

static errno_t remcons_read(con_srv_t *srv, void *data, size_t size,
    size_t *nread)
{
	telnet_user_t *user = srv_to_user(srv);
	errno_t rc;

	rc = telnet_user_recv(user, data, size, nread);
	if (rc != EOK)
		return rc;

	return EOK;
}

static errno_t remcons_write(con_srv_t *srv, void *data, size_t size, size_t *nwritten)
{
	remcons_t *remcons = srv_to_remcons(srv);
	errno_t rc;

	rc = telnet_user_send_data(remcons->user, data, size);
	if (rc != EOK)
		return rc;

	rc = telnet_user_flush(remcons->user);
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
	remcons_t *remcons = srv_to_remcons(srv);

	if (remcons->enable_ctl) {
		vt100_cls(remcons->vt);
		vt100_set_pos(remcons->vt, 0, 0);
		remcons->user->cursor_x = 0;
		remcons->user->cursor_y = 0;
	}
}

static void remcons_set_pos(con_srv_t *srv, sysarg_t col, sysarg_t row)
{
	remcons_t *remcons = srv_to_remcons(srv);
	telnet_user_t *user = srv_to_user(srv);

	if (remcons->enable_ctl) {
		vt100_set_pos(remcons->vt, col, row);
		remcons->user->cursor_x = col;
		remcons->user->cursor_y = row;
		(void)telnet_user_flush(remcons->user);
	} else {
		telnet_user_update_cursor_x(user, col);
	}
}

static errno_t remcons_get_pos(con_srv_t *srv, sysarg_t *col, sysarg_t *row)
{
	telnet_user_t *user = srv_to_user(srv);

	*col = user->cursor_x;
	*row = user->cursor_y;

	return EOK;
}

static errno_t remcons_get_size(con_srv_t *srv, sysarg_t *cols, sysarg_t *rows)
{
	remcons_t *remcons = srv_to_remcons(srv);

	if (remcons->enable_ctl) {
		*cols = remcons->vt->cols;
		*rows = remcons->vt->rows;
	} else {
		*cols = 100;
		*rows = 1;
	}

	return EOK;
}

static errno_t remcons_get_color_cap(con_srv_t *srv, console_caps_t *ccaps)
{
	remcons_t *remcons = srv_to_remcons(srv);

	*ccaps = 0;

	if (remcons->enable_ctl) {
		*ccaps |= CONSOLE_CAP_CURSORCTL | CONSOLE_CAP_STYLE |
		    CONSOLE_CAP_INDEXED;
	}

	if (remcons->enable_rgb)
		*ccaps |= CONSOLE_CAP_RGB;

	return EOK;
}

static void remcons_set_style(con_srv_t *srv, console_style_t style)
{
	remcons_t *remcons = srv_to_remcons(srv);
	char_attrs_t attrs;

	if (remcons->enable_ctl) {
		attrs.type = CHAR_ATTR_STYLE;
		attrs.val.style = style;
		vt100_set_attr(remcons->vt, attrs);
	}
}

static void remcons_set_color(con_srv_t *srv, console_color_t bgcolor,
    console_color_t fgcolor, console_color_attr_t flags)
{
	remcons_t *remcons = srv_to_remcons(srv);
	char_attrs_t attrs;

	if (remcons->enable_ctl) {
		attrs.type = CHAR_ATTR_INDEX;
		attrs.val.index.bgcolor = bgcolor;
		attrs.val.index.fgcolor = fgcolor;
		attrs.val.index.attr = flags;
		vt100_set_attr(remcons->vt, attrs);
	}
}

static void remcons_set_rgb_color(con_srv_t *srv, pixel_t bgcolor,
    pixel_t fgcolor)
{
	remcons_t *remcons = srv_to_remcons(srv);
	char_attrs_t attrs;

	if (remcons->enable_ctl) {
		attrs.type = CHAR_ATTR_RGB;
		attrs.val.rgb.bgcolor = bgcolor;
		attrs.val.rgb.fgcolor = fgcolor;
		vt100_set_attr(remcons->vt, attrs);
	}
}

static void remcons_cursor_visibility(con_srv_t *srv, bool visible)
{
	remcons_t *remcons = srv_to_remcons(srv);

	if (remcons->enable_ctl) {
		if (!remcons->curs_visible && visible) {
			vt100_set_pos(remcons->vt, remcons->user->cursor_x,
			    remcons->user->cursor_y);
		}
		vt100_cursor_visibility(remcons->vt, visible);
	}

	remcons->curs_visible = visible;
}

static errno_t remcons_set_caption(con_srv_t *srv, const char *caption)
{
	remcons_t *remcons = srv_to_remcons(srv);

	if (remcons->enable_ctl) {
		vt100_set_title(remcons->vt, caption);
	}

	return EOK;
}

/** Creates new keyboard event from given char.
 *
 * @param type Event type (press / release).
 * @param c Pressed character.
 */
static remcons_event_t *new_kbd_event(kbd_event_type_t type, keymod_t mods,
    keycode_t key, char c)
{
	remcons_event_t *event = malloc(sizeof(remcons_event_t));
	if (event == NULL) {
		fprintf(stderr, "Out of memory.\n");
		return NULL;
	}

	link_initialize(&event->link);
	event->cev.type = CEV_KEY;
	event->cev.ev.key.type = type;
	event->cev.ev.key.mods = mods;
	event->cev.ev.key.key = key;
	event->cev.ev.key.c = c;

	return event;
}

/** Creates new position event.
 *
 * @param ev Position event.
 * @param c Pressed character.
 */
static remcons_event_t *new_pos_event(pos_event_t *ev)
{
	remcons_event_t *event = malloc(sizeof(remcons_event_t));
	if (event == NULL) {
		fprintf(stderr, "Out of memory.\n");
		return NULL;
	}

	link_initialize(&event->link);
	event->cev.type = CEV_POS;
	event->cev.ev.pos = *ev;

	return event;
}

/** Creates new console resize event.
 */
static remcons_event_t *new_resize_event(void)
{
	remcons_event_t *event = malloc(sizeof(remcons_event_t));
	if (event == NULL) {
		fprintf(stderr, "Out of memory.\n");
		return NULL;
	}

	link_initialize(&event->link);
	event->cev.type = CEV_RESIZE;

	return event;
}

static errno_t remcons_get_event(con_srv_t *srv, cons_event_t *event)
{
	remcons_t *remcons = srv_to_remcons(srv);
	telnet_user_t *user = srv_to_user(srv);
	size_t nread;

	while (list_empty(&remcons->in_events)) {
		char next_byte = 0;

		errno_t rc = telnet_user_recv(user, &next_byte, 1,
		    &nread);
		if (rc != EOK)
			return rc;

		vt100_rcvd_char(remcons->vt, next_byte);
	}

	link_t *link = list_first(&remcons->in_events);
	list_remove(link);

	remcons_event_t *tmp = list_get_instance(link, remcons_event_t, link);

	*event = tmp->cev;
	free(tmp);

	return EOK;
}

static errno_t remcons_map(con_srv_t *srv, sysarg_t cols, sysarg_t rows,
    charfield_t **rbuf)
{
	remcons_t *remcons = srv_to_remcons(srv);
	void *buf;

	if (!remcons->enable_ctl)
		return ENOTSUP;

	if (remcons->ubuf != NULL)
		return EBUSY;

	buf = as_area_create(AS_AREA_ANY, cols * rows * sizeof(charfield_t),
	    AS_AREA_READ | AS_AREA_WRITE | AS_AREA_CACHEABLE, AS_AREA_UNPAGED);
	if (buf == AS_MAP_FAILED)
		return ENOMEM;

	remcons->ucols = cols;
	remcons->urows = rows;
	remcons->ubuf = buf;

	*rbuf = buf;
	return EOK;

}

static void remcons_unmap(con_srv_t *srv)
{
	remcons_t *remcons = srv_to_remcons(srv);
	void *buf;

	buf = remcons->ubuf;
	remcons->ubuf = NULL;

	if (buf != NULL)
		as_area_destroy(buf);
}

static void remcons_update(con_srv_t *srv, sysarg_t c0, sysarg_t r0,
    sysarg_t c1, sysarg_t r1)
{
	remcons_t *remcons = srv_to_remcons(srv);
	charfield_t *ch;
	sysarg_t col, row;
	sysarg_t old_x, old_y;

	if (remcons->ubuf == NULL)
		return;

	/* Make sure we have meaningful coordinates, within bounds */

	if (c1 > remcons->ucols)
		c1 = remcons->ucols;
	if (c1 > remcons->user->cols)
		c1 = remcons->user->cols;
	if (c0 >= c1)
		return;

	if (r1 > remcons->urows)
		r1 = remcons->urows;
	if (r1 > remcons->user->rows)
		r1 = remcons->user->rows;
	if (r0 >= r1)
		return;

	/* Update screen from user buffer */

	old_x = remcons->user->cursor_x;
	old_y = remcons->user->cursor_y;

	if (remcons->curs_visible)
		vt100_cursor_visibility(remcons->vt, false);

	for (row = r0; row < r1; row++) {
		for (col = c0; col < c1; col++) {
			vt100_set_pos(remcons->vt, col, row);
			ch = &remcons->ubuf[row * remcons->ucols + col];
			vt100_set_attr(remcons->vt, ch->attrs);
			vt100_putuchar(remcons->vt, ch->ch);
		}
	}

	if (remcons->curs_visible) {
		old_x = remcons->user->cursor_x = old_x;
		remcons->user->cursor_y = old_y;
		vt100_set_pos(remcons->vt, old_x, old_y);
		vt100_cursor_visibility(remcons->vt, true);
	}

	/* Flush data */
	(void)telnet_user_flush(remcons->user);
}

/** Callback when client connects to a telnet terminal. */
static void client_connection(ipc_call_t *icall, void *arg)
{
	/* Find the user. */
	telnet_user_t *user = telnet_user_get_for_client_connection(ipc_get_arg2(icall));
	if (user == NULL) {
		async_answer_0(icall, ENOENT);
		return;
	}

	/* Handle messages. */
	con_conn(icall, &user->srvs);
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
		fibril_mutex_lock(&user->recv_lock);
		user->task_finished = true;
		user->srvs.aborted = true;
		fibril_condvar_signal(&user->refcount_cv);
		fibril_mutex_unlock(&user->recv_lock);
		return EOK;
	}

	fibril_mutex_lock(&user->recv_lock);
	user->task_id = task;
	fibril_mutex_unlock(&user->recv_lock);

	task_exit_t task_exit;
	int task_retval;
	task_wait(&wait, &task_exit, &task_retval);
	telnet_user_log(user, "%s terminated %s, exit code %d.", APP_GETTERM,
	    task_exit == TASK_EXIT_NORMAL ? "normally" : "unexpectedly",
	    task_retval);

	/* Announce destruction. */
	fibril_mutex_lock(&user->recv_lock);
	user->task_finished = true;
	user->srvs.aborted = true;
	fibril_condvar_signal(&user->refcount_cv);
	fibril_mutex_unlock(&user->recv_lock);

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

static void remcons_vt_putchar(void *arg, char32_t c)
{
	remcons_t *remcons = (remcons_t *)arg;
	char buf[STR_BOUNDS(1)];
	size_t off;
	errno_t rc;

	(void)arg;

	off = 0;
	rc = chr_encode(c, buf, &off, sizeof(buf));
	if (rc != EOK)
		return;

	(void)telnet_user_send_data(remcons->user, buf, off);
}

static void remcons_vt_cputs(void *arg, const char *str)
{
	remcons_t *remcons = (remcons_t *)arg;

	(void)telnet_user_send_raw(remcons->user, str, str_size(str));
}

static void remcons_vt_flush(void *arg)
{
	remcons_t *remcons = (remcons_t *)arg;
	(void)telnet_user_flush(remcons->user);
}

static void remcons_vt_key(void *arg, keymod_t mods, keycode_t key, char c)
{
	remcons_t *remcons = (remcons_t *)arg;

	remcons_event_t *down = new_kbd_event(KEY_PRESS, mods, key, c);
	if (down == NULL)
		return;

	remcons_event_t *up = new_kbd_event(KEY_RELEASE, mods, key, c);
	if (up == NULL) {
		free(down);
		return;
	}

	list_append(&down->link, &remcons->in_events);
	list_append(&up->link, &remcons->in_events);
}

static void remcons_vt_pos_event(void *arg, pos_event_t *ev)
{
	remcons_t *remcons = (remcons_t *)arg;

	remcons_event_t *cev = new_pos_event(ev);
	if (cev == NULL)
		return;

	list_append(&cev->link, &remcons->in_events);
}

/** Window size update callback.
 *
 * @param arg Argument (remcons_t *)
 * @param cols New number of columns
 * @param rows New number of rows
 */
static void remcons_telnet_ws_update(void *arg, unsigned cols, unsigned rows)
{
	remcons_t *remcons = (remcons_t *)arg;

	vt100_resize(remcons->vt, cols, rows);
	telnet_user_resize(remcons->user, cols, rows);

	remcons_event_t *resize = new_resize_event();
	if (resize == NULL)
		return;

	list_append(&resize->link, &remcons->in_events);
}

/** Handle network connection.
 *
 * @param lst  Listener
 * @param conn Connection
 */
static void remcons_new_conn(tcp_listener_t *lst, tcp_conn_t *conn)
{
	char_attrs_t attrs;
	remcons_t *remcons = NULL;
	telnet_user_t *user = NULL;

	remcons = calloc(1, sizeof(remcons_t));
	if (remcons == NULL) {
		fprintf(stderr, "Out of memory.\n");
		goto error;
	}

	user = telnet_user_create(conn, &remcons_telnet_cb,
	    (void *)remcons);
	if (user == NULL) {
		fprintf(stderr, "Out of memory.\n");
		goto error;
	}

	remcons->enable_ctl = !no_ctl;
	remcons->enable_rgb = !no_ctl && !no_rgb;
	remcons->user = user;
	list_initialize(&remcons->in_events);

	if (remcons->enable_ctl) {
		user->cols = 80;
		user->rows = 25;
	} else {
		user->cols = 100;
		user->rows = 1;
	}

	remcons->curs_visible = true;

	remcons->vt = vt100_create((void *)remcons, 80, 25, &remcons_vt_cb);
	if (remcons->vt == NULL) {
		fprintf(stderr, "Error creating VT100 driver instance.\n");
		goto error;
	}

	remcons->vt->enable_rgb = remcons->enable_rgb;

	if (remcons->enable_ctl) {
		attrs.type = CHAR_ATTR_STYLE;
		attrs.val.style = STYLE_NORMAL;
		vt100_set_sgr(remcons->vt, attrs);
		vt100_cls(remcons->vt);
		vt100_set_pos(remcons->vt, 0, 0);
		vt100_set_button_reporting(remcons->vt, true);
	}

	con_srvs_init(&user->srvs);
	user->srvs.ops = &con_ops;
	user->srvs.sarg = remcons;
	user->srvs.abort_timeout = 1000000;

	telnet_user_add(user);

	errno_t rc = loc_service_register(remcons_srv, user->service_name,
	    &user->service_id);
	if (rc != EOK) {
		telnet_user_error(user, "Unable to register %s with loc: %s.",
		    user->service_name, str_error(rc));
		goto error;
	}

	telnet_user_log(user, "Service %s registerd with id %" PRIun ".",
	    user->service_name, user->service_id);

	fid_t spawn_fibril = fibril_create(spawn_task_fibril, user);
	if (spawn_fibril == 0) {
		fprintf(stderr, "Failed creating fibril.\n");
		goto error;
	}
	fibril_add_ready(spawn_fibril);

	/* Wait for all clients to exit. */
	fibril_mutex_lock(&user->recv_lock);
	while (!user_can_be_destroyed_no_lock(user)) {
		if (user->task_finished) {
			user->socket_closed = true;
			user->srvs.aborted = true;
		} else if (user->socket_closed) {
			if (user->task_id != 0) {
				task_kill(user->task_id);
			}
		}
		fibril_condvar_wait_timeout(&user->refcount_cv,
		    &user->recv_lock, 1000000);
	}
	fibril_mutex_unlock(&user->recv_lock);

	rc = loc_service_unregister(remcons_srv, user->service_id);
	if (rc != EOK) {
		telnet_user_error(user,
		    "Unable to unregister %s from loc: %s (ignored).",
		    user->service_name, str_error(rc));
	}

	telnet_user_log(user, "Destroying...");

	if (remcons->enable_ctl) {
		/* Disable mouse tracking */
		vt100_set_button_reporting(remcons->vt, false);

		/* Reset all character attributes and clear screen */
		vt100_sgr(remcons->vt, 0);
		vt100_cls(remcons->vt);
		vt100_set_pos(remcons->vt, 0, 0);

		telnet_user_flush(user);
	}

	tcp_conn_send_fin(user->conn);
	user->conn = NULL;

	telnet_user_destroy(user);
	vt100_destroy(remcons->vt);
	free(remcons);
	return;
error:
	if (user != NULL && user->service_id != 0)
		loc_service_unregister(remcons_srv, user->service_id);
	if (user != NULL)
		free(user);
	if (remcons != NULL && remcons->vt != NULL)
		vt100_destroy(remcons->vt);
	if (remcons != NULL)
		free(remcons);
}

static void print_syntax(void)
{
	fprintf(stderr, "syntax: remcons [<options>]\n");
	fprintf(stderr, "\t--no-ctl      Disable all terminal control sequences\n");
	fprintf(stderr, "\t--no-rgb      Disable RGB colors\n");
	fprintf(stderr, "\t--port <port> Listening port (default: %u)\n",
	    DEF_PORT);
}

int main(int argc, char *argv[])
{
	errno_t rc;
	tcp_listener_t *lst;
	tcp_t *tcp;
	inet_ep_t ep;
	uint16_t port;
	int i;

	port = DEF_PORT;

	i = 1;
	while (i < argc) {
		if (argv[i][0] == '-') {
			if (str_cmp(argv[i], "--no-ctl") == 0) {
				no_ctl = true;
			} else if (str_cmp(argv[i], "--no-rgb") == 0) {
				no_rgb = true;
			} else if (str_cmp(argv[i], "--port") == 0) {
				++i;
				if (i >= argc) {
					fprintf(stderr, "Option argument "
					    "missing.\n");
					print_syntax();
					return EINVAL;
				}
				rc = str_uint16_t(argv[i], NULL, 10, true, &port);
				if (rc != EOK) {
					fprintf(stderr, "Invalid port number "
					    "'%s'.\n", argv[i]);
					print_syntax();
					return EINVAL;
				}
			} else {
				fprintf(stderr, "Unknown option '%s'.\n",
				    argv[i]);
				print_syntax();
				return EINVAL;
			}
		} else {
			fprintf(stderr, "Unexpected argument.\n");
			print_syntax();
			return EINVAL;
		}

		++i;
	}

	async_set_fallback_port_handler(client_connection, NULL);
	rc = loc_server_register(NAME, &remcons_srv);
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
	ep.port = port;

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
