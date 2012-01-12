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
#include <adt/prodcons.h>
#include <ipc/input.h>
#include <ipc/console.h>
#include <ipc/vfs.h>
#include <errno.h>
#include <str_error.h>
#include <loc.h>
#include <event.h>
#include <io/keycode.h>
#include <align.h>
#include <malloc.h>
#include <as.h>
#include <fibril_synch.h>
#include <task.h>
#include <net/in.h>
#include <net/inet.h>
#include <net/socket.h>
#include <io/console.h>
#include <inttypes.h>
#include "telnet.h"
#include "user.h"

#define APP_GETTERM  "/app/getterm"


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


static kbd_event_t* new_kbd_event(kbd_event_type_t type, wchar_t c) {
	kbd_event_t *event = malloc(sizeof(kbd_event_t));
	assert(event);

	link_initialize(&event->link);
	event->type = type;
	event->c = c;
	event->mods = 0;
	event->key = (c == '\n' ? KC_ENTER : KC_A);

	return event;
}

static void client_connection_message_loop(telnet_user_t *user)
{
	while (true) {
		ipc_call_t call;
		ipc_callid_t callid = 0;
		while (callid == 0) {
			callid = async_get_call_timeout(&call, 1000);

			fibril_mutex_lock(&user->refcount_mutex);
			bool bail_out = user->socket_closed || user->task_finished;
			fibril_mutex_unlock(&user->refcount_mutex);

			if (bail_out) {
				if (callid != 0) {
					async_answer_0(callid, EINTR);
				}
				return;
			}
		}
		
		if (!IPC_GET_IMETHOD(call)) {
			/* Clean-up. */
			return;
		}

		switch (IPC_GET_IMETHOD(call)) {
		case CONSOLE_GET_SIZE:
			async_answer_2(callid, EOK, 100, 1);
			break;
		case CONSOLE_GET_POS:
			async_answer_2(callid, EOK, 0, 0);
			break;
		case CONSOLE_GET_EVENT: {
			if (list_empty(&user->in_events.list)) {
				retry:
				if (user->socket_buffer_len <= user->socket_buffer_pos) {
					int recv_length = recv(user->socket, user->socket_buffer, BUFFER_SIZE, 0);
					if ((recv_length == 0) || (recv_length == ENOTCONN)) {
						fibril_mutex_lock(&user->refcount_mutex);
						user->socket_closed = true;
						fibril_mutex_unlock(&user->refcount_mutex);
						async_answer_0(callid, ENOENT);
						return;
					}
					if (recv_length < 0) {
						async_answer_0(callid, EINVAL);
						return;
					}
					user->socket_buffer_len = recv_length;
					user->socket_buffer_pos = 0;
				}
				char data = user->socket_buffer[user->socket_buffer_pos++];
				if (data == 13) {
					data = 10;
				}
				if (data == 0)
					goto retry;

				kbd_event_t *down = new_kbd_event(KEY_PRESS, data);
				kbd_event_t *up = new_kbd_event(KEY_RELEASE, data);
				assert(down);
				assert(up);
				prodcons_produce(&user->in_events, &down->link);
				prodcons_produce(&user->in_events, &up->link);
			}


			link_t *link = prodcons_consume(&user->in_events);
			kbd_event_t *event = list_get_instance(link, kbd_event_t, link);
			async_answer_4(callid, EOK, event->type, event->key, event->mods, event->c);
			free(event);
			break;
		}
		case CONSOLE_GOTO:
			async_answer_0(callid, ENOTSUP);
			break;
		case VFS_OUT_READ:
			async_answer_0(callid, ENOTSUP);
			break;
		case VFS_OUT_WRITE: {
			char *buf;
			char *buf_converted;
			size_t size;
			int rc = async_data_write_accept((void **)&buf, false, 0, 0, 0, &size);

			if (rc != EOK) {
				async_answer_0(callid, rc);
				break;
			}
			buf_converted = malloc(2 * size);
			assert(buf_converted);
			int buf_converted_size = 0;
			/* Convert new-lines. */
			for (size_t i = 0; i < size; i++) {
				if (buf[i] == 10) {
					buf_converted[buf_converted_size++] = 13;
					buf_converted[buf_converted_size++] = 10;
				} else {
					buf_converted[buf_converted_size++] = buf[i];
				}
			}
			rc = send(user->socket, buf_converted, buf_converted_size, 0);
			free(buf);

			if (rc != EOK) {
				printf("Problem sending data: %s\n", str_error(rc));
				async_answer_0(callid, rc);
				break;
			}

			async_answer_1(callid, EOK, size);

			break;
		}
		case VFS_OUT_SYNC:
			async_answer_0(callid, EOK);
			break;
		case CONSOLE_CLEAR:
			async_answer_0(callid, EOK);
			break;

		case CONSOLE_GET_COLOR_CAP:
			async_answer_1(callid, EOK, CONSOLE_CAP_NONE);
			break;
		case CONSOLE_SET_STYLE:
			async_answer_0(callid, ENOTSUP);
			break;
		case CONSOLE_SET_COLOR:
			async_answer_0(callid, ENOTSUP);
			break;
		case CONSOLE_SET_RGB_COLOR:
			async_answer_0(callid, ENOTSUP);
			break;

		case CONSOLE_CURSOR_VISIBILITY:
			async_answer_0(callid, ENOTSUP);
			break;

		default:
			async_answer_0(callid, EINVAL);
			break;
		}
	}
}

static void client_connection(ipc_callid_t iid, ipc_call_t *icall, void *arg)
{
	/* Find the user. */
	telnet_user_t *user = telnet_user_get_for_client_connection(IPC_GET_ARG1(*icall));
	if (user == NULL) {
		async_answer_0(iid, ENOENT);
		return;
	}

	printf("New user for service %s.\n", user->service_name);

	/* Accept the connection, increment reference. */
	async_answer_0(iid, EOK);

	/* Force character mode. */
	send(user->socket, (void *)telnet_force_character_mode_command,
	    telnet_force_character_mode_command_count, 0);

	client_connection_message_loop(user);

	/* Announce user disconnection. */
	fibril_mutex_lock(&user->refcount_mutex);
	user->locsrv_connection_count--;
	fibril_condvar_signal(&user->refcount_cv);
	fibril_mutex_unlock(&user->refcount_mutex);
}

static int spawn_task_fibril(void *arg)
{
	telnet_user_t *user = arg;
	int rc;

	char term[LOC_NAME_MAXLEN];
	snprintf(term, LOC_NAME_MAXLEN, "%s/%s", "/loc", user->service_name);

	task_id_t task;
	rc = task_spawnl(&task, APP_GETTERM, APP_GETTERM, term, "/app/bdsh", NULL);
	if (rc != EOK) {
		printf("%s: Error spawning %s -w %s %s (%s)\n", NAME,
		    APP_GETTERM, term, "/app/bdsh", str_error(rc));
		fibril_mutex_lock(&user->refcount_mutex);
		user->task_finished = true;
		fibril_condvar_signal(&user->refcount_cv);
		fibril_mutex_unlock(&user->refcount_mutex);
		return EOK;
	}

	fibril_mutex_lock(&user->refcount_mutex);
	user->task_id = task;
	fibril_mutex_unlock(&user->refcount_mutex);

	task_exit_t task_exit;
	int task_retval;
	task_wait(task, &task_exit, &task_retval);
	printf("%s: getterm terminated: %d, %d\n", NAME, task_exit, task_retval);

	/* Announce destruction. */
	fibril_mutex_lock(&user->refcount_mutex);
	user->task_finished = true;
	fibril_condvar_signal(&user->refcount_cv);
	fibril_mutex_unlock(&user->refcount_mutex);

	return EOK;
}

static bool user_can_be_destroyed_no_lock(telnet_user_t *user)
{
	return user->task_finished && user->socket_closed &&
	    (user->locsrv_connection_count == 0);
}

static int network_user_fibril(void *arg)
{
	int rc;
	telnet_user_t *user = arg;

	rc = loc_service_register(user->service_name, &user->service_id);
	if (rc != EOK) {
		fprintf(stderr, "%s: Unable to register device %s\n", NAME,
		    user->service_name);
		return EOK;
	}
	printf("Service %s registered as %" PRIun "\n", user->service_name,
	    user->service_id);

	fid_t spawn_fibril = fibril_create(spawn_task_fibril, user);
	assert(spawn_fibril);
	fibril_add_ready(spawn_fibril);

	/* Wait for all clients to exit. */
	fibril_mutex_lock(&user->refcount_mutex);
	while (!user_can_be_destroyed_no_lock(user)) {
		if (user->task_finished) {
			closesocket(user->socket);
			user->socket_closed = true;
			continue;
		} else if (user->socket_closed) {
			if (user->task_id != 0) {
				task_kill(user->task_id);
			}
		}
		fibril_condvar_wait_timeout(&user->refcount_cv, &user->refcount_mutex, 1000);
	}
	fibril_mutex_unlock(&user->refcount_mutex);

	rc = loc_service_unregister(user->service_id);
	if (rc != EOK) {
		fprintf(stderr, "Warning: failed to unregister %s: %s\n", user->service_name, str_error(rc));
	}

	printf("Destroying service %s.\n", user->service_name);
	telnet_user_destroy(user);

	return EOK;
}

int main(int argc, char *argv[])
{
	int port = 2223;
	
	int rc = loc_server_register(NAME, client_connection);
	if (rc < 0) {
		printf("%s: Unable to register server (%s).\n", NAME,
		    str_error(rc));
		return 1;
	}

	struct sockaddr_in addr;

	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);

	rc = inet_pton(AF_INET, "127.0.0.1", (void *)
	    &addr.sin_addr.s_addr);
	if (rc != EOK) {
		fprintf(stderr, "Error parsing network address (%s)\n",
		    str_error(rc));
		return 2;
	}

	int listen_sd = socket(PF_INET, SOCK_STREAM, 0);
	if (listen_sd < 0) {
		fprintf(stderr, "Error creating listening socket (%s)\n",
		    str_error(listen_sd));
		return 3;
	}

	rc = bind(listen_sd, (struct sockaddr *) &addr, sizeof(addr));
	if (rc != EOK) {
		fprintf(stderr, "Error binding socket (%s)\n",
		    str_error(rc));
		return 4;
	}

	rc = listen(listen_sd, BACKLOG_SIZE);
	if (rc != EOK) {
		fprintf(stderr, "listen() failed (%s)\n", str_error(rc));
		return 5;
	}

	printf("%s: HelenOS Remote console service\n", NAME);

	while (true) {
		struct sockaddr_in raddr;
		socklen_t raddr_len = sizeof(raddr);
		int conn_sd = accept(listen_sd, (struct sockaddr *) &raddr,
		    &raddr_len);

		if (conn_sd < 0) {
			fprintf(stderr, "accept() failed (%s)\n", str_error(rc));
			continue;
		}

		telnet_user_t *user = telnet_user_create(conn_sd);
		assert(user);

		fid_t fid = fibril_create(network_user_fibril, user);
		assert(fid);
		fibril_add_ready(fid);
	}

	return 0;
}

/** @}
 */
