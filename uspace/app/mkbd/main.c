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
#include <usb/host.h>
#include <usb/driver.h>
#include <usb/hid/iface.h>
#include <usb/dev/pipes.h>

#define NAME "mkbd"

static int dev_phone = -1;

//static void print_found_hc(size_t class_index, const char *path)
//{
//	// printf(NAME ": host controller %zu is `%s'.\n", class_index, path);
//	printf("Bus %02zu: %s\n", class_index, path);
//}
//static void print_found_dev(usb_address_t addr, const char *path)
//{
//	// printf(NAME ":     device with address %d is `%s'.\n", addr, path);
//	printf("  Device %02d: %s\n", addr, path);
//}

//static void print_hc_devices(devman_handle_t hc_handle)
//{
//	int rc;
//	usb_hc_connection_t conn;

//	usb_hc_connection_initialize(&conn, hc_handle);
//	rc = usb_hc_connection_open(&conn);
//	if (rc != EOK) {
//		printf(NAME ": failed to connect to HC: %s.\n",
//		    str_error(rc));
//		return;
//	}
//	usb_address_t addr;
//	for (addr = 1; addr < 5; addr++) {
//		devman_handle_t dev_handle;
//		rc = usb_hc_get_handle_by_address(&conn, addr, &dev_handle);
//		if (rc != EOK) {
//			continue;
//		}
//		char path[MAX_PATH_LENGTH];
//		rc = devman_get_device_path(dev_handle, path, MAX_PATH_LENGTH);
//		if (rc != EOK) {
//			continue;
//		}
//		print_found_dev(addr, path);
//	}
//	usb_hc_connection_close(&conn);
//}

//static bool try_parse_class_and_address(const char *path,
//    devman_handle_t *out_hc_handle, usb_address_t *out_device_address)
//{
//	size_t class_index;
//	size_t address;
//	int rc;
//	char *ptr;

//	rc = str_size_t(path, &ptr, 10, false, &class_index);
//	if (rc != EOK) {
//		return false;
//	}
//	if ((*ptr == ':') || (*ptr == '.')) {
//		ptr++;
//	} else {
//		return false;
//	}
//	rc = str_size_t(ptr, NULL, 10, true, &address);
//	if (rc != EOK) {
//		return false;
//	}
//	rc = usb_ddf_get_hc_handle_by_class(class_index, out_hc_handle);
//	if (rc != EOK) {
//		return false;
//	}
//	if (out_device_address != NULL) {
//		*out_device_address = (usb_address_t) address;
//	}
//	return true;
//}

//static bool resolve_hc_handle_and_dev_addr(const char *devpath,
//    devman_handle_t *out_hc_handle, usb_address_t *out_device_address)
//{
//	int rc;

//	/* Hack for QEMU to save-up on typing ;-). */
//	if (str_cmp(devpath, "qemu") == 0) {
//		devpath = "/hw/pci0/00:01.2/uhci-rh/usb00_a1";
//	}

//	/* Hack for virtual keyboard. */
//	if (str_cmp(devpath, "virt") == 0) {
//		devpath = "/virt/usbhc/usb00_a1/usb00_a2";
//	}

//	if (try_parse_class_and_address(devpath,
//	    out_hc_handle, out_device_address)) {
//		return true;
//	}

//	char *path = str_dup(devpath);
//	if (path == NULL) {
//		return ENOMEM;
//	}

//	devman_handle_t hc = 0;
//	bool hc_found = false;
//	usb_address_t addr = 0;
//	bool addr_found = false;

//	/* Remove suffixes and hope that we will encounter device node. */
//	while (str_length(path) > 0) {
//		/* Get device handle first. */
//		devman_handle_t dev_handle;
//		rc = devman_device_get_handle(path, &dev_handle, 0);
//		if (rc != EOK) {
//			free(path);
//			return false;
//		}

//		/* Try to find its host controller. */
//		if (!hc_found) {
//			rc = usb_hc_find(dev_handle, &hc);
//			if (rc == EOK) {
//				hc_found = true;
//			}
//		}
//		/* Try to get its address. */
//		if (!addr_found) {
//			addr = usb_device_get_assigned_address(dev_handle);
//			if (addr >= 0) {
//				addr_found = true;
//			}
//		}

//		/* Speed-up. */
//		if (hc_found && addr_found) {
//			break;
//		}

//		/* Remove the last suffix. */
//		char *slash_pos = str_rchr(path, '/');
//		if (slash_pos != NULL) {
//			*slash_pos = 0;
//		}
//	}

//	free(path);

//	if (hc_found && addr_found) {
//		if (out_hc_handle != NULL) {
//			*out_hc_handle = hc;
//		}
//		if (out_device_address != NULL) {
//			*out_device_address = addr;
//		}
//		return true;
//	} else {
//		return false;
//	}
//}

static void print_usage(char *app_name)
{
#define _INDENT "      "

	printf(NAME ": Print out what multimedia keys were pressed.\n\n");
	printf("Usage: %s device\n", app_name);
	printf(_INDENT "The device is a devman path to the device.\n");

#undef _OPTION
#undef _INDENT
}

#define MAX_PATH_LENGTH 1024

int main(int argc, char *argv[])
{
	
	if (argc <= 1) {
		print_usage(argv[0]);
		return -1;
	}
	
	char *devpath = argv[1];

//	/* The initialization is here only to make compiler happy. */
//	devman_handle_t hc_handle = 0;
//	usb_address_t dev_addr = 0;
//	bool found = resolve_hc_handle_and_dev_addr(devpath,
//	    &hc_handle, &dev_addr);
//	if (!found) {
//		fprintf(stderr, NAME ": device `%s' not found "
//		    "or not of USB kind. Exiting.\n", devpath);
//		return -1;
//	}
	
//	int rc;
//	usb_hc_connection_t conn;

//	usb_hc_connection_initialize(&conn, hc_handle);
//	rc = usb_hc_connection_open(&conn);
//	if (rc != EOK) {
//		printf(NAME ": failed to connect to HC: %s.\n",
//		    str_error(rc));
//		return -1;
//	}

//	devman_handle_t dev_handle;
//	rc = usb_hc_get_handle_by_address(&conn, dev_addr, &dev_handle);
//	if (rc != EOK) {
//		printf(NAME ": failed getting handle to the device: %s.\n",
//		       str_error(rc));
//		return -1;
//	}

//	usb_hc_connection_close(&conn);
	
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
	printf("Got phone to the device: %d\n", dev_phone);
	
	char path[MAX_PATH_LENGTH];
	rc = devman_get_device_path(dev_handle, path, MAX_PATH_LENGTH);
	if (rc != EOK) {
		return ENOMEM;
	}
	
	printf("Device path: %s\n", path);
	
	size_t size;
	rc = usbhid_dev_get_event_length(dev_phone, &size);
	if (rc != EOK) {
		printf("Failed to get event length: %s.\n",
		    str_error(rc));
		return rc;
	}
	
	printf("Event length: %zu\n", size);
	int32_t *event = (int32_t *)malloc(size);
	if (event == NULL) {
		// hangup phone?
		return ENOMEM;
	}
	
	printf("Event length: %zu\n", size);
	
	size_t actual_size;
	
	while (1) {
		// get event from the driver
		printf("Getting event from the driver.\n");
		rc = usbhid_dev_get_event(dev_phone, event, size, &actual_size, 
		    0);
		if (rc != EOK) {
			// hangup phone?
			printf("Error in getting event from the HID driver:"
			    "%s.\n", str_error(rc));
			break;
		}
		
		printf("Got buffer: %p, size: %zu\n", event, actual_size);
	}
	
	return 0;
}


/** @}
 */
