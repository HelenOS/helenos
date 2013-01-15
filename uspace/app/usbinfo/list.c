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
#include <usb_iface.h>

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

static void print_usb_devices(devman_handle_t bus_handle,
    devman_handle_t *fhs, size_t count)
{
	for (size_t i = 0; i < count; ++i) {
		if (fhs[i] == bus_handle)
			continue;
		char path[MAX_PATH_LENGTH];
		int rc = devman_fun_get_path(fhs[i], path, MAX_PATH_LENGTH);
		if (rc != EOK) {
			printf(NAME "Failed to get path for device %" PRIun
			    "\n", fhs[i]);
			continue;
		}
		// TODO remove this. Device name contains USB address
		// and addresses as external id are going away
		usb_dev_session_t *sess = usb_dev_connect(fhs[i]);
		if (!sess) {
			printf(NAME "Failed to connect to device %" PRIun
			    "\n", fhs[i]);
			continue;
		}
		async_exch_t *exch = async_exchange_begin(sess);
		if (!exch) {
			printf("Failed to create exchange to dev %" PRIun
			    "\n", fhs[i]);
			usb_dev_session_close(sess);
			continue;
		}
		usb_address_t address;
		rc = usb_get_my_address(exch, &address);
		async_exchange_end(exch);
		usb_dev_session_close(sess);
		if (rc != EOK) {
			printf("Failed to get address for device %" PRIun
			    "\n", fhs[i]);
			continue;
		}
		print_found_dev(address, path);

	}
}

void list(void)
{
	category_id_t usbhc_cat;
	service_id_t *svcs;
	size_t count;
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

	for (unsigned i = 0; i < count; ++i) {
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
		(void)print_hc_devices;

		// TODO replace this with something sane
		char name[10];
		rc = devman_fun_get_name(hc_handle, name, 10);
		if (rc != EOK) {
			printf(NAME ": Error resolving name of HC with SID %"
			    PRIun ", skipping.\n", svcs[i]);
			continue;
		}

		devman_handle_t fh;
		path[str_size(path) - str_size(name) - 1] = '\0';
		rc = devman_fun_get_handle(path, &fh, IPC_FLAG_BLOCKING);
		if (rc != EOK) {
			printf(NAME ": Error resolving parent handle of HC with"
			    " SID %" PRIun ", skipping.\n", svcs[i]);
			continue;
		}
		devman_handle_t dh;
		rc = devman_fun_get_child(fh, &dh);
		if (rc != EOK) {
			printf(NAME ": Error resolving parent handle of HC with"
			    " SID %" PRIun ", skipping.\n", svcs[i]);
			continue;
		}
		devman_handle_t *fhs = 0;
		size_t count;
		rc = devman_dev_get_functions(dh, &fhs, &count);
		if (rc != EOK) {
			printf(NAME ": Error siblings of HC with"
			    " SID %" PRIun ", skipping.\n", svcs[i]);
			continue;
		}
		print_usb_devices(hc_handle, fhs, count);
		free(fhs);
	}

	free(svcs);
}


/** @}
 */
