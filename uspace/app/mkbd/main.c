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
#include <str_error.h>
#include <bool.h>
#include <getopt.h>
#include <devman.h>
#include <devmap.h>
#include <usb/dev/hub.h>
//#include <usb/host.h>
//#include <usb/driver.h>
#include <usb/hid/iface.h>
#include <usb/dev/pipes.h>
#include <async.h>
#include <usb/hid/usages/core.h>
#include <usb/hid/hidparser.h>
#include <usb/hid/hiddescriptor.h>
#include <usb/hid/usages/consumer.h>
#include <assert.h>

#define NAME "mkbd"

static int dev_phone = -1;

static int initialize_report_parser(int dev_phone, usb_hid_report_t **report)
{
	*report = (usb_hid_report_t *)malloc(sizeof(usb_hid_report_t));
	if (*report == NULL) {
		return ENOMEM;
	}
	
	int rc = usb_hid_report_init(*report);
	if (rc != EOK) {
		usb_hid_free_report(*report);
		*report = NULL;
		//printf("usb_hid_report_init() failed.\n");
		return rc;
	}
	
	// get the report descriptor length from the device
	size_t report_desc_size;
	rc = usbhid_dev_get_report_descriptor_length(
	    dev_phone, &report_desc_size);
	if (rc != EOK) {
		usb_hid_free_report(*report);
		*report = NULL;
		//printf("usbhid_dev_get_report_descriptor_length() failed.\n");
		return rc;
	}
	
	if (report_desc_size == 0) {
		usb_hid_free_report(*report);
		*report = NULL;
		//printf("usbhid_dev_get_report_descriptor_length() returned 0.\n");
		return EINVAL;	// TODO: other error code?
	}
	
	uint8_t *desc = (uint8_t *)malloc(report_desc_size);
	if (desc == NULL) {
		usb_hid_free_report(*report);
		*report = NULL;
		return ENOMEM;
	}
	
	// get the report descriptor from the device
	size_t actual_size;
	rc = usbhid_dev_get_report_descriptor(dev_phone, desc, report_desc_size,
	    &actual_size);
	if (rc != EOK) {
		usb_hid_free_report(*report);
		*report = NULL;
		free(desc);
		//printf("usbhid_dev_get_report_descriptor() failed.\n");
		return rc;
	}
	
	if (actual_size != report_desc_size) {
		usb_hid_free_report(*report);
		*report = NULL;
		free(desc);
//		printf("usbhid_dev_get_report_descriptor() returned wrong size:"
//		    " %zu, expected: %zu.\n", actual_size, report_desc_size);
		return EINVAL;	// TODO: other error code?
	}
	
	// initialize the report parser
	
	rc = usb_hid_parse_report_descriptor(*report, desc, report_desc_size);
	free(desc);
	
	if (rc != EOK) {
		free(desc);
//		printf("usb_hid_parse_report_descriptor() failed.\n");
		return rc;
	}
	
	return EOK;
}

static void print_key(uint8_t *buffer, size_t size, usb_hid_report_t *report)
{
	assert(buffer != NULL);
	assert(report != NULL);
	
//	printf("Calling usb_hid_parse_report() with size %zu and "
//	    "buffer: \n", size);
//	for (size_t i = 0; i < size; ++i) {
//		printf(" %X ", buffer[i]);
//	}
//	printf("\n");
	
	uint8_t report_id;
	int rc = usb_hid_parse_report(report, buffer, size, &report_id);
	if (rc != EOK) {
//		printf("Error parsing report: %s\n", str_error(rc));
		return;
	}
	
	usb_hid_report_path_t *path = usb_hid_report_path();
	if (path == NULL) {
		return;
	}
	
	usb_hid_report_path_append_item(path, USB_HIDUT_PAGE_CONSUMER, 0);
	
	usb_hid_report_path_set_report_id(path, report_id);

	usb_hid_report_field_t *field = usb_hid_report_get_sibling(
	    report, NULL, path, USB_HID_PATH_COMPARE_END 
	    | USB_HID_PATH_COMPARE_USAGE_PAGE_ONLY, 
	    USB_HID_REPORT_TYPE_INPUT);
	
//	printf("Field: %p\n", field);
	
	while (field != NULL) {
//		printf("Field usage: %u, field value: %d\n", field->usage, 
//		    field->value);
		if (field->value != 0) {
			const char *key_str = 
			    usbhid_multimedia_usage_to_str(field->usage);
			printf("Pressed key: %s\n", key_str);
		}
		
		field = usb_hid_report_get_sibling(
		    report, field, path, USB_HID_PATH_COMPARE_END
		    | USB_HID_PATH_COMPARE_USAGE_PAGE_ONLY, 
		    USB_HID_REPORT_TYPE_INPUT);
//		printf("Next field: %p\n", field);
	}
	
	usb_hid_report_path_free(path);
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
	//const char *devpath = "/hw/pci0/00:06.0/ohci-rh/usb00_a2/HID1/hid";
	
	int rc;
	
	devman_handle_t dev_handle = 0;
	rc = devman_device_get_handle(devpath, &dev_handle, 0);
	if (rc != EOK) {
		printf("Failed to get handle from devman: %s.\n",
		    str_error(rc));
		return rc;
	}
	
	rc = devman_device_connect(dev_handle, 0);
	if (rc < 0) {
		printf(NAME ": failed to connect to the device (handle %"
		       PRIun "): %s.\n", dev_handle, str_error(rc));
		return rc;
	}
	
	dev_phone = rc;
//	printf("Got phone to the device: %d\n", dev_phone);
	
	char path[MAX_PATH_LENGTH];
	rc = devman_get_device_path(dev_handle, path, MAX_PATH_LENGTH);
	if (rc != EOK) {
		return ENOMEM;
	}
	
	printf("Device path: %s\n", path);
	
	
	usb_hid_report_t *report = NULL;
	rc = initialize_report_parser(dev_phone, &report);
	if (rc != EOK) {
		printf("Failed to initialize report parser: %s\n",
		    str_error(rc));
		return rc;
	}
	
	assert(report != NULL);
	
	size_t size;
	rc = usbhid_dev_get_event_length(dev_phone, &size);
	if (rc != EOK) {
		printf("Failed to get event length: %s.\n", str_error(rc));
		return rc;
	}
	
//	printf("Event length: %zu\n", size);
	uint8_t *event = (uint8_t *)malloc(size);
	if (event == NULL) {
		// hangup phone?
		return ENOMEM;
	}
	
//	printf("Event length: %zu\n", size);
	
	size_t actual_size;
	int event_nr;
	
	while (1) {
		// get event from the driver
//		printf("Getting event from the driver.\n");
		
		/** @todo Try blocking call. */
		rc = usbhid_dev_get_event(dev_phone, event, size, &actual_size, 
		    &event_nr, 0);
		if (rc != EOK) {
			// hangup phone?
			printf("Error in getting event from the HID driver:"
			    "%s.\n", str_error(rc));
			break;
		}
		
//		printf("Got buffer: %p, size: %zu, max size: %zu\n", event, 
//		    actual_size, size);
		
//		printf("Event number: %d, my actual event: %d\n", event_nr, 
//		    act_event);
		if (event_nr > act_event) {
			print_key(event, size, report);
			act_event = event_nr;
		}
		
		async_usleep(10000);
	}
	
	return 0;
}


/** @}
 */
