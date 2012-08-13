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
#include <macros.h>

#define BUF_SIZE 64

#define START_OF_PACKET 128
#define CONTROL_PACKET 64
#define TOUCH_EVENT 16
#define FINGER1 1
#define FINGER2 2
#define TIP 1
#define BUTTON1 2
#define BUTTON2 4
#define PROXIMITY 32

#define CMD_START '1'
#define CMD_STOP '0'
#define CMD_QUERY_STYLUS '*'
#define CMD_QUERY_TOUCH '%'

typedef struct isdv4_event isdv4_event_t;

typedef void (*isdv4_event_fn)(const isdv4_event_t *);

typedef struct {
	/* Stylus information */
	unsigned int stylus_max_x;
	unsigned int stylus_max_y;
	unsigned int stylus_max_pressure;
	unsigned int stylus_max_xtilt;
	unsigned int stylus_max_ytilt;
	bool stylus_tilt_supported;

	/* Touch information */
	unsigned int touch_type;
	unsigned int touch_max_x;
	unsigned int touch_max_y;

	/* Event state */
	bool stylus_in_proximity;
	bool stylus_is_eraser;
	bool tip_pressed; /* Reported as stylus button 1 */
	bool button1_pressed; /* Reported as stylus button 2 */
	bool button2_pressed; /* Reported as stylus button 3 */
	bool finger1_pressed; /* Reported as touch button 1 */

	/* Session to the serial device */
	async_sess_t *sess;

	/* Receive buffer state */
	uint8_t *buf;
	size_t buf_size;
	size_t buf_end;

	/* Callbacks */
	isdv4_event_fn emit_event_fn;
} isdv4_state_t;

typedef enum {
	UNKNOWN, PRESS, RELEASE, PROXIMITY_IN, PROXIMITY_OUT, MOVE
} isdv4_event_type_t;

typedef enum {
	STYLUS_TIP, STYLUS_ERASER, TOUCH
} isdv4_source_type_t;

typedef struct isdv4_event {
	isdv4_event_type_t type;
	isdv4_source_type_t source;
	unsigned int x;
	unsigned int y;
	unsigned int pressure;
	unsigned int button;
} isdv4_event_t;

static void isdv4_event_init(isdv4_event_t *event)
{
	memset(event, 0, sizeof(isdv4_event_t));
}

/* packet_consumer_fn(uint8_t *packet, size_t size, isdv4_state_t *state)
   return true if reading of packets should continue */
typedef bool (*packet_consumer_fn)(uint8_t *, size_t, isdv4_state_t *);

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

static bool parse_event(uint8_t *packet, size_t size, isdv4_state_t *state)
{
	if (size < 1) {
		printf("Invalid packet size\n");
		return false;
	}
	bool control_packet = ((packet[0] & CONTROL_PACKET) > 0);
	if (control_packet) {
		printf("This is not an event packet\n");
		return true;
	}

	/* This is an event initiated by the device */
	isdv4_event_t event;
	isdv4_event_init(&event);


	if (size == 5 || size == 7) {
		/* This is a touch event */
		bool finger1 = (packet[0] & FINGER1) > 0;
		event.x = ((packet[1] & 127) << 7) | (packet[2] & 127);
		event.y = ((packet[3] & 127) << 7) | (packet[4] & 127);
		event.source = TOUCH;

		if (!state->stylus_in_proximity) {
			if (!finger1 && state->finger1_pressed) {
				state->finger1_pressed = false;

				event.type = RELEASE;
				event.button = 1;
				state->emit_event_fn(&event);
			}
			else if (finger1 && !state->finger1_pressed) {
				state->finger1_pressed = true;

				event.type = PRESS;
				event.button = 1;
				state->emit_event_fn(&event);
			}
			else {
				event.type = MOVE;
				event.button = 1;
				state->emit_event_fn(&event);
			}
		}
	}
	else if (size == 9) {
		/* This is a stylus event */
		bool tip = packet[0] & TIP;
		bool button1 = packet[0] & BUTTON1;
		bool button2 = packet[0] & BUTTON2;
		bool proximity = packet[0] & PROXIMITY;
		event.x = ((packet[1] & 127) << 7) | (packet[2] & 124) | ((packet[6] >> 5) & 3);
		event.y = ((packet[3] & 127) << 7) | (packet[4] & 124) | ((packet[6] >> 3) & 3);
		event.pressure = (packet[5] & 127) | ((packet[6] & 7) << 7);

		if (proximity && !state->stylus_in_proximity) {
			/* Stylus came into proximity */
			state->stylus_in_proximity = true;
			state->stylus_is_eraser = !tip && button2;
			event.source = (state->stylus_is_eraser ? STYLUS_ERASER : STYLUS_TIP);
			event.type = PROXIMITY_IN;
			state->emit_event_fn(&event);
		}
		else if (!proximity && state->stylus_in_proximity) {
			/* Stylus came out of proximity */
			state->stylus_in_proximity = false;
			event.source = (state->stylus_is_eraser ? STYLUS_ERASER : STYLUS_TIP);
			event.type = PROXIMITY_OUT;
			state->emit_event_fn(&event);
		}
		else {
			/* Proximity state didn't change, but we need to check if it is still eraser */
			if (state->stylus_is_eraser && !button2) {
				event.type = PROXIMITY_OUT;
				event.source = STYLUS_ERASER;
				state->emit_event_fn(&event);
				event.type = PROXIMITY_IN;
				event.source = STYLUS_TIP;
				state->emit_event_fn(&event);
				state->stylus_is_eraser = false;
			}
			else if (!state->stylus_is_eraser && !tip && button2) {
				event.type = PROXIMITY_OUT;
				event.source = STYLUS_TIP;
				state->emit_event_fn(&event);
				event.type = PROXIMITY_IN;
				event.source = STYLUS_ERASER;
				state->emit_event_fn(&event);
				state->stylus_is_eraser = true;
			}
		}
		
		if (!state->stylus_is_eraser) {
			if (tip && !state->tip_pressed) {
				state->tip_pressed = true;
				event.type = PRESS;
				event.source = STYLUS_TIP;
				event.button = 1;
				state->emit_event_fn(&event);
			}
			else if (!tip && state->tip_pressed) {
				state->tip_pressed = false;
				event.type = RELEASE;
				event.source = STYLUS_TIP;
				event.button = 1;
				state->emit_event_fn(&event);
			}
			if (button1 && !state->button1_pressed) {
				state->button1_pressed = true;
				event.type = PRESS;
				event.source = STYLUS_TIP;
				event.button = 2;
				state->emit_event_fn(&event);
			}
			else if (!button1 && state->button1_pressed) {
				state->button1_pressed = false;
				event.type = RELEASE;
				event.source = STYLUS_TIP;
				event.button = 2;
				state->emit_event_fn(&event);
			}
			if (button2 && !state->button2_pressed) {
				state->button2_pressed = true;
				event.type = PRESS;
				event.source = STYLUS_TIP;
				event.button = 3;
				state->emit_event_fn(&event);
			}
			else if (!button2 && state->button2_pressed) {
				state->button2_pressed = false;
				event.type = RELEASE;
				event.source = STYLUS_TIP;
				event.button = 3;
				state->emit_event_fn(&event);
			}
			event.type = MOVE;
			event.source = STYLUS_TIP;
			event.button = (tip ? 1: 0) | (button1 ? 2 : 0) | (button2 ? 4 : 0);
			state->emit_event_fn(&event);
		}
		else {
			if (tip && !state->tip_pressed) {
				state->tip_pressed = true;
				event.type = PRESS;
				event.source = STYLUS_ERASER;
				event.button = 1;
				state->emit_event_fn(&event);
			}
			else if (!tip && state->tip_pressed) {
				state->tip_pressed = false;
				event.type = RELEASE;
				event.source = STYLUS_ERASER;
				event.button = 1;
				state->emit_event_fn(&event);
			}
			event.type = MOVE;
			event.source = STYLUS_ERASER;
			event.button = (tip ? 1 : 0);
			state->emit_event_fn(&event);
		}
	}

	return true;
}

static bool parse_response_stylus(uint8_t *packet, size_t size,
    isdv4_state_t *state)
{
	if (size != 11) {
		fprintf(stderr, "Unexpected length of stylus response packet\n");
		return false;
	}
	bool control_packet = ((packet[0] & CONTROL_PACKET) > 0);
	if (!control_packet) {
		fprintf(stderr, "This is not a control packet\n");
		return false;
	}

	unsigned int data_id = (packet[0] & 63);
	unsigned int version = ((packet[9] & 127) << 7) | (packet[10] & 127);

	unsigned int max_x = ((packet[1] & 127) << 7) | (packet[2] & 124) |
	    ((packet[6] >> 5) & 3);
	unsigned int max_y = ((packet[3] & 127) << 7) | (packet[4] & 124) |
	    ((packet[6] >> 3) & 3);
	unsigned int max_pressure = (packet[5] & 63) | ((packet[6] & 7) << 7);
	unsigned int max_xtilt = packet[8] & 127;
	unsigned int max_ytilt = packet[7] & 127;

	printf("Stylus info: data_id=%u version=%u max_x=%u max_y=%u max_pressure=%u "
	    "max_xtilt=%u max_ytilt=%u\n", data_id, version, max_x, max_y,
	    max_pressure, max_xtilt, max_ytilt);
	
	return false;
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

static bool parse_response_touch(uint8_t *packet, size_t size,
    isdv4_state_t *state)
{
	if (size != 11) {
		fprintf(stderr, "Unexpected length of touch response packet\n");
		return false;
	}
	bool control_packet = ((packet[0] & CONTROL_PACKET) > 0);
	if (!control_packet) {
		fprintf(stderr, "This is not a control packet\n");
		return false;
	}

	state->touch_type = (packet[0] & 63);
	unsigned int version = ((packet[9] & 127) << 7) | (packet[10] & 127);

	unsigned int touch_resolution = packet[1] & 127;
	state->touch_max_x = ((packet[2] >> 5) & 3) | ((packet[3] & 127) << 7) |
	    (packet[4] & 124);
	state->touch_max_y = ((packet[2] >> 3) & 3) | ((packet[5] & 127) << 7) |
	    (packet[6] & 124);
	
	if (touch_resolution == 0)
		touch_resolution = 10;

	if (state->touch_max_x == 0 || state->touch_max_y == 0) {
		state->touch_max_x = (1 << touch_resolution);
		state->touch_max_y = (1 << touch_resolution);
	}

	printf("Touch info: data_id=%u (%s) version=%u max_x=%u "
	    "max_y=%u\n", state->touch_type, touch_type(state->touch_type), version,
	    state->touch_max_x, state->touch_max_y);
	return false;
}

static void read_packets(isdv4_state_t *state, packet_consumer_fn consumer)
{
	bool reading = true;
	size_t packet_remaining = 1;
	while (reading) {
		if (packet_remaining == 0) packet_remaining = 1;
		ssize_t read = char_dev_read(state->sess, state->buf + state->buf_end,
		    state->buf_size - state->buf_end);
		if (read < 0) {
			fprintf(stderr, "Failed reading from serial device\n");
			return;
		}
		state->buf_end += read;
		
		size_t i = 0;
		
		/* Skip data until a start of packet is found */
		while (i < state->buf_end && (state->buf[i] & START_OF_PACKET) == 0) i++;
		
		size_t start = i;
		size_t end = i;
		size_t processed_end = i;
		
		/* Process packets one by one */
		while (reading && i < state->buf_end) {
			/* Determine the packet length */
			if (state->buf[i] & CONTROL_PACKET) {
				packet_remaining = 11;
			}
			else if (state->buf[i] & TOUCH_EVENT) {
				packet_remaining = 5;
			}
			else {
				packet_remaining = 9;
			}
			
			/* Find the end of the packet */
			i++; /* We need to skip the first byte with START_OF_PACKET set */
			packet_remaining--; 
			while (packet_remaining > 0 && i < state->buf_end &&
			    (state->buf[i] & START_OF_PACKET) == 0) {
				i++;
				packet_remaining--;
			}
			end = i;
			
			/* If we have whole packet, process it */
			if (end > start && packet_remaining == 0) {
				reading = consumer(state->buf + start, end - start, state);
				start = end;
				processed_end = end;
			}
		}
		
		if (processed_end == 0 && state->buf_end == BUF_SIZE) {
			fprintf(stderr, "Buffer overflow detected, discarding contents\n");
			state->buf_end = 0;
		}
		
		/* Shift the buffer contents to the left */
		size_t unprocessed_len = state->buf_end - processed_end;
		memcpy(state->buf, state->buf + processed_end, unprocessed_len);
		state->buf_end = unprocessed_len;
	}
}
static bool write_command(async_sess_t *sess, uint8_t command)
{
	return char_dev_write(sess, &command, 1) == 1;
}

static void isdv4_init(isdv4_state_t *state, async_sess_t *sess,
    uint8_t *buf, size_t buf_size, isdv4_event_fn event_fn)
{
	memset(state, 0, sizeof(isdv4_state_t));
	state->sess = sess;
	state->buf = buf;
	state->buf_size = buf_size;
	state->emit_event_fn = event_fn;
}

static void isdv4_init_tablet(isdv4_state_t *state)
{
	write_command(state->sess, CMD_STOP);
	usleep(250000); /* 250 ms */
	// FIXME: Read all possible garbage before sending commands
	write_command(state->sess, CMD_QUERY_STYLUS);
	read_packets(state, parse_response_stylus);
	write_command(state->sess, CMD_QUERY_TOUCH);
	read_packets(state, parse_response_touch);
	write_command(state->sess, CMD_START);
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
	
	uint8_t buf[BUF_SIZE];
	
	isdv4_state_t state;
	isdv4_init(&state, sess, buf, BUF_SIZE, print_event);
	isdv4_init_tablet(&state);
	
	read_packets(&state, parse_event);
	
	return 0;
}
