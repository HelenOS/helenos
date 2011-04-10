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

/** @addtogroup drvusbmast
 * @{
 */
/**
 * @file
 * Main routines of USB mass storage driver.
 */
#include <usb/devdrv.h>
#include <usb/debug.h>
#include <usb/classes/classes.h>
#include <usb/classes/massstor.h>
#include <errno.h>
#include <str_error.h>
#include "cmds.h"
#include "scsi.h"

#define NAME "usbmast"

#define BULK_IN_EP 0
#define BULK_OUT_EP 1

#define GET_BULK_IN(dev) ((dev)->pipes[BULK_IN_EP].pipe)
#define GET_BULK_OUT(dev) ((dev)->pipes[BULK_OUT_EP].pipe)

static usb_endpoint_description_t bulk_in_ep = {
	.transfer_type = USB_TRANSFER_BULK,
	.direction = USB_DIRECTION_IN,
	.interface_class = USB_CLASS_MASS_STORAGE,
	.interface_subclass = USB_MASSSTOR_SUBCLASS_SCSI,
	.interface_protocol = USB_MASSSTOR_PROTOCOL_BBB,
	.flags = 0
};
static usb_endpoint_description_t bulk_out_ep = {
	.transfer_type = USB_TRANSFER_BULK,
	.direction = USB_DIRECTION_OUT,
	.interface_class = USB_CLASS_MASS_STORAGE,
	.interface_subclass = USB_MASSSTOR_SUBCLASS_SCSI,
	.interface_protocol = USB_MASSSTOR_PROTOCOL_BBB,
	.flags = 0
};

usb_endpoint_description_t *mast_endpoints[] = {
	&bulk_in_ep,
	&bulk_out_ep,
	NULL
};

#define INQUIRY_RESPONSE_LENGTH 35

static void try_inquiry(usb_device_t *dev)
{
	usb_massstor_cbw_t cbw;
	scsi_cmd_inquiry_t inquiry = {
		.op_code = 0x12,
		.lun_evpd = 0,
		.page_code = 0,
		.alloc_length = INQUIRY_RESPONSE_LENGTH,
		.ctrl = 0
	};
	size_t response_len;
	uint8_t response[INQUIRY_RESPONSE_LENGTH];
	usb_massstor_csw_t csw;
	size_t csw_len;

	usb_massstor_cbw_prepare(&cbw, 0xdeadbeef, INQUIRY_RESPONSE_LENGTH,
	    USB_DIRECTION_IN, 0, sizeof(inquiry), (uint8_t *) &inquiry);

	int rc;
	rc = usb_pipe_write(GET_BULK_OUT(dev), &cbw, sizeof(cbw));
	usb_log_debug("Wrote CBW: %s.\n", str_error(rc));
	if (rc != EOK) {
		return;
	}

	rc = usb_pipe_read(GET_BULK_IN(dev), response, INQUIRY_RESPONSE_LENGTH,
	    &response_len);
	usb_log_debug("Read response (%zuB): '%s' (%s).\n", response_len,
	    usb_debug_str_buffer(response, response_len, 0),
	    str_error(rc));
	if (rc != EOK) {
		return;
	}

	rc = usb_pipe_read(GET_BULK_IN(dev), &csw, sizeof(csw), &csw_len);
	usb_log_debug("Read CSW (%zuB): '%s' (%s).\n", csw_len,
	    usb_debug_str_buffer((uint8_t *) &csw, csw_len, 0),
	    str_error(rc));

}

/** Callback when new device is attached and recognized as a mass storage.
 *
 * @param dev Representation of a the USB device.
 * @return Error code.
 */
static int usbmast_add_device(usb_device_t *dev)
{
	int rc;
	const char *fun_name = "ctl";

	ddf_fun_t *ctl_fun = ddf_fun_create(dev->ddf_dev, fun_exposed,
	    fun_name);
	if (ctl_fun == NULL) {
		usb_log_error("Failed to create control function.\n");
		return ENOMEM;
	}
	rc = ddf_fun_bind(ctl_fun);
	if (rc != EOK) {
		usb_log_error("Failed to bind control function: %s.\n",
		    str_error(rc));
		return rc;
	}

	usb_log_info("Pretending to control mass storage `%s'.\n",
	    dev->ddf_dev->name);
	usb_log_debug(" Bulk in endpoint: %d [%zuB].\n",
	    dev->pipes[BULK_IN_EP].pipe->endpoint_no,
	    (size_t) dev->pipes[BULK_IN_EP].descriptor->max_packet_size);
	usb_log_debug("Bulk out endpoint: %d [%zuB].\n",
	    dev->pipes[BULK_OUT_EP].pipe->endpoint_no,
	    (size_t) dev->pipes[BULK_OUT_EP].descriptor->max_packet_size);

	try_inquiry(dev);

	return EOK;
}

/** USB mass storage driver ops. */
static usb_driver_ops_t usbmast_driver_ops = {
	.add_device = usbmast_add_device,
};

/** USB mass storage driver. */
static usb_driver_t usbmast_driver = {
	.name = NAME,
	.ops = &usbmast_driver_ops,
	.endpoints = mast_endpoints
};

int main(int argc, char *argv[])
{
	usb_log_enable(USB_LOG_LEVEL_DEFAULT, NAME);

	return usb_driver_main(&usbmast_driver);
}

/**
 * @}
 */
