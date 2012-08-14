/*
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

#include <device/char_dev.h>
#include <errno.h>
#include <ipc/serial_ctl.h>
#include <loc.h>
#include <stdio.h>

#include "isdv4.h"

static void syntax_print(void)
{
	fprintf(stderr, "Usage: wacomdump [--baud=<baud>] [device_service]\n");
}

static void print_event(const isdv4_event_t *event)
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
			return;
		case UNKNOWN:
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

	const char *buttons = "none";
	switch (event->button) {
		case 1:
			buttons = "button1";
			break;
		case 2:
			buttons = "button2";
			break;
		case 3:
			buttons = "both";
			break;
	}

	printf("%s %s %u %u %u %s\n", type, source, event->x, event->y,
	    event->pressure, buttons);
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

	int arg = 1;
	int rc;

	if (argc > arg && str_test_prefix(argv[arg], "--baud=")) {
		size_t arg_offset = str_lsize(argv[arg], 7);
		char* arg_str = argv[arg] + arg_offset;
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

	if (argc > arg) {
		rc = loc_service_get_id(argv[arg], &svc_id, 0);
		if (rc != EOK) {
			fprintf(stderr, "Cannot find device service %s\n",
			    argv[arg]);
			return 1;
		}
		arg++;
	}
	else {
		category_id_t serial_cat_id;

		rc = loc_category_get_id("serial", &serial_cat_id, 0);
		if (rc != EOK) {
			fprintf(stderr, "Failed getting id of category "
			    "'serial'\n");
			return 1;
		}

		service_id_t *svc_ids;
		size_t svc_count;

		rc = loc_category_get_svcs(serial_cat_id, &svc_ids, &svc_count);		if (rc != EOK) {
			fprintf(stderr, "Failed getting list of services\n");
			return 1;
		}

		if (svc_count == 0) {
			fprintf(stderr, "No service in category 'serial'\n");
			free(svc_ids);
			return 1;
		}

		svc_id = svc_ids[0];
		free(svc_ids);
	}

	if (argc > arg) {
		fprintf(stderr, "Too many arguments\n");
		syntax_print();
		return 1;
	}

	async_sess_t *sess = loc_service_connect(EXCHANGE_SERIALIZE, svc_id,
	    IPC_FLAG_BLOCKING);
	if (!sess) {
		fprintf(stderr, "Failed connecting to service\n");
	}

	async_exch_t *exch = async_exchange_begin(sess);
	rc = async_req_4_0(exch, SERIAL_SET_COM_PROPS, baud,
	    SERIAL_NO_PARITY, 8, 1);
	async_exchange_end(exch);

	if (rc != EOK) {
		fprintf(stderr, "Failed setting serial properties\n");
		return 2;
	}

	isdv4_state_t state;
	rc = isdv4_init(&state, sess, print_event);
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
	}
	else {
		printf("not supported\n");
	}
	printf(" Touch: %ux%u type: %s\n", state.touch_max_x, state.touch_max_y,
		touch_type(state.touch_type));

	rc = isdv4_read_events(&state);
	if (rc != EOK) {
		fprintf(stderr, "Failed reading events");
		return 2;
	}

	isdv4_fini(&state);

	return 0;
}
