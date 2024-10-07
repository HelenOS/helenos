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

#ifndef TELNET_USER_H_
#define TELNET_USER_H_

#include <fibril_synch.h>
#include <inet/tcp.h>
#include <inttypes.h>
#include <io/con_srv.h>

#define BUFFER_SIZE 32
#define SEND_BUF_SIZE 512

/** Telnet callbacks */
typedef struct {
	void (*ws_update)(void *, unsigned, unsigned);
} telnet_cb_t;

/** Representation of a connected (human) user. */
typedef struct {
	/** Synchronize send operations */
	fibril_mutex_t send_lock;
	/** Synchronize receive operations */
	fibril_mutex_t recv_lock;
	/** Callback functions */
	telnet_cb_t *cb;
	/** Argument to callback functions */
	void *arg;

	/** Internal id, used for creating locfs entries. */
	int id;
	/** Associated connection. */
	tcp_conn_t *conn;
	/** Location service id assigned to the virtual terminal. */
	service_id_t service_id;
	/** Path name of the service. */
	char *service_name;
	/** Console service setup */
	con_srvs_t srvs;

	link_t link;
	char socket_buffer[BUFFER_SIZE];
	size_t socket_buffer_len;
	size_t socket_buffer_pos;
	char send_buf[SEND_BUF_SIZE];
	size_t send_buf_used;

	/** Task id of the launched application. */
	task_id_t task_id;

	/* Reference counting. */
	fibril_condvar_t refcount_cv;
	bool task_finished;
	int locsrv_connection_count;
	bool socket_closed;

	/** X position of the cursor. */
	int cursor_x;
	/** Y position of the cursor. */
	int cursor_y;
	/** Total number of columns */
	unsigned cols;
	/** Total number of rows */
	unsigned rows;
} telnet_user_t;

extern telnet_user_t *telnet_user_create(tcp_conn_t *, telnet_cb_t *, void *);
extern void telnet_user_add(telnet_user_t *);
extern void telnet_user_destroy(telnet_user_t *);
extern telnet_user_t *telnet_user_get_for_client_connection(service_id_t);
extern bool telnet_user_is_zombie(telnet_user_t *);
extern void telnet_user_notify_client_disconnected(telnet_user_t *);
extern errno_t telnet_user_get_next_keyboard_event(telnet_user_t *, kbd_event_t *);
extern errno_t telnet_user_send_data(telnet_user_t *, const char *, size_t);
extern errno_t telnet_user_send_raw(telnet_user_t *, const char *, size_t);
extern errno_t telnet_user_flush(telnet_user_t *);
extern errno_t telnet_user_recv(telnet_user_t *, void *, size_t, size_t *);
extern void telnet_user_update_cursor_x(telnet_user_t *, int);
extern void telnet_user_resize(telnet_user_t *, unsigned, unsigned);

/** Print informational message about connected user. */
#ifdef CONFIG_DEBUG
#define telnet_user_log(user, fmt, ...) \
	printf(NAME " [console %d (%d)]: " fmt "\n", \
	    user->id, (int) user->service_id, ##__VA_ARGS__)
#else
#define telnet_user_log(user, fmt, ...) ((void) 0)
#endif

/** Print error message associated with connected user. */
#define telnet_user_error(user, fmt, ...) \
	fprintf(stderr, NAME " [console %d (%d)]: ERROR: " fmt "\n", \
	    user->id, (int) user->service_id, ##__VA_ARGS__)

#endif

/**
 * @}
 */
