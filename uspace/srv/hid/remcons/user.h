/*
 * SPDX-FileCopyrightText: 2012 Vojtech Horky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup remcons
 * @{
 */
/** @file
 */

#ifndef TELNET_USER_H_
#define TELNET_USER_H_

#include <adt/prodcons.h>
#include <fibril_synch.h>
#include <inet/tcp.h>
#include <inttypes.h>
#include <io/con_srv.h>
#include "remcons.h"

#define BUFFER_SIZE 32

/** Representation of a connected (human) user. */
typedef struct {
	/** Mutex guarding the whole structure. */
	fibril_mutex_t guard;

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

	/** Producer-consumer of kbd_event_t. */
	prodcons_t in_events;
	link_t link;
	char socket_buffer[BUFFER_SIZE];
	size_t socket_buffer_len;
	size_t socket_buffer_pos;

	/** Task id of the launched application. */
	task_id_t task_id;

	/* Reference counting. */
	fibril_condvar_t refcount_cv;
	bool task_finished;
	int locsrv_connection_count;
	bool socket_closed;

	/** X position of the cursor. */
	int cursor_x;
} telnet_user_t;

extern telnet_user_t *telnet_user_create(tcp_conn_t *);
extern void telnet_user_add(telnet_user_t *);
extern void telnet_user_destroy(telnet_user_t *);
extern telnet_user_t *telnet_user_get_for_client_connection(service_id_t);
extern bool telnet_user_is_zombie(telnet_user_t *);
extern void telnet_user_notify_client_disconnected(telnet_user_t *);
extern errno_t telnet_user_get_next_keyboard_event(telnet_user_t *, kbd_event_t *);
extern errno_t telnet_user_send_data(telnet_user_t *, uint8_t *, size_t);
extern void telnet_user_update_cursor_x(telnet_user_t *, int);

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
