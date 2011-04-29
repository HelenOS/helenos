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
#include <usb/usbdevice.h>
#include <usb/pipes.h>
#include "usbinfo.h"

static bool resolve_hc_handle_and_dev_addr(const char *devpath,
    devman_handle_t *out_hc_handle, usb_address_t *out_device_address)
{
	int rc;

	/* Hack for QEMU to save-up on typing ;-). */
	if (str_cmp(devpath, "qemu") == 0) {
		devpath = "/hw/pci0/00:01.2/uhci-rh/usb00_a1";
	}

	/* Hack for virtual keyboard. */
	if (str_cmp(devpath, "virt") == 0) {
		devpath = "/virt/usbhc/usb00_a1/usb00_a2";
	}

	char *path = str_dup(devpath);
	if (path == NULL) {
		return ENOMEM;
	}

	devman_handle_t hc = 0;
	bool hc_found = false;
	usb_address_t addr = 0;
	bool addr_found = false;

	/* Remove suffixes and hope that we will encounter device node. */
	while (str_length(path) > 0) {
		/* Get device handle first. */
		devman_handle_t dev_handle;
		rc = devman_device_get_handle(path, &dev_handle, 0);
		if (rc != EOK) {
			free(path);
			return false;
		}

		/* Try to find its host controller. */
		if (!hc_found) {
			rc = usb_hc_find(dev_handle, &hc);
			if (rc == EOK) {
				hc_found = true;
			}
		}
		/* Try to get its address. */
		if (!addr_found) {
			addr = usb_device_get_assigned_address(dev_handle);
			if (addr >= 0) {
				addr_found = true;
			}
		}

		/* Speed-up. */
		if (hc_found && addr_found) {
			break;
		}

		/* Remove the last suffix. */
		char *slash_pos = str_rchr(path, '/');
		if (slash_pos != NULL) {
			*slash_pos = 0;
		}
	}

	free(path);

	if (hc_found && addr_found) {
		if (out_hc_handle != NULL) {
			*out_hc_handle = hc;
		}
		if (out_device_address != NULL) {
			*out_device_address = addr;
		}
		return true;
	} else {
		return false;
	}
}

static void print_usage(char *app_name)
{
#define _INDENT "      "
#define _OPTION(opt, description) \
	printf(_INDENT opt "\n" _INDENT _INDENT description "\n")

	printf(NAME ": query USB devices for descriptors\n\n");
	printf("Usage: %s [options] device [device [device [ ... ]]]\n",
	    app_name);
	printf(_INDENT "The device is a devman path to the device.\n");

	_OPTION("-h --help", "Print this help and exit.");
	_OPTION("-i --identification", "Brief device identification.");
	_OPTION("-m --match-ids", "Print match ids generated for the device.");
	_OPTION("-t --descriptor-tree", "Print descriptor tree.");
	_OPTION("-T --descriptor-tree-full", "Print detailed descriptor tree");
	_OPTION("-s --strings", "Try to print all string descriptors.");
	_OPTION("-S --status", "Get status of the device.");

	printf("\n");
	printf("If no option is specified, `-i' is considered default.\n");
	printf("\n");

#undef _OPTION
#undef _INDENT
}

static struct option long_options[] = {
	{"help", no_argument, NULL, 'h'},
	{"identification", no_argument, NULL, 'i'},
	{"match-ids", no_argument, NULL, 'm'},
	{"descriptor-tree", no_argument, NULL, 't'},
	{"descriptor-tree-full", no_argument, NULL, 'T'},
	{"strings", no_argument, NULL, 's'},
	{"status", no_argument, NULL, 'S'},
	{0, 0, NULL, 0}
};
static const char *short_options = "himtTsS";

static usbinfo_action_t actions[] = {
	{
		.opt = 'i',
		.action = dump_short_device_identification,
		.active = false
	},
	{
		.opt = 'm',
		.action = dump_device_match_ids,
		.active = false
	},
	{
		.opt = 't',
		.action = dump_descriptor_tree_brief,
		.active = false
	},
	{
		.opt = 'T',
		.action = dump_descriptor_tree_full,
		.active = false
	},
	{
		.opt = 's',
		.action = dump_strings,
		.active = false
	},
	{
		.opt = 'S',
		.action = dump_status,
		.active = false
	},
	{
		.opt = 0
	}
};

int main(int argc, char *argv[])
{
	if (argc <= 1) {
		print_usage(argv[0]);
		return -1;
	}

	/*
	 * Process command-line options. They determine what shall be
	 * done with the device.
	 */
	int opt;
	do {
		opt = getopt_long(argc, argv,
		    short_options, long_options, NULL);
		switch (opt) {
			case -1:
				break;
			case '?':
				print_usage(argv[0]);
				return 1;
			case 'h':
				print_usage(argv[0]);
				return 0;
			default: {
				int idx = 0;
				while (actions[idx].opt != 0) {
					if (actions[idx].opt == opt) {
						actions[idx].active = true;
						break;
					}
					idx++;
				}
				break;
			}
		}
	} while (opt > 0);

	/* Set the default action. */
	int idx = 0;
	bool something_active = false;
	while (actions[idx].opt != 0) {
		if (actions[idx].active) {
			something_active = true;
			break;
		}
		idx++;
	}
	if (!something_active) {
		actions[0].active = true;
	}

	/*
	 * Go through all devices given on the command line and run the
	 * specified actions.
	 */
	int i;
	for (i = optind; i < argc; i++) {
		char *devpath = argv[i];

		/* The initialization is here only to make compiler happy. */
		devman_handle_t hc_handle = 0;
		usb_address_t dev_addr = 0;
		bool found = resolve_hc_handle_and_dev_addr(devpath,
		    &hc_handle, &dev_addr);
		if (!found) {
			fprintf(stderr, NAME ": device `%s' not found "
			    "or not of USB kind, skipping.\n",
			    devpath);
			continue;
		}

		usbinfo_device_t *dev = prepare_device(hc_handle, dev_addr);
		if (dev == NULL) {
			continue;
		}

		/* Run actions the user specified. */
		printf("%s\n", devpath);

		int action = 0;
		while (actions[action].opt != 0) {
			if (actions[action].active) {
				actions[action].action(dev);
			}
			action++;
		}

		/* Destroy the control pipe (close the session etc.). */
		destroy_device(dev);
	}

	return 0;
}


/** @}
 */
