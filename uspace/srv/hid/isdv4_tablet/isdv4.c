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

#include <async.h>
#include <errno.h>
#include <io/chardev.h>
#include <mem.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include "isdv4.h"

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

/* packet_consumer_fn(uint8_t *packet, size_t size, isdv4_state_t *state,
   void *data)
   return true if reading of packets should continue */
typedef bool (*packet_consumer_fn)(uint8_t *, size_t, isdv4_state_t *);

static void isdv4_event_init(isdv4_event_t *event)
{
	memset(event, 0, sizeof(isdv4_event_t));
}

/**
 * Parse event packet and emit events
 * @return true if reading of packets should continue
 */
static bool parse_event(uint8_t *packet, size_t size, isdv4_state_t *state)
{
	if (size < 1)
		return false;

	bool control_packet = ((packet[0] & CONTROL_PACKET) > 0);
	if (control_packet)
		return true;

	/* This is an event initiated by the device */
	isdv4_event_t event;
	isdv4_event_init(&event);

	if (packet[0] & TOUCH_EVENT) {
		if (size != 5)
			return true;

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
	else {
		if (size != 9)
			return true;

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
			event.button = 0;
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
			event.button = 0;
			state->emit_event_fn(&event);
		}
	}

	return true;
}

static bool parse_response_stylus(uint8_t *packet, size_t size,
    isdv4_state_t *state)
{
	if (size < 1)
		return false;

	bool control_packet = ((packet[0] & CONTROL_PACKET) > 0);
	if (!control_packet)
		return true;

	if (size != 11)
		return false;

	state->stylus_max_x = ((packet[1] & 127) << 7) | (packet[2] & 124) |
	    ((packet[6] >> 5) & 3);
	state->stylus_max_y = ((packet[3] & 127) << 7) | (packet[4] & 124) |
	    ((packet[6] >> 3) & 3);
	state->stylus_max_pressure = (packet[5] & 63) | ((packet[6] & 7) << 7);
	state->stylus_max_xtilt = packet[8] & 127;
	state->stylus_max_ytilt = packet[7] & 127;
	state->stylus_tilt_supported = (state->stylus_max_xtilt &&
	    state->stylus_max_ytilt);

	return false;
}

static bool parse_response_touch(uint8_t *packet, size_t size,
    isdv4_state_t *state)
{
	if (size < 1)
		return false;

	bool control_packet = ((packet[0] & CONTROL_PACKET) > 0);
	if (!control_packet)
		return true;

	if (size != 11)
		return false;

	state->touch_type = (packet[0] & 63);

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

	return false;
}

static errno_t read_packets(isdv4_state_t *state, packet_consumer_fn consumer)
{
	bool reading = true;
	while (reading) {
		size_t nread;
		errno_t rc;

		rc = chardev_read(state->chardev, state->buf + state->buf_end,
		    state->buf_size - state->buf_end, &nread);
		if (rc != EOK && nread == 0)
			return EIO;
		state->buf_end += nread;

		size_t i = 0;

		/* Skip data until a start of packet is found */
		while (i < state->buf_end && (state->buf[i] & START_OF_PACKET) == 0) i++;

		size_t start = i;
		size_t end = i;
		size_t processed_end = i;

		/* Process packets one by one */
		while (reading && i < state->buf_end) {
			/* Determine the packet length */
			size_t packet_remaining;
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

		if (processed_end == 0 && state->buf_end == state->buf_size) {
			/* Packet too large, throw it away */
			state->buf_end = 0;
		}

		/* Shift the buffer contents to the left */
		size_t unprocessed_len = state->buf_end - processed_end;
		memcpy(state->buf, state->buf + processed_end, unprocessed_len);
		state->buf_end = unprocessed_len;
	}
	return EOK;
}

static bool write_command(chardev_t *chardev, uint8_t command)
{
	errno_t rc;
	size_t nwr;

	rc = chardev_write(chardev, &command, 1, &nwr);
	return rc == EOK;
}

errno_t isdv4_init(isdv4_state_t *state, async_sess_t *sess,
    isdv4_event_fn event_fn)
{
	chardev_t *chardev;
	errno_t rc;

	rc = chardev_open(sess, &chardev);
	if (rc != EOK)
		return rc;

	memset(state, 0, sizeof(isdv4_state_t));

	state->sess = sess;
	state->chardev = chardev;

	state->buf = malloc(BUF_SIZE);
	if (state->buf == NULL) {
		chardev_close(chardev);
		return ENOMEM;
	}

	state->buf_size = BUF_SIZE;
	state->emit_event_fn = event_fn;
	return EOK;
}

errno_t isdv4_init_tablet(isdv4_state_t *state)
{
	if (!write_command(state->chardev, CMD_STOP))
		return EIO;

	async_usleep(250000); /* 250 ms */

	// FIXME: Read all possible garbage before sending commands
	if (!write_command(state->chardev, CMD_QUERY_STYLUS))
		return EIO;

	errno_t rc = read_packets(state, parse_response_stylus);
	if (rc != EOK)
		return rc;

	if (!write_command(state->chardev, CMD_QUERY_TOUCH))
		return EIO;

	rc = read_packets(state, parse_response_touch);
	if (rc != EOK)
		return rc;

	if (!write_command(state->chardev, CMD_START))
		return EIO;

	return EOK;
}

errno_t isdv4_read_events(isdv4_state_t *state)
{
	return read_packets(state, parse_event);
}

void isdv4_fini(isdv4_state_t *state)
{
	free(state->buf);
}
