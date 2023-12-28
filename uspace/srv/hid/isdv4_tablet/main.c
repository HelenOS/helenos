/*
 * Copyright (c) 2023 Jiri Svoboda
 * Copyright (c) 2012 Martin Sucha
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

#include <async.h>
#include <errno.h>
#include <fibril_synch.h>
#include <io/serial.h>
#include <ipc/mouseev.h>
#include <loc.h>
#include <stddef.h>
#include <stdio.h>
#include <str.h>
#include <task.h>

#include "isdv4.h"

#define NAME "isdv4_tablet"

static async_sess_t *client_sess = NULL;
static fibril_mutex_t client_mutex;
static isdv4_state_t state;

static void syntax_print(void)
{
	fprintf(stderr, "Usage: %s [--baud=<baud>] [--print-events] [device_service]\n", NAME);
}

static errno_t read_fibril(void *unused)
{
	errno_t rc = isdv4_read_events(&state);
	if (rc != EOK) {
		fprintf(stderr, "Failed reading events");
		return rc;
	}

	isdv4_fini(&state);
	return EOK;
}

static void mouse_connection(ipc_call_t *icall, void *arg)
{
	async_accept_0(icall);

	async_sess_t *sess =
	    async_callback_receive(EXCHANGE_SERIALIZE);

	fibril_mutex_lock(&client_mutex);

	if (client_sess == NULL)
		client_sess = sess;

	fibril_mutex_unlock(&client_mutex);

	while (true) {
		ipc_call_t call;
		async_get_call(&call);

		if (!ipc_get_imethod(&call)) {
			async_answer_0(&call, EOK);
			break;
		}

		async_answer_0(&call, ENOTSUP);
	}
}

static void emit_event(const isdv4_event_t *event)
{
	fibril_mutex_lock(&client_mutex);
	async_sess_t *sess = client_sess;
	fibril_mutex_unlock(&client_mutex);

	if (!sess)
		return;

	async_exch_t *exch = async_exchange_begin(sess);
	if (exch) {
		unsigned int max_x = state.stylus_max_x;
		unsigned int max_y = state.stylus_max_y;
		if (event->source == TOUCH) {
			max_x = state.touch_max_x;
			max_y = state.touch_max_y;
		}
		async_msg_4(exch, MOUSEEV_ABS_MOVE_EVENT, event->x, event->y,
		    max_x, max_y);
		if (event->type == PRESS || event->type == RELEASE) {
			async_msg_2(exch, MOUSEEV_BUTTON_EVENT, event->button,
			    event->type == PRESS);
		}
	}
	async_exchange_end(exch);
}

static void print_and_emit_event(const isdv4_event_t *event)
{
	const char *type = NULL;
	switch (event->type) {
	case PRESS:
		type = "PRESS";
		break;
	case RELEASE:
		type = "RELEASE";
		break;
	case PROXIMITY_IN:
		type = "PROXIMITY IN";
		break;
	case PROXIMITY_OUT:
		type = "PROXIMITY OUT";
		break;
	case MOVE:
		type = "MOVE";
		break;
	default:
		type = "UNKNOWN";
		break;
	}

	const char *source = NULL;
	switch (event->source) {
	case STYLUS_TIP:
		source = "stylus tip";
		break;
	case STYLUS_ERASER:
		source = "stylus eraser";
		break;
	case TOUCH:
		source = "touch";
		break;
	}

	printf("%s %s %u %u %u %u\n", type, source, event->x, event->y,
	    event->pressure, event->button);

	emit_event(event);
}

static const char *touch_type(unsigned int data_id)
{
	switch (data_id) {
	case 0:
		return "resistive+stylus";
	case 1:
		return "capacitive+stylus";
	case 2:
		return "resistive";
	case 3:
	case 4:
		return "capacitive";
	case 5:
		return "penabled";
	}
	return "unknown";
}

int main(int argc, char **argv)
{
	sysarg_t baud = 38400;
	service_id_t svc_id;
	serial_t *serial;
	char *serial_port_name = NULL;
	loc_srv_t *srv;

	int arg = 1;
	errno_t rc;

	isdv4_event_fn event_fn = emit_event;

	if (argc > arg && str_test_prefix(argv[arg], "--baud=")) {
		size_t arg_offset = str_lsize(argv[arg], 7);
		char *arg_str = argv[arg] + arg_offset;
		if (str_length(arg_str) == 0) {
			fprintf(stderr, "--baud requires an argument\n");
			syntax_print();
			return 1;
		}
		char *endptr;
		baud = strtol(arg_str, &endptr, 10);
		if (*endptr != '\0') {
			fprintf(stderr, "Invalid value for baud\n");
			syntax_print();
			return 1;
		}
		arg++;
	}

	if (argc > arg && str_cmp(argv[arg], "--print-events") == 0) {
		event_fn = print_and_emit_event;
		arg++;
	}

	if (argc > arg) {
		serial_port_name = argv[arg];
		rc = loc_service_get_id(serial_port_name, &svc_id, 0);
		if (rc != EOK) {
			fprintf(stderr, "Cannot find device service %s\n",
			    argv[arg]);
			return 1;
		}
		arg++;
	} else {
		category_id_t serial_cat_id;

		rc = loc_category_get_id("serial", &serial_cat_id, 0);
		if (rc != EOK) {
			fprintf(stderr, "Failed getting id of category "
			    "'serial'\n");
			return 1;
		}

		service_id_t *svc_ids;
		size_t svc_count;

		rc = loc_category_get_svcs(serial_cat_id, &svc_ids, &svc_count);
		if (rc != EOK) {
			fprintf(stderr, "Failed getting list of services\n");
			return 1;
		}

		if (svc_count == 0) {
			fprintf(stderr, "No service in category 'serial'\n");
			free(svc_ids);
			return 1;
		}

		svc_id = svc_ids[0];

		rc = loc_service_get_name(svc_id, &serial_port_name);
		if (rc != EOK) {
			fprintf(stderr, "Failed getting name of serial service\n");
			return 1;
		}

		free(svc_ids);
	}

	if (argc > arg) {
		fprintf(stderr, "Too many arguments\n");
		syntax_print();
		return 1;
	}

	fibril_mutex_initialize(&client_mutex);

	printf(NAME ": Using serial port %s\n", serial_port_name);

	async_sess_t *sess = loc_service_connect(svc_id, INTERFACE_DDF,
	    IPC_FLAG_BLOCKING);
	if (!sess) {
		fprintf(stderr, "Failed connecting to service\n");
		return 2;
	}

	rc = serial_open(sess, &serial);
	if (rc != EOK) {
		fprintf(stderr, "Failed opening serial port\n");
		return 2;
	}

	rc = serial_set_comm_props(serial, baud, SERIAL_NO_PARITY, 8, 1);
	if (rc != EOK) {
		fprintf(stderr, "Failed setting serial properties\n");
		return 2;
	}

	rc = isdv4_init(&state, sess, event_fn);
	if (rc != EOK) {
		fprintf(stderr, "Failed initializing isdv4 state");
		return 2;
	}

	rc = isdv4_init_tablet(&state);
	if (rc != EOK) {
		fprintf(stderr, "Failed initializing tablet");
		return 2;
	}

	printf("Tablet information:\n");
	printf(" Stylus: %ux%u pressure: %u tilt: ", state.stylus_max_x,
	    state.stylus_max_y, state.stylus_max_pressure);
	if (state.stylus_tilt_supported) {
		printf("%ux%u\n", state.stylus_max_xtilt, state.stylus_max_ytilt);
	} else {
		printf("not supported\n");
	}
	printf(" Touch: %ux%u type: %s\n", state.touch_max_x, state.touch_max_y,
	    touch_type(state.touch_type));

	fid_t fibril = fibril_create(read_fibril, NULL);
	/* From this on, state is to be used only by read_fibril */
	fibril_add_ready(fibril);

	async_set_fallback_port_handler(mouse_connection, NULL);
	rc = loc_server_register(NAME, &srv);
	if (rc != EOK) {
		printf("%s: Unable to register driver.\n", NAME);
		return rc;
	}

	service_id_t service_id;
	char *service_name;
	rc = asprintf(&service_name, "mouse/isdv4-%" PRIun, svc_id);
	if (rc < 0) {
		printf(NAME ": Unable to create service name\n");
		return rc;
	}

	rc = loc_service_register(srv, service_name, &service_id);
	if (rc != EOK) {
		loc_server_unregister(srv);
		printf(NAME ": Unable to register service %s.\n", service_name);
		return rc;
	}

	category_id_t mouse_category;
	rc = loc_category_get_id("mouse", &mouse_category, IPC_FLAG_BLOCKING);
	if (rc != EOK) {
		printf(NAME ": Unable to get mouse category id.\n");
	} else {
		rc = loc_service_add_to_cat(srv, service_id, mouse_category);
		if (rc != EOK) {
			printf(NAME ": Unable to add device to mouse category.\n");
		}
	}

	printf("%s: Accepting connections\n", NAME);
	task_retval(0);
	async_manager();

	/* Not reached */
	return 0;
}
