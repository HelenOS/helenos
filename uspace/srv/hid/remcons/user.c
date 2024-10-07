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
#include <async.h>
#include <stdio.h>
#include <stdlib.h>
#include <adt/prodcons.h>
#include <errno.h>
#include <macros.h>
#include <mem.h>
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
#include "remcons.h"
#include "user.h"
#include "telnet.h"

static FIBRIL_MUTEX_INITIALIZE(users_guard);
static LIST_INITIALIZE(users);

static errno_t telnet_user_send_raw_locked(telnet_user_t *, const void *,
    size_t);
static errno_t telnet_user_flush_locked(telnet_user_t *);

/** Create new telnet user.
 *
 * @param conn Incoming connection.
 * @param cb Callback functions
 * @param arg Argument to callback functions
 * @return New telnet user or NULL when out of memory.
 */
telnet_user_t *telnet_user_create(tcp_conn_t *conn, telnet_cb_t *cb, void *arg)
{
	static int telnet_user_id_counter = 0;

	telnet_user_t *user = malloc(sizeof(telnet_user_t));
	if (user == NULL) {
		return NULL;
	}

	user->cb = cb;
	user->arg = arg;
	user->id = ++telnet_user_id_counter;

	int rc = asprintf(&user->service_name, "%s/telnet%u.%d", NAMESPACE,
	    (unsigned)task_get_id(), user->id);
	if (rc < 0) {
		free(user);
		return NULL;
	}

	user->conn = conn;
	user->service_id = (service_id_t) -1;
	link_initialize(&user->link);
	user->socket_buffer_len = 0;
	user->socket_buffer_pos = 0;
	user->send_buf_used = 0;

	fibril_condvar_initialize(&user->refcount_cv);
	fibril_mutex_initialize(&user->send_lock);
	fibril_mutex_initialize(&user->recv_lock);
	user->task_finished = false;
	user->socket_closed = false;
	user->locsrv_connection_count = 0;

	user->cursor_x = 0;
	user->cursor_y = 0;

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
	fibril_mutex_lock(&tmp->recv_lock);
	user->locsrv_connection_count++;

	/*
	 * Refuse to return user whose task already finished or when
	 * the socket is already closed().
	 */
	if (user->task_finished || user->socket_closed) {
		user = NULL;
		user->locsrv_connection_count--;
	}

	fibril_mutex_unlock(&tmp->recv_lock);

	fibril_mutex_unlock(&users_guard);

	return user;
}

/** Notify that client disconnected from the remote terminal.
 *
 * @param user To which user the client was connected.
 */
void telnet_user_notify_client_disconnected(telnet_user_t *user)
{
	fibril_mutex_lock(&user->recv_lock);
	assert(user->locsrv_connection_count > 0);
	user->locsrv_connection_count--;
	fibril_condvar_signal(&user->refcount_cv);
	fibril_mutex_unlock(&user->recv_lock);
}

/** Tell whether the launched task already exited and socket is already closed.
 *
 * @param user Telnet user in question.
 */
bool telnet_user_is_zombie(telnet_user_t *user)
{
	fibril_mutex_lock(&user->recv_lock);
	bool zombie = user->socket_closed || user->task_finished;
	fibril_mutex_unlock(&user->recv_lock);

	return zombie;
}

static errno_t telnet_user_fill_recv_buf(telnet_user_t *user)
{
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

	return EOK;
}

/** Receive next byte from a socket (use buffering).
 *
 * @param user Telnet user
 * @param byte Place to store the received byte
 * @return EOK on success or an error code
 */
static errno_t telnet_user_recv_next_byte_locked(telnet_user_t *user,
    uint8_t *byte)
{
	errno_t rc;

	/* No more buffered data? */
	if (user->socket_buffer_len <= user->socket_buffer_pos) {
		rc = telnet_user_fill_recv_buf(user);
		if (rc != EOK)
			return rc;
	}

	*byte = (uint8_t)user->socket_buffer[user->socket_buffer_pos++];
	return EOK;
}

/** Determine if a received byte is available without waiting.
 *
 * @param user Telnet user
 * @return @c true iff a byte is currently available
 */
static bool telnet_user_byte_avail(telnet_user_t *user)
{
	return user->socket_buffer_len > user->socket_buffer_pos;
}

static errno_t telnet_user_send_opt(telnet_user_t *user, telnet_cmd_t cmd,
    telnet_cmd_t opt)
{
	uint8_t cmdb[3];

	cmdb[0] = TELNET_IAC;
	cmdb[1] = cmd;
	cmdb[2] = opt;

	return telnet_user_send_raw_locked(user, (char *)cmdb, sizeof(cmdb));
}

/** Process telnet WILL NAWS command.
 *
 * @param user Telnet user structure.
 * @param cmd Telnet command.
 */
static void process_telnet_will_naws(telnet_user_t *user)
{
	telnet_user_log(user, "WILL NAWS");
	/* Send DO NAWS */
	(void) telnet_user_send_opt(user, TELNET_DO, TELNET_NAWS);
	(void) telnet_user_flush_locked(user);
}

/** Process telnet SB NAWS command.
 *
 * @param user Telnet user structure.
 * @param cmd Telnet command.
 */
static void process_telnet_sb_naws(telnet_user_t *user)
{
	uint8_t chi, clo;
	uint8_t rhi, rlo;
	uint16_t cols;
	uint16_t rows;
	uint8_t iac;
	uint8_t se;
	errno_t rc;

	telnet_user_log(user, "SB NAWS...");

	rc = telnet_user_recv_next_byte_locked(user, &chi);
	if (rc != EOK)
		return;
	rc = telnet_user_recv_next_byte_locked(user, &clo);
	if (rc != EOK)
		return;

	rc = telnet_user_recv_next_byte_locked(user, &rhi);
	if (rc != EOK)
		return;
	rc = telnet_user_recv_next_byte_locked(user, &rlo);
	if (rc != EOK)
		return;

	rc = telnet_user_recv_next_byte_locked(user, &iac);
	if (rc != EOK)
		return;
	rc = telnet_user_recv_next_byte_locked(user, &se);
	if (rc != EOK)
		return;

	cols = (chi << 8) | clo;
	rows = (rhi << 8) | rlo;

	telnet_user_log(user, "cols=%u rows=%u\n", cols, rows);

	if (cols < 1 || rows < 1) {
		telnet_user_log(user, "Ignoring invalid window size update.");
		return;
	}

	user->cb->ws_update(user->arg, cols, rows);
}

/** Process telnet WILL command.
 *
 * @param user Telnet user structure.
 * @param opt Option code.
 */
static void process_telnet_will(telnet_user_t *user, telnet_cmd_t opt)
{
	telnet_user_log(user, "WILL");
	switch (opt) {
	case TELNET_NAWS:
		process_telnet_will_naws(user);
		return;
	}

	telnet_user_log(user, "Ignoring telnet command %u %u %u.",
	    TELNET_IAC, TELNET_WILL, opt);
}

/** Process telnet SB command.
 *
 * @param user Telnet user structure.
 * @param opt Option code.
 */
static void process_telnet_sb(telnet_user_t *user, telnet_cmd_t opt)
{
	telnet_user_log(user, "SB");
	switch (opt) {
	case TELNET_NAWS:
		process_telnet_sb_naws(user);
		return;
	}

	telnet_user_log(user, "Ignoring telnet command %u %u %u.",
	    TELNET_IAC, TELNET_SB, opt);
}

/** Process telnet command.
 *
 * @param user Telnet user structure.
 * @param option_code Command option code.
 * @param cmd Telnet command.
 */
static void process_telnet_command(telnet_user_t *user,
    telnet_cmd_t option_code, telnet_cmd_t cmd)
{
	switch (option_code) {
	case TELNET_SB:
		process_telnet_sb(user, cmd);
		return;
	case TELNET_WILL:
		process_telnet_will(user, cmd);
		return;
	}

	if (option_code != 0) {
		telnet_user_log(user, "Ignoring telnet command %u %u %u.",
		    TELNET_IAC, option_code, cmd);
	} else {
		telnet_user_log(user, "Ignoring telnet command %u %u.",
		    TELNET_IAC, cmd);
	}
}

/** Receive data from telnet connection.
 *
 * @param user Telnet user.
 * @param buf Destination buffer
 * @param size Buffer size
 * @param nread Place to store number of bytes read (>0 on success)
 * @return EOK on success or an error code
 */
errno_t telnet_user_recv(telnet_user_t *user, void *buf, size_t size,
    size_t *nread)
{
	uint8_t *bp = (uint8_t *)buf;
	fibril_mutex_lock(&user->recv_lock);

	assert(size > 0);
	*nread = 0;

	do {
		uint8_t next_byte = 0;
		bool inside_telnet_command = false;

		telnet_cmd_t telnet_option_code = 0;

		/* Skip zeros, bail-out on error. */
		do {
			errno_t rc = telnet_user_recv_next_byte_locked(user,
			    &next_byte);
			if (rc != EOK) {
				fibril_mutex_unlock(&user->recv_lock);
				return rc;
			}
			uint8_t byte = next_byte;

			/* Skip telnet commands. */
			if (inside_telnet_command) {
				inside_telnet_command = false;
				next_byte = 0;
				if (TELNET_IS_OPTION_CODE(byte) ||
				    byte == TELNET_SB) {
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
		} while (next_byte == 0 && telnet_user_byte_avail(user));

		/* CR-LF conversions. */
		if (next_byte == 13) {
			next_byte = 10;
		}

		if (next_byte != 0) {
			*bp++ = next_byte;
			++*nread;
			--size;
		}
	} while (size > 0 && (telnet_user_byte_avail(user) || *nread == 0));

	fibril_mutex_unlock(&user->recv_lock);
	return EOK;
}

static errno_t telnet_user_send_raw_locked(telnet_user_t *user,
    const void *data, size_t size)
{
	size_t remain;
	size_t now;
	errno_t rc;

	remain = sizeof(user->send_buf) - user->send_buf_used;
	while (size > 0) {
		if (remain == 0) {
			rc = tcp_conn_send(user->conn, user->send_buf,
			    sizeof(user->send_buf));
			if (rc != EOK)
				return rc;

			user->send_buf_used = 0;
			remain = sizeof(user->send_buf);
		}

		now = min(remain, size);
		memcpy(user->send_buf + user->send_buf_used, data, now);
		user->send_buf_used += now;
		remain -= now;
		data += now;
		size -= now;
	}

	return EOK;
}

/** Send data (convert them first) to the socket, no locking.
 *
 * @param user Telnet user.
 * @param data Data buffer (not zero terminated).
 * @param size Size of @p data buffer in bytes.
 */
static errno_t telnet_user_send_data_locked(telnet_user_t *user,
    const char *data, size_t size)
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
			if (user->cursor_y < (int)user->rows - 1)
				++user->cursor_y;
		} else {
			converted[converted_size++] = data[i];
			if (data[i] == '\b') {
				user->cursor_x--;
			} else {
				user->cursor_x++;
			}
		}
	}

	errno_t rc = telnet_user_send_raw_locked(user, converted,
	    converted_size);
	free(converted);

	return rc;
}

/** Send data (convert them first) to the socket.
 *
 * @param user Telnet user.
 * @param data Data buffer (not zero terminated).
 * @param size Size of @p data buffer in bytes.
 */
errno_t telnet_user_send_data(telnet_user_t *user, const char *data,
    size_t size)
{
	fibril_mutex_lock(&user->send_lock);

	errno_t rc = telnet_user_send_data_locked(user, data, size);

	fibril_mutex_unlock(&user->send_lock);

	return rc;
}

/** Send raw non-printable data to the socket.
 *
 * @param user Telnet user.
 * @param data Data buffer (not zero terminated).
 * @param size Size of @p data buffer in bytes.
 */
errno_t telnet_user_send_raw(telnet_user_t *user, const char *data,
    size_t size)
{
	fibril_mutex_lock(&user->send_lock);

	errno_t rc = telnet_user_send_raw_locked(user, data, size);

	fibril_mutex_unlock(&user->send_lock);

	return rc;
}

static errno_t telnet_user_flush_locked(telnet_user_t *user)
{
	errno_t rc;

	rc = tcp_conn_send(user->conn, user->send_buf, user->send_buf_used);
	if (rc != EOK)
		return rc;

	user->send_buf_used = 0;
	return EOK;
}

errno_t telnet_user_flush(telnet_user_t *user)
{
	errno_t rc;

	fibril_mutex_lock(&user->send_lock);
	rc = telnet_user_flush_locked(user);
	fibril_mutex_unlock(&user->send_lock);
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
	fibril_mutex_lock(&user->send_lock);
	if (user->cursor_x - 1 == new_x) {
		char data = '\b';
		/* Ignore errors. */
		telnet_user_send_data_locked(user, &data, 1);
	}
	user->cursor_x = new_x;
	fibril_mutex_unlock(&user->send_lock);

}

/** Resize telnet session.
 *
 * @param user Telnet user
 * @param cols New number of columns
 * @param rows New number of rows
 */
void telnet_user_resize(telnet_user_t *user, unsigned cols, unsigned rows)
{
	user->cols = cols;
	user->rows = rows;
	if ((unsigned)user->cursor_x > cols - 1)
		user->cursor_x = cols - 1;
	if ((unsigned)user->cursor_y > rows - 1)
		user->cursor_y = rows - 1;
}

/**
 * @}
 */
