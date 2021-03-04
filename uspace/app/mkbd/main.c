/*
 * Copyright (c) 2011 Lubos Slovak
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

/** @addtogroup mkbd
 * @{
 */
/**
 * @file
 * Sample application using the data from multimedia keys on keyboard
 */

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <fibril.h>
#include <str_error.h>
#include <stdbool.h>
#include <getopt.h>
#include <devman.h>
#include <loc.h>
#include <usbhid_iface.h>
#include <usb/dev/pipes.h>
#include <async.h>
#include <usb/dev.h>
#include <usb/hid/usages/core.h>
#include <usb/hid/hidparser.h>
#include <usb/hid/hiddescriptor.h>
#include <usb/hid/usages/consumer.h>
#include <io/console.h>
#include <io/keycode.h>
#include <assert.h>

#define NAME "mkbd"

static async_sess_t *dev_sess = NULL;

static errno_t initialize_report_parser(async_sess_t *dev_sess,
    usb_hid_report_t **report)
{
	*report = (usb_hid_report_t *) malloc(sizeof(usb_hid_report_t));
	if (*report == NULL)
		return ENOMEM;

	errno_t rc = usb_hid_report_init(*report);
	if (rc != EOK) {
		usb_hid_report_deinit(*report);
		*report = NULL;
		return rc;
	}

	/* Get the report descriptor length from the device */
	size_t report_desc_size;
	rc = usbhid_dev_get_report_descriptor_length(dev_sess,
	    &report_desc_size);
	if (rc != EOK) {
		usb_hid_report_deinit(*report);
		*report = NULL;
		return rc;
	}

	if (report_desc_size == 0) {
		usb_hid_report_deinit(*report);
		*report = NULL;
		// TODO: other error code?
		return EINVAL;
	}

	uint8_t *desc = (uint8_t *) malloc(report_desc_size);
	if (desc == NULL) {
		usb_hid_report_deinit(*report);
		*report = NULL;
		return ENOMEM;
	}

	/* Get the report descriptor from the device */
	size_t actual_size;
	rc = usbhid_dev_get_report_descriptor(dev_sess, desc, report_desc_size,
	    &actual_size);
	if (rc != EOK) {
		usb_hid_report_deinit(*report);
		*report = NULL;
		free(desc);
		return rc;
	}

	if (actual_size != report_desc_size) {
		usb_hid_report_deinit(*report);
		*report = NULL;
		free(desc);
		// TODO: other error code?
		return EINVAL;
	}

	/* Initialize the report parser */

	rc = usb_hid_parse_report_descriptor(*report, desc, report_desc_size);
	free(desc);

	return rc;
}

static void print_key(uint8_t *buffer, size_t size, usb_hid_report_t *report)
{
	assert(buffer != NULL);
	assert(report != NULL);

	uint8_t report_id;
	errno_t rc = usb_hid_parse_report(report, buffer, size, &report_id);
	if (rc != EOK)
		return;

	usb_hid_report_path_t *path = usb_hid_report_path();
	if (path == NULL) {
		return;
	}

	usb_hid_report_path_append_item(path, USB_HIDUT_PAGE_CONSUMER, 0);

	usb_hid_report_path_set_report_id(path, report_id);

	usb_hid_report_field_t *field = usb_hid_report_get_sibling(
	    report, NULL, path, USB_HID_PATH_COMPARE_END |
	    USB_HID_PATH_COMPARE_USAGE_PAGE_ONLY,
	    USB_HID_REPORT_TYPE_INPUT);

	while (field != NULL) {
		if (field->value != 0) {
			const char *key_str =
			    usbhid_multimedia_usage_to_str(field->usage);
			printf("Pressed key: %s\n", key_str);
		}

		field = usb_hid_report_get_sibling(
		    report, field, path, USB_HID_PATH_COMPARE_END |
		    USB_HID_PATH_COMPARE_USAGE_PAGE_ONLY,
		    USB_HID_REPORT_TYPE_INPUT);
	}

	usb_hid_report_path_free(path);
}

static errno_t wait_for_quit_fibril(void *arg)
{
	console_ctrl_t *con = console_init(stdin, stdout);

	printf("Press <ESC> to quit the application.\n");

	while (true) {
		cons_event_t ev;
		errno_t rc = console_get_event(con, &ev);
		if (rc != EOK) {
			printf("Connection with console broken: %s.\n",
			    str_error(errno));
			break;
		}

		if (ev.type == CEV_KEY && ev.ev.key.type == KEY_PRESS &&
		    ev.ev.key.key == KC_ESCAPE) {
			break;
		}
	}

	console_done(con);

	exit(0);

	return EOK;
}

#define MAX_PATH_LENGTH 1024

static void print_usage(char *app_name)
{
#define _INDENT "      "
	printf(NAME ": Print out what multimedia keys were pressed.\n\n");
	printf("Usage: %s device\n", app_name);
	printf(_INDENT "The device is a devman path to the device.\n");
#undef _OPTION
#undef _INDENT
}

int main(int argc, char *argv[])
{
	int act_event = -1;

	if (argc <= 1) {
		print_usage(argv[0]);
		return -1;
	}

	char *devpath = argv[1];

	devman_handle_t dev_handle = 0;

	errno_t rc = usb_resolve_device_handle(devpath, &dev_handle);
	if (rc != EOK) {
		printf("Device not found or not of USB kind: %s.\n",
		    str_error(rc));
		return rc;
	}

	async_sess_t *sess = devman_device_connect(dev_handle, 0);
	if (!sess) {
		printf(NAME ": failed to connect to the device (handle %"
		    PRIun "): %s.\n", dev_handle, str_error(errno));
		return errno;
	}

	dev_sess = sess;

	char path[MAX_PATH_LENGTH];
	rc = devman_fun_get_path(dev_handle, path, MAX_PATH_LENGTH);
	if (rc != EOK) {
		printf(NAME ": failed to get path (handle %"
		    PRIun "): %s.\n", dev_handle, str_error(errno));
		return ENOMEM;
	}

	printf("Device path: %s\n", path);

	usb_hid_report_t *report = NULL;
	rc = initialize_report_parser(dev_sess, &report);
	if (rc != EOK) {
		printf("Failed to initialize report parser: %s\n",
		    str_error(rc));
		return rc;
	}

	assert(report != NULL);

	size_t size;
	rc = usbhid_dev_get_event_length(dev_sess, &size);
	if (rc != EOK) {
		printf("Failed to get event length: %s.\n", str_error(rc));
		return rc;
	}

	uint8_t *event = (uint8_t *)malloc(size);
	if (event == NULL) {
		printf("Out of memory.\n");
		// TODO: hangup phone?
		return ENOMEM;
	}

	fid_t quit_fibril = fibril_create(wait_for_quit_fibril, NULL);
	if (quit_fibril == 0) {
		printf("Failed to start extra fibril.\n");
		return -1;
	}
	fibril_add_ready(quit_fibril);

	size_t actual_size;
	int event_nr;

	while (true) {
		/** @todo Try blocking call. */
		rc = usbhid_dev_get_event(dev_sess, event, size, &actual_size,
		    &event_nr, 0);
		if (rc != EOK) {
			// TODO: hangup phone?
			printf("Error in getting event from the HID driver:"
			    "%s.\n", str_error(rc));
			break;
		}

		if (event_nr > act_event) {
			print_key(event, size, report);
			act_event = event_nr;
		}

		fibril_usleep(10000);
	}

	return 0;
}

/** @}
 */
