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


#define APP_GETTERM  "/app/getterm"
#define NAME       "remterm"
#define NAMESPACE  "term"
#define BACKLOG_SIZE  5

#define BUFFER_SIZE 1024

typedef struct {
	int id;
	int socket;
	service_id_t service_id;
	/** Producer-consumer of kbd_event_t. */
	prodcons_t in_events;
	link_t link;
	char socket_buffer[BUFFER_SIZE];
	size_t socket_buffer_len;
	size_t socket_buffer_pos;
} client_t;

static LIST_INITIALIZE(clients);
static int client_counter = 0;


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

static void client_connection(ipc_callid_t iid, ipc_call_t *icall, void *arg)
{
	service_id_t id = IPC_GET_ARG1(*icall);

	/* Find the client. */
	client_t *client = NULL;
	list_foreach(clients, link) {
		client_t *tmp = list_get_instance(link, client_t, link);
		if (tmp->service_id == IPC_GET_ARG1(*icall)) {
			client = tmp;
			break;
		}
	}

	if (client == NULL) {
		async_answer_0(iid, ENOENT);
		return;
	}

	printf("New client for service %" PRIun ".\n", id);

	/* Accept the connection */
	async_answer_0(iid, EOK);

	/*
	 * Force character mode.
	 * IAC WILL ECHO IAC WILL SUPPRESS_GO_AHEAD IAC WONT LINEMODE
	 * http://stackoverflow.com/questions/273261/force-telnet-client-into-character-mode
	 */
	const char force_char_mode[] = {255, 251, 1, 255, 251, 3, 255, 252, 34};
	send(client->socket, (void *)force_char_mode, sizeof(force_char_mode), 0);

	while (true) {
		ipc_call_t call;
		ipc_callid_t callid = async_get_call(&call);
		
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
			if (list_empty(&client->in_events.list)) {
				retry:
				if (client->socket_buffer_len <= client->socket_buffer_pos) {
					int recv_length = recv(client->socket, client->socket_buffer, BUFFER_SIZE, 0);
					if (recv_length == 0) {
						async_answer_0(callid, ENOENT);
						return;
					}
					if (recv_length < 0) {
						async_answer_0(callid, EINVAL);
						break;
					}
					client->socket_buffer_len = recv_length;
					client->socket_buffer_pos = 0;
				}
				char data = client->socket_buffer[client->socket_buffer_pos++];
				if (data == 13) {
					data = 10;
				}
				if (data == 0)
					goto retry;

				kbd_event_t *down = new_kbd_event(KEY_PRESS, data);
				kbd_event_t *up = new_kbd_event(KEY_RELEASE, data);
				assert(down);
				assert(up);
				prodcons_produce(&client->in_events, &down->link);
				prodcons_produce(&client->in_events, &up->link);
			}


			link_t *link = prodcons_consume(&client->in_events);
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
			rc = send(client->socket, buf_converted, buf_converted_size, 0);
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


static int network_client_fibril(void *arg)
{
	int rc;
	client_t *client = arg;

	// FIXME: locking
	list_append(&client->link, &clients);

	char vc[LOC_NAME_MAXLEN + 1];
	snprintf(vc, LOC_NAME_MAXLEN, "%s/rem%d", NAMESPACE, client->id);
	if (loc_service_register(vc, &client->service_id) != EOK) {
		fprintf(stderr, "%s: Unable to register device %s\n", NAME, vc);
		return EOK;
	}
	printf("Service %s registered as %" PRIun "\n", vc, client->service_id);


	prodcons_initialize(&client->in_events);
	client->socket_buffer_len = 0;
	client->socket_buffer_pos = 0;

	char term[LOC_NAME_MAXLEN];
	snprintf(term, LOC_NAME_MAXLEN, "%s/%s", "/loc", vc);

	task_id_t task;
	rc = task_spawnl(&task, APP_GETTERM, APP_GETTERM, term, "/app/bdsh", NULL);
	if (rc != EOK) {
		printf("%s: Error spawning %s -w %s %s (%s)\n", NAME,
		    APP_GETTERM, term, "/app/bdsh", str_error(rc));
		return EOK;
	}

	task_exit_t task_exit;
	int task_retval;
	task_wait(task, &task_exit, &task_retval);
	printf("%s: getterm terminated: %d, %d\n", NAME, task_exit, task_retval);

	closesocket(client->socket);

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

		client_t *client = malloc(sizeof(client_t));
		assert(client);
		client->id = ++client_counter;
		client->socket = conn_sd;

		fid_t fid = fibril_create(network_client_fibril, client);
		assert(fid);
		fibril_add_ready(fid);
	}

	return 0;
}

/** @}
 */
