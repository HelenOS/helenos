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
#include <stdbool.h>
#include <getopt.h>
#include <devman.h>
#include <loc.h>
#include <usb/dev/hub.h>
#include <usb/hc.h>

#include "usbinfo.h"

#define MAX_USB_ADDRESS USB11_ADDRESS_MAX
#define MAX_PATH_LENGTH 1024

static void print_found_hc(service_id_t sid, const char *path)
{
	printf("Bus %" PRIun ": %s\n", sid, path);
}
static void print_found_dev(usb_address_t addr, const char *path)
{
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
		rc = devman_fun_get_path(dev_handle, path, MAX_PATH_LENGTH);
		if (rc != EOK) {
			continue;
		}
		print_found_dev(addr, path);
	}
	usb_hc_connection_close(&conn);
}

void list(void)
{
	category_id_t usbhc_cat;
	service_id_t *svcs;
	size_t count;
	size_t i;
	int rc;

	rc = loc_category_get_id(USB_HC_CATEGORY, &usbhc_cat, 0);
	if (rc != EOK) {
		printf(NAME ": Error resolving category '%s'",
		    USB_HC_CATEGORY);
		return;
	}

	rc = loc_category_get_svcs(usbhc_cat, &svcs, &count);
	if (rc != EOK) {
		printf(NAME ": Error getting list of host controllers.\n");
		return;
	}

	for (i = 0; i < count; i++) {
		devman_handle_t hc_handle = 0;
		int rc = usb_ddf_get_hc_handle_by_sid(svcs[i], &hc_handle);
		if (rc != EOK) {
			printf(NAME ": Error resolving handle of HC with SID %"
			    PRIun ", skipping.\n", svcs[i]);
			continue;
		}
		char path[MAX_PATH_LENGTH];
		rc = devman_fun_get_path(hc_handle, path, MAX_PATH_LENGTH);
		if (rc != EOK) {
			printf(NAME ": Error resolving path of HC with SID %"
			    PRIun ", skipping.\n", svcs[i]);
			continue;
		}
		print_found_hc(svcs[i], path);
		print_hc_devices(hc_handle);
	}

	free(svcs);
}


/** @}
 */
