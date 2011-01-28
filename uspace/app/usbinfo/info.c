/*
 * Copyright (c) 2011 Vojtech Horky
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
 * Dumping of generic device properties.
 */
#include <stdio.h>
#include <str_error.h>
#include <errno.h>
#include <usb/usbdrv.h>
#include "usbinfo.h"

int dump_device(int hc_phone, usb_address_t address)
{
	/*
	 * Dump information about possible match ids.
	 */
	match_id_list_t match_id_list;
	init_match_ids(&match_id_list);
	int rc = usb_drv_create_device_match_ids(hc_phone, &match_id_list, address);
	if (rc != EOK) {
		fprintf(stderr,
		    NAME ": failed to fetch match ids of the device: %s.\n",
		    str_error(rc));
		return rc;
	}
	dump_match_ids(&match_id_list);

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
	dump_usb_descriptor((uint8_t *)&device_descriptor, sizeof(device_descriptor));

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
	//dump_standard_configuration_descriptor(config_index, &config_descriptor);

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

	dump_descriptor_tree(full_config_descriptor,
	    config_descriptor.total_length);

	return EOK;
}

/** @}
 */
