/*
 * Copyright (c) 2010-2011 Vojtech Horky
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

/** @addtogroup lsusb
 * @{
 */
/**
 * @file
 * Listing of USB host controllers.
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
#include <usb/hc.h>

#define NAME "lsusb"

#define MAX_USB_ADDRESS USB11_ADDRESS_MAX
#define MAX_FAILED_ATTEMPTS 10
#define MAX_PATH_LENGTH 1024

static void print_found_hc(size_t class_index, const char *path)
{
	// printf(NAME ": host controller %zu is `%s'.\n", class_index, path);
	printf("Bus %02zu: %s\n", class_index, path);
}
static void print_found_dev(usb_address_t addr, const char *path)
{
	// printf(NAME ":     device with address %d is `%s'.\n", addr, path);
	printf("  Device %02d: %s\n", addr, path);
}

static void print_hc_devices(devman_handle_t hc_handle)
{
	int rc;
	usb_hc_connection_t conn;

	usb_hc_connection_initialize(&conn, hc_handle);
	rc = usb_hc_connection_open(&conn);
	if (rc != EOK) {
		printf(NAME ": failed to connect to HC: %s.\n",
		    str_error(rc));
		return;
	}
	usb_address_t addr;
	for (addr = 1; addr < MAX_USB_ADDRESS; addr++) {
		devman_handle_t dev_handle;
		rc = usb_hc_get_handle_by_address(&conn, addr, &dev_handle);
		if (rc != EOK) {
			continue;
		}
		char path[MAX_PATH_LENGTH];
		rc = devman_get_device_path(dev_handle, path, MAX_PATH_LENGTH);
		if (rc != EOK) {
			continue;
		}
		print_found_dev(addr, path);
	}
	usb_hc_connection_close(&conn);
}

int main(int argc, char *argv[])
{
	size_t class_index = 0;
	size_t failed_attempts = 0;

	while (failed_attempts < MAX_FAILED_ATTEMPTS) {
		class_index++;
		devman_handle_t hc_handle = 0;
		int rc = usb_ddf_get_hc_handle_by_class(class_index, &hc_handle);
		if (rc != EOK) {
			failed_attempts++;
			continue;
		}
		char path[MAX_PATH_LENGTH];
		rc = devman_get_device_path(hc_handle, path, MAX_PATH_LENGTH);
		if (rc != EOK) {
			continue;
		}
		print_found_hc(class_index, path);
		print_hc_devices(hc_handle);
	}

	return 0;
}


/** @}
 */
