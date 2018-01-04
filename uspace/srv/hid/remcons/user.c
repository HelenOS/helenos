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
#include <stdio.h>
#include <stdlib.h>
#include <adt/prodcons.h>
#include <errno.h>
#include <str_error.h>
#include <loc.h>
#include <io/keycode.h>
#include <align.h>
#include <as.h>
#include <fibril_synch.h>
#include <task.h>
#include <inet/tcp.h>
#include <io/console.h>
#include <inttypes.h>
#include <assert.h>
#include "user.h"
#include "telnet.h"

static FIBRIL_MUTEX_INITIALIZE(users_guard);
static LIST_INITIALIZE(users);

/** Create new telnet user.
 *
 * @param conn Incoming connection.
 * @return New telnet user or NULL when out of memory.
 */
telnet_user_t *telnet_user_create(tcp_conn_t *conn)
{
	static int telnet_user_id_counter = 0;

	telnet_user_t *user = malloc(sizeof(telnet_user_t));
	if (user == NULL) {
		return NULL;
	}

	user->id = ++telnet_user_id_counter;

	int rc = asprintf(&user->service_name, "%s/telnet%d", NAMESPACE, user->id);
	if (rc < 0) {
		free(user);
		return NULL;
	}

	user->conn = conn;
	user->service_id = (service_id_t) -1;
	prodcons_initialize(&user->in_events);
	link_initialize(&user->link);
	user->socket_buffer_len = 0;
	user->socket_buffer_pos = 0;

	fibril_condvar_initialize(&user->refcount_cv);
	fibril_mutex_initialize(&user->guard);
	user->task_finished = false;
	user->socket_closed = false;
	user->locsrv_connection_count = 0;

	user->cursor_x = 0;

	return user;
}

void telnet_user_add(telnet_user_t *user)
{
	fibril_mutex_lock(&users_guard);
	list_append(&user->link, &users);
	fibril_mutex_unlock(&users_guard);
}

/** Destroy telnet user structure.
 *
 * @param user User to be destroyed.
 */
void telnet_user_destroy(telnet_user_t *user)
{
	assert(user);

	fibril_mutex_lock(&users_guard);
	list_remove(&user->link);
	fibril_mutex_unlock(&users_guard);

	free(user);
}

/** Find user by service id and increments reference counter.
 *
 * @param id Location service id of the telnet user's terminal.
 */
telnet_user_t *telnet_user_get_for_client_connection(service_id_t id)
{
	telnet_user_t *user = NULL;

	fibril_mutex_lock(&users_guard);
	list_foreach(users, link, telnet_user_t, tmp) {
		if (tmp->service_id == id) {
			user = tmp;
			break;
		}
	}
	if (user == NULL) {
		fibril_mutex_unlock(&users_guard);
		return NULL;
	}

	telnet_user_t *tmp = user;
	fibril_mutex_lock(&tmp->guard);
	user->locsrv_connection_count++;

	/*
	 * Refuse to return user whose task already finished or when
	 * the socket is already closed().
	 */
	if (user->task_finished || user->socket_closed) {
		user = NULL;
		user->locsrv_connection_count--;
	}

	fibril_mutex_unlock(&tmp->guard);


	fibril_mutex_unlock(&users_guard);

	return user;
}

/** Notify that client disconnected from the remote terminal.
 *
 * @param user To which user the client was connected.
 */
void telnet_user_notify_client_disconnected(telnet_user_t *user)
{
	fibril_mutex_lock(&user->guard);
	assert(user->locsrv_connection_count > 0);
	user->locsrv_connection_count--;
	fibril_condvar_signal(&user->refcount_cv);
	fibril_mutex_unlock(&user->guard);
}

/** Tell whether the launched task already exited and socket is already closed.
 *
 * @param user Telnet user in question.
 */
bool telnet_user_is_zombie(telnet_user_t *user)
{
	fibril_mutex_lock(&user->guard);
	bool zombie = user->socket_closed || user->task_finished;
	fibril_mutex_unlock(&user->guard);

	return zombie;
}

/** Receive next byte from a socket (use buffering.
 * We need to return the value via extra argument because the read byte
 * might be negative.
 */
static errno_t telnet_user_recv_next_byte_no_lock(telnet_user_t *user, char *byte)
{
	/* No more buffered data? */
	if (user->socket_buffer_len <= user->socket_buffer_pos) {
		errno_t rc;
		size_t recv_length;

		rc = tcp_conn_recv_wait(user->conn, user->socket_buffer,
		    BUFFER_SIZE, &recv_length);
		if (rc != EOK)
			return rc;

		if (recv_length == 0) {
			user->socket_closed = true;
			user->srvs.aborted = true;
			return ENOENT;
		}

		user->socket_buffer_len = recv_length;
		user->socket_buffer_pos = 0;
	}

	*byte = user->socket_buffer[user->socket_buffer_pos++];

	return EOK;
}

/** Creates new keyboard event from given char.
 *
 * @param type Event type (press / release).
 * @param c Pressed character.
 */
static kbd_event_t* new_kbd_event(kbd_event_type_t type, wchar_t c) {
	kbd_event_t *event = malloc(sizeof(kbd_event_t));
	assert(event);

	link_initialize(&event->link);
	event->type = type;
	event->c = c;
	event->mods = 0;

	switch (c) {
	case '\n':
		event->key = KC_ENTER;
		break;
	case '\t':
		event->key = KC_TAB;
		break;
	case '\b':
	case 127: /* This is what Linux telnet sends. */
		event->key = KC_BACKSPACE;
		event->c = '\b';
		break;
	default:
		event->key = KC_A;
		break;
	}

	return event;
}

/** Process telnet command (currently only print to screen).
 *
 * @param user Telnet user structure.
 * @param option_code Command option code.
 * @param cmd Telnet command.
 */
static void process_telnet_command(telnet_user_t *user,
    telnet_cmd_t option_code, telnet_cmd_t cmd)
{
	if (option_code != 0) {
		telnet_user_log(user, "Ignoring telnet command %u %u %u.",
		    TELNET_IAC, option_code, cmd);
	} else {
		telnet_user_log(user, "Ignoring telnet command %u %u.",
		    TELNET_IAC, cmd);
	}
}

/** Get next keyboard event.
 *
 * @param user Telnet user.
 * @param event Where to store the keyboard event.
 * @return Error code.
 */
errno_t telnet_user_get_next_keyboard_event(telnet_user_t *user, kbd_event_t *event)
{
	fibril_mutex_lock(&user->guard);
	if (list_empty(&user->in_events.list)) {
		char next_byte = 0;
		bool inside_telnet_command = false;

		telnet_cmd_t telnet_option_code = 0;

		/* Skip zeros, bail-out on error. */
		while (next_byte == 0) {
			errno_t rc = telnet_user_recv_next_byte_no_lock(user, &next_byte);
			if (rc != EOK) {
				fibril_mutex_unlock(&user->guard);
				return rc;
			}
			uint8_t byte = (uint8_t) next_byte;

			/* Skip telnet commands. */
			if (inside_telnet_command) {
				inside_telnet_command = false;
				next_byte = 0;
				if (TELNET_IS_OPTION_CODE(byte)) {
					telnet_option_code = byte;
					inside_telnet_command = true;
				} else {
					process_telnet_command(user,
					    telnet_option_code, byte);
				}
			}
			if (byte == TELNET_IAC) {
				inside_telnet_command = true;
				next_byte = 0;
			}
		}

		/* CR-LF conversions. */
		if (next_byte == 13) {
			next_byte = 10;
		}

		kbd_event_t *down = new_kbd_event(KEY_PRESS, next_byte);
		kbd_event_t *up = new_kbd_event(KEY_RELEASE, next_byte);
		assert(down);
		assert(up);
		prodcons_produce(&user->in_events, &down->link);
		prodcons_produce(&user->in_events, &up->link);
	}

	link_t *link = prodcons_consume(&user->in_events);
	kbd_event_t *tmp = list_get_instance(link, kbd_event_t, link);

	fibril_mutex_unlock(&user->guard);

	*event = *tmp;

	free(tmp);

	return EOK;
}

/** Send data (convert them first) to the socket, no locking.
 *
 * @param user Telnet user.
 * @param data Data buffer (not zero terminated).
 * @param size Size of @p data buffer in bytes.
 */
static errno_t telnet_user_send_data_no_lock(telnet_user_t *user, uint8_t *data, size_t size)
{
	uint8_t *converted = malloc(3 * size + 1);
	assert(converted);
	int converted_size = 0;

	/* Convert new-lines. */
	for (size_t i = 0; i < size; i++) {
		if (data[i] == 10) {
			converted[converted_size++] = 13;
			converted[converted_size++] = 10;
			user->cursor_x = 0;
		} else {
			converted[converted_size++] = data[i];
			if (data[i] == '\b') {
				user->cursor_x--;
			} else {
				user->cursor_x++;
			}
		}
	}


	errno_t rc = tcp_conn_send(user->conn, converted, converted_size);
	free(converted);

	return rc;
}

/** Send data (convert them first) to the socket.
 *
 * @param user Telnet user.
 * @param data Data buffer (not zero terminated).
 * @param size Size of @p data buffer in bytes.
 */
errno_t telnet_user_send_data(telnet_user_t *user, uint8_t *data, size_t size)
{
	fibril_mutex_lock(&user->guard);

	errno_t rc = telnet_user_send_data_no_lock(user, data, size);

	fibril_mutex_unlock(&user->guard);

	return rc;
}

/** Update cursor X position.
 *
 * This call may result in sending control commands over socket.
 *
 * @param user Telnet user.
 * @param new_x New cursor location.
 */
void telnet_user_update_cursor_x(telnet_user_t *user, int new_x)
{
	fibril_mutex_lock(&user->guard);
	if (user->cursor_x - 1 == new_x) {
		uint8_t data = '\b';
		/* Ignore errors. */
		telnet_user_send_data_no_lock(user, &data, 1);
	}
	user->cursor_x = new_x;
	fibril_mutex_unlock(&user->guard);

}

/**
 * @}
 */
