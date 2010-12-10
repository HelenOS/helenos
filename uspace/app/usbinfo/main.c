/*
 * Copyright (c) 2010 Vojtech Horky
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

/** @addtogroup usb
 * @{
 */
/**
 * @file
 * @brief USB querying.
 */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <str_error.h>
#include <bool.h>
#include <devman.h>
#include <usb/usbdrv.h>
#include "usbinfo.h"

#define DEFAULT_HOST_CONTROLLER_PATH "/virt/usbhc"

static void print_usage(char *app_name)
{
	printf(NAME ": query USB devices for descriptors\n\n");
	printf("Usage: %s /path/to/hc usb-address\n where\n", app_name);
	printf("   /path/to/hc   Devman path to USB host controller " \
	    "(use `-' for\n");
	printf("                   default HC at `%s').\n",
	    DEFAULT_HOST_CONTROLLER_PATH);
	printf("   usb-address   USB address of device to be queried\n");
	printf("\n");
}

static int connect_to_hc(const char *path)
{
	int rc;
	devman_handle_t handle;

	rc = devman_device_get_handle(path, &handle, 0);
	if (rc != EOK) {
		return rc;
	}

	int phone = devman_device_connect(handle, 0);

	return phone;
}

int main(int argc, char *argv[])
{
	if (argc != 3) {
		print_usage(argv[0]);
		return EINVAL;
	}

	char *hc_path = argv[1];
	long int address_long = strtol(argv[2], NULL, 0);

	/*
	 * Connect to given host controller driver.
	 */
	if (str_cmp(hc_path, "-") == 0) {
		hc_path = (char *) DEFAULT_HOST_CONTROLLER_PATH;
	}
	int hc_phone = connect_to_hc(hc_path);
	if (hc_phone < 0) {
		fprintf(stderr,
		    NAME ": unable to connect to HC at `%s': %s.\n",
		    hc_path, str_error(hc_phone));
		return hc_phone;
	}

	/*
	 * Verify address is okay.
	 */
	usb_address_t address = (usb_address_t) address_long;
	if ((address < 0) || (address >= USB11_ADDRESS_MAX)) {
		fprintf(stderr, NAME ": USB address out of range.\n");
		return ERANGE;
	}

	/*
	 * Now, learn information about the device.
	 */
	int rc;

	/*
	 * Get device descriptor and dump it.
	 */
	usb_standard_device_descriptor_t device_descriptor;
	usb_dprintf(NAME, 1,
	    "usb_drv_req_get_device_descriptor(%d, %d, %p)\n",
	    hc_phone, (int) address, &device_descriptor);

	rc = usb_drv_req_get_device_descriptor(hc_phone, address,
	    &device_descriptor);
	if (rc != EOK) {
		fprintf(stderr,
		    NAME ": failed to fetch standard device descriptor: %s.\n",
		    str_error(rc));
		return rc;
	}
	dump_standard_device_descriptor(&device_descriptor);

	/*
	 * Get first configuration descriptor and dump it.
	 */
	usb_standard_configuration_descriptor_t config_descriptor;
	int config_index = 0;
	usb_dprintf(NAME, 1,
	    "usb_drv_req_get_bare_configuration_descriptor(%d, %d, %d, %p)\n",
	    hc_phone, (int) address, config_index, &config_descriptor);

	rc = usb_drv_req_get_bare_configuration_descriptor(hc_phone, address,
	    config_index, &config_descriptor );
	if (rc != EOK) {
		fprintf(stderr,
		    NAME ": failed to fetch standard configuration descriptor: %s.\n",
		    str_error(rc));
		return rc;
	}
	dump_standard_configuration_descriptor(config_index,
	    &config_descriptor);

	void *full_config_descriptor = malloc(config_descriptor.total_length);
	usb_dprintf(NAME, 1,
	    "usb_drv_req_get_full_configuration_descriptor(%d, %d, %d, %p, %zu)\n",
	    hc_phone, (int) address, config_index,
	    full_config_descriptor, config_descriptor.total_length);

	rc = usb_drv_req_get_full_configuration_descriptor(hc_phone, address,
	    config_index,
	    full_config_descriptor, config_descriptor.total_length, NULL);
	if (rc != EOK) {
		fprintf(stderr,
		    NAME ": failed to fetch full configuration descriptor: %s.\n",
		    str_error(rc));
		return rc;
	}
	dump_buffer("Full configuration descriptor:",
	    full_config_descriptor, config_descriptor.total_length);

	return EOK;
}


/** @}
 */
