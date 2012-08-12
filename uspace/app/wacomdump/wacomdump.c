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

#define BUF_SIZE 32

#define START_OF_PACKET 128
#define CONTROL_PACKET 64
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

/* packet_consumer_fn(uint8_t *packet, size_t size)
   return true if reading of packets should continue */
typedef bool (*packet_consumer_fn)(uint8_t *, size_t);

static void syntax_print(void)
{
	fprintf(stderr, "Usage: wacomdump [--baud=<baud>] [device_service]\n");
}

static bool parse_packet(uint8_t *packet, size_t size)
{
	if (size < 1) {
		printf("Invalid packet size");
		return false;
	}

	bool control_packet = ((packet[0] & CONTROL_PACKET) > 0);
	if (!control_packet) {
		/* This is an event initiated by the device */
		printf("Event");
		if (size == 5 || size == 7) {
			printf(" touch");
			if (packet[0] & FINGER1) {
				printf(" finger1");
			}
			if (packet[0] & FINGER2) {
				printf(" finger2");
			}
			int x = ((packet[1] & 127) << 7) | (packet[2] & 127);
			int y = ((packet[3] & 127) << 7) | (packet[4] & 127);
			printf(" x=%d y=%d", x, y);
		}
		else if (size == 9) {
			printf(" stylus");
			if (packet[0] & TIP) {
				printf(" tip");
			}
			if (packet[0] & BUTTON1) {
				printf(" button1");
			}
			if (packet[0] & BUTTON2) {
				printf(" button2");
			}
			if (packet[0] & PROXIMITY) {
				printf(" proximity");
			}
			int x = ((packet[1] & 127) << 7) | (packet[2] & 124) | ((packet[6] >> 5) & 3);
			int y = ((packet[3] & 127) << 7) | (packet[4] & 124) | ((packet[6] >> 3) & 3);
			int xtilt = (packet[8] & 127);
			int ytilt = (packet[7] & 127);
			printf(" x=%d y=%d xtilt=%d ytilt=%d", x, y, xtilt, ytilt);
		}
	}
	else {
		printf("Response");
	}

	printf("\n");
	return true;
}

static bool parse_response_stylus(uint8_t *packet, size_t size)
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

static bool parse_response_touch(uint8_t *packet, size_t size)
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

	unsigned int data_id = (packet[0] & 63);
	unsigned int version = ((packet[9] & 127) << 7) | (packet[10] & 127);

	unsigned int touch_resolution = packet[1] & 127;
	unsigned int sensor_id = packet[2] & 7;
	unsigned int max_x = ((packet[2] >> 5) & 3) | ((packet[3] & 127) << 7) |
	    (packet[4] & 124);
	unsigned int max_y = ((packet[2] >> 3) & 3) | ((packet[5] & 127) << 7) |
	    (packet[6] & 124);
	unsigned int capacitance_res = packet[7] & 127;

	if (touch_resolution == 0)
		touch_resolution = 10;

	if (max_x == 0 || max_y == 0) {
		max_x = (1 << touch_resolution);
		max_y = (1 << touch_resolution);
	}

	printf("Touch info: data_id=%u (%s) version=%u sensor_id=%u max_x=%u "
	    "max_y=%u capacitance_res=%u\n", data_id, touch_type(data_id), version,
	    sensor_id, max_x, max_y, capacitance_res);
	return false;
}

static void read_packets(async_sess_t *sess, packet_consumer_fn consumer)
{
	uint8_t buf[BUF_SIZE];
	ssize_t buf_end = 0;
	
	while (true) {
		ssize_t read = char_dev_read(sess, buf + buf_end, BUF_SIZE - buf_end);
		if (read < 0) {
			fprintf(stderr, "Failed reading from serial device\n");
			return;
		}
		buf_end += read;
		
		ssize_t i = 0;
		
		/* Skip data until a start of packet is found */
		while (i < buf_end && (buf[i] & START_OF_PACKET) == 0) i++;
		
		ssize_t start = i;
		ssize_t end = i;
		ssize_t processed_end = i;
		
		/* Process packets one by one */
		while (i < buf_end) {
			/* Find a start of next packet */
			i++; /* We need to skip the first byte with START_OF_PACKET set */
			while (i < buf_end && (buf[i] & START_OF_PACKET) == 0) i++;
			end = i;
			
			/* If we have whole packet, process it */
			if (end - start > 0 && (end != buf_end || read == 0)) {
				if (!consumer(buf + start, end - start)) {
					return;
				}
				start = end;
				processed_end = end;
			}
		}
		
		if (processed_end == 0 && buf_end == BUF_SIZE) {
			fprintf(stderr, "Buffer overflow detected, discarding contents\n");
			buf_end = 0;
		}
		
		/* Shift the buffer contents to the left */
		size_t unprocessed_len = buf_end - processed_end;
		memcpy(buf, buf + processed_end, unprocessed_len);
		buf_end = unprocessed_len;
	}
	
	/* not reached */
}
static bool write_command(async_sess_t *sess, uint8_t command)
{
	return char_dev_write(sess, &command, 1) == 1;
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
	
	write_command(sess, CMD_STOP);
	usleep(250000); /* 250 ms */
	uint8_t buf[BUF_SIZE];
	while (char_dev_read(sess, buf, BUF_SIZE) > 0);
	write_command(sess, CMD_QUERY_STYLUS);
	read_packets(sess, parse_response_stylus);
	write_command(sess, CMD_QUERY_TOUCH);
	read_packets(sess, parse_response_touch);
	write_command(sess, CMD_START);
	read_packets(sess, parse_packet);
	
	return 0;
}
