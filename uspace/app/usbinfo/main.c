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

/** @addtogroup usbinfo
 * @{
 */
/**
 * @file
 * USB querying.
 */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <str_error.h>
#include <bool.h>
#include <getopt.h>
#include <devman.h>
#include <devmap.h>
#include <usb/usbdrv.h>
#include "usbinfo.h"

enum {
	ACTION_HELP = 256,
	ACTION_DEVICE_ADDRESS,
	ACTION_HOST_CONTROLLER,
	ACTION_DEVICE,
};

static struct option long_options[] = {
	{"help", no_argument, NULL, ACTION_HELP},
	{"address", required_argument, NULL, ACTION_DEVICE_ADDRESS},
	{"host-controller", required_argument, NULL, ACTION_HOST_CONTROLLER},
	{"device", required_argument, NULL, ACTION_DEVICE},
	{0, 0, NULL, 0}
};
static const char *short_options = "ha:t:d:";

static void print_usage(char *app_name)
{
#define INDENT "      "
	printf(NAME ": query USB devices for descriptors\n\n");
	printf("Usage: %s [options]\n", app_name);
	printf(" -h --help\n" INDENT \
	    "Display this help.\n");
	printf(" -tID --host-controller ID\n" INDENT \
	    "Set host controller (ID can be path or class number)\n");
	printf(" -aADDR --address ADDR\n" INDENT \
	    "Set device address\n");
	printf("\n");
#undef INDENT
}

static int get_host_controller_handle(const char *path,
    devman_handle_t *hc_handle)
{
	int rc;

	if (str_cmp(path, "uhci") == 0) {
		path = "/hw/pci0/00:01.2";
	}

	devman_handle_t handle;
	rc = devman_device_get_handle(path, &handle, 0);
	if (rc != EOK) {
		fprintf(stderr,
		    NAME ": failed getting handle of `devman::/%s'.\n",
		    path);
		return rc;
	}
	*hc_handle = handle;

	return EOK;
}

static int get_device_address(const char *str_address, usb_address_t *address)
{
	usb_address_t addr = (usb_address_t) strtol(str_address, NULL, 0);
	if ((addr < 0) || (addr >= USB11_ADDRESS_MAX)) {
		fprintf(stderr, NAME ": USB address out of range.\n");
		return ERANGE;
	}

	*address = addr;
	return EOK;
}


int main(int argc, char *argv[])
{
	devman_handle_t hc_handle = (devman_handle_t) -1;
	usb_address_t device_address = (usb_address_t) -1;

	if (argc <= 1) {
		print_usage(argv[0]);
		return -1;
	}

	int i;
	do {
		i = getopt_long(argc, argv, short_options, long_options, NULL);
		switch (i) {
			case -1:
				break;

			case '?':
				print_usage(argv[0]);
				return -1;

			case 'h':
			case ACTION_HELP:
				print_usage(argv[0]);
				return 0;

			case 'a':
			case ACTION_DEVICE_ADDRESS: {
				int rc = get_device_address(optarg,
				    &device_address);
				if (rc != EOK) {
					return rc;
				}
				break;
			}

			case 't':
			case ACTION_HOST_CONTROLLER: {
				int rc = get_host_controller_handle(optarg,
				   &hc_handle);
				if (rc != EOK) {
					return rc;
				}
				break;
			}

			case 'd':
			case ACTION_DEVICE:
				break;

			default:
				break;
		}

	} while (i != -1);

	if ((hc_handle == (devman_handle_t) -1)
	    || (device_address == (usb_address_t) -1)) {
		fprintf(stderr, NAME ": no target specified.\n");
		return EINVAL;
	}

	dump_device(hc_handle, device_address);

	return 0;
}


/** @}
 */
