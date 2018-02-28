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
#include <usb_iface.h>
#include <str.h>

#include "usbinfo.h"

#define MAX_PATH_LENGTH 1024

static void print_usb_device(devman_handle_t handle)
{
	char path[MAX_PATH_LENGTH];
	errno_t rc = devman_fun_get_path(handle, path, MAX_PATH_LENGTH);
	if (rc != EOK) {
		printf(NAME "Failed to get path for device %"PRIun"\n", handle);
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

	/* Construct device's path.
	 * That's "hc function path" - ( '/' + "hc function name" ) */
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
