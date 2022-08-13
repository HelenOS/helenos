/*
 * SPDX-FileCopyrightText: 2010-2011 Vojtech Horky
 *
 * SPDX-License-Identifier: BSD-3-Clause
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
#include <usb_iface.h>
#include <str.h>

#include "usbinfo.h"

#define MAX_PATH_LENGTH 1024

static void print_usb_device(devman_handle_t handle)
{
	char path[MAX_PATH_LENGTH];
	errno_t rc = devman_fun_get_path(handle, path, MAX_PATH_LENGTH);
	if (rc != EOK) {
		printf(NAME "Failed to get path for device %" PRIun "\n", handle);
		return;
	}
	printf("\tDevice %" PRIun ": %s\n", handle, path);
}

static void print_usb_bus(service_id_t svc)
{
	devman_handle_t hc_handle = 0;
	errno_t rc = devman_fun_sid_to_handle(svc, &hc_handle);
	if (rc != EOK) {
		printf(NAME ": Error resolving handle of HC with SID %"
		    PRIun ", skipping.\n", svc);
		return;
	}

	char path[MAX_PATH_LENGTH];
	rc = devman_fun_get_path(hc_handle, path, sizeof(path));
	if (rc != EOK) {
		printf(NAME ": Error resolving path of HC with SID %"
		    PRIun ", skipping.\n", svc);
		return;
	}
	printf("Bus %" PRIun ": %s\n", svc, path);

	/*
	 * Construct device's path.
	 * That's "hc function path" - ( '/' + "hc function name" )
	 */
	// TODO replace this with something sane

	/* Get function name */
	char name[10];
	rc = devman_fun_get_name(hc_handle, name, sizeof(name));
	if (rc != EOK) {
		printf(NAME ": Error resolving name of HC with SID %"
		    PRIun ", skipping.\n", svc);
		return;
	}

	/* Get handle of parent device */
	devman_handle_t fh;
	path[str_size(path) - str_size(name) - 1] = '\0';
	rc = devman_fun_get_handle(path, &fh, IPC_FLAG_BLOCKING);
	if (rc != EOK) {
		printf(NAME ": Error resolving parent handle of HC with"
		    " SID %" PRIun ", skipping.\n", svc);
		return;
	}

	/* Get child handle */
	devman_handle_t dh;
	rc = devman_fun_get_child(fh, &dh);
	if (rc != EOK) {
		printf(NAME ": Error resolving parent handle of HC with"
		    " SID %" PRIun ", skipping.\n", svc);
		return;
	}

	devman_handle_t *fhs = 0;
	size_t count;
	rc = devman_dev_get_functions(dh, &fhs, &count);
	if (rc != EOK) {
		printf(NAME ": Error siblings of HC with"
		    " SID %" PRIun ", skipping.\n", svc);
		return;
	}

	for (size_t i = 0; i < count; ++i) {
		if (fhs[i] != hc_handle)
			print_usb_device(fhs[i]);
	}
	free(fhs);
}

void list(void)
{
	category_id_t usbhc_cat;
	service_id_t *svcs;
	size_t count;
	errno_t rc;

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

	for (unsigned int i = 0; i < count; ++i) {
		print_usb_bus(svcs[i]);
	}

	free(svcs);
}

/** @}
 */
