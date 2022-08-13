/*
 * SPDX-FileCopyrightText: 2011 Martin Sucha
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <errno.h>
#include <io/chardev.h>
#include <io/serial.h>
#include <loc.h>
#include <stdio.h>
#include <stdlib.h>
#include <str.h>

#define BUF_SIZE 1

static void syntax_print(void)
{
	fprintf(stderr, "Usage: sportdmp [--baud=<baud>] [device_service]\n");
}

int main(int argc, char **argv)
{
	sysarg_t baud = 9600;
	service_id_t svc_id;
	chardev_t *chardev;
	serial_t *serial;
	size_t nread;

	int arg = 1;
	errno_t rc;

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

	if (argc > arg) {
		rc = loc_service_get_id(argv[arg], &svc_id, 0);
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
		free(svc_ids);
	}

	if (argc > arg) {
		fprintf(stderr, "Too many arguments\n");
		syntax_print();
		return 1;
	}

	async_sess_t *sess = loc_service_connect(svc_id, INTERFACE_DDF,
	    IPC_FLAG_BLOCKING);
	if (sess == NULL) {
		fprintf(stderr, "Failed connecting to service\n");
		return 2;
	}

	rc = chardev_open(sess, &chardev);
	if (rc != EOK) {
		fprintf(stderr, "Failed opening character device\n");
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

	uint8_t *buf = (uint8_t *) malloc(BUF_SIZE);
	if (buf == NULL) {
		fprintf(stderr, "Failed allocating buffer\n");
		return 3;
	}

	while (true) {
		rc = chardev_read(chardev, buf, BUF_SIZE, &nread,
		    chardev_f_none);
		for (size_t i = 0; i < nread; i++) {
			printf("%02hhx ", buf[i]);
		}
		if (rc != EOK) {
			fprintf(stderr, "\nFailed reading from serial device\n");
			break;
		}
		fflush(stdout);
	}

	free(buf);
	serial_close(serial);
	chardev_close(chardev);
	async_hangup(sess);
	return 0;
}
