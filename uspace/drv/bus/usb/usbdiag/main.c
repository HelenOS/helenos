/*
 * Copyright (c) 2017 Petr Manek
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

/** @addtogroup drvusbdiag
 * @{
 */
/**
 * @file
 * Main routines of USB diagnostic device driver.
 */
#include <errno.h>
#include <usb/debug.h>
#include <usb/classes/classes.h>
#include <usb/dev/driver.h>
#include <usbdiag_iface.h>
#include <str_error.h>

#include "device.h"

#define NAME "usbdiag"

static const usb_endpoint_description_t *diag_endpoints[];

static errno_t device_add(usb_device_t *dev)
{
	errno_t rc;
	usb_log_info("Adding device '%s'", usb_device_get_name(dev));

	usbdiag_dev_t *diag_dev;
	if ((rc = usbdiag_dev_create(dev, &diag_dev, diag_endpoints))) {
		usb_log_error("Failed create device: %s.", str_error(rc));
		goto err;
	}

	if ((rc = ddf_fun_bind(diag_dev->fun))) {
		usb_log_error("Failed to bind DDF function: %s.", str_error(rc));
		goto err_create;
	}

	if ((rc = ddf_fun_add_to_category(diag_dev->fun, USBDIAG_CATEGORY))) {
		usb_log_error("Failed add DDF to category '"
		    USBDIAG_CATEGORY "': %s.\n", str_error(rc));
		goto err_bind;
	}

	return EOK;

err_bind:
	ddf_fun_unbind(diag_dev->fun);
err_create:
	usbdiag_dev_destroy(diag_dev);
err:
	return rc;
}

static errno_t device_cleanup(usbdiag_dev_t *diag_dev)
{
	/* TODO: Join some fibrils? */

	/* Free memory. */
	usbdiag_dev_destroy(diag_dev);
	return EOK;
}

static errno_t device_remove(usb_device_t *dev)
{
	errno_t rc;
	usb_log_info("Removing device '%s'", usb_device_get_name(dev));

	usbdiag_dev_t *diag_dev = usb_device_to_usbdiag_dev(dev);

	/* TODO: Make sure nothing is going on with the device. */

	if ((rc = ddf_fun_unbind(diag_dev->fun))) {
		usb_log_error("Failed to unbind DDF function: %s", str_error(rc));
		goto err;
	}

	usb_log_info("Device '%s' removed.", usb_device_get_name(dev));
	return device_cleanup(diag_dev);

err:
	return rc;
}

static errno_t device_gone(usb_device_t *dev)
{
	errno_t rc;
	usb_log_info("Device '%s' gone.", usb_device_get_name(dev));

	usbdiag_dev_t *diag_dev = usb_device_to_usbdiag_dev(dev);

	/* TODO: Make sure nothing is going on with the device. */

	if ((rc = ddf_fun_unbind(diag_dev->fun))) {
		usb_log_error("Failed to unbind DDF function: %s", str_error(rc));
		goto err;
	}

	return device_cleanup(diag_dev);

err:
	return rc;
}

static errno_t function_online(ddf_fun_t *fun)
{
	return ddf_fun_online(fun);
}

static errno_t function_offline(ddf_fun_t *fun)
{
	return ddf_fun_offline(fun);
}

static const usb_endpoint_description_t burst_intr_in_ep = {
	.transfer_type = USB_TRANSFER_INTERRUPT,
	.direction = USB_DIRECTION_IN,
	.interface_class = USB_CLASS_DIAGNOSTIC,
	.interface_subclass = 0x00,
	.interface_protocol = 0x01,
	.flags = 0
};
static const usb_endpoint_description_t burst_intr_out_ep = {
	.transfer_type = USB_TRANSFER_INTERRUPT,
	.direction = USB_DIRECTION_OUT,
	.interface_class = USB_CLASS_DIAGNOSTIC,
	.interface_subclass = 0x00,
	.interface_protocol = 0x01,
	.flags = 0
};
static const usb_endpoint_description_t burst_bulk_in_ep = {
	.transfer_type = USB_TRANSFER_BULK,
	.direction = USB_DIRECTION_IN,
	.interface_class = USB_CLASS_DIAGNOSTIC,
	.interface_subclass = 0x00,
	.interface_protocol = 0x01,
	.flags = 0
};
static const usb_endpoint_description_t burst_bulk_out_ep = {
	.transfer_type = USB_TRANSFER_BULK,
	.direction = USB_DIRECTION_OUT,
	.interface_class = USB_CLASS_DIAGNOSTIC,
	.interface_subclass = 0x00,
	.interface_protocol = 0x01,
	.flags = 0
};
static const usb_endpoint_description_t burst_isoch_in_ep = {
	.transfer_type = USB_TRANSFER_ISOCHRONOUS,
	.direction = USB_DIRECTION_IN,
	.interface_class = USB_CLASS_DIAGNOSTIC,
	.interface_subclass = 0x00,
	.interface_protocol = 0x01,
	.flags = 0
};
static const usb_endpoint_description_t burst_isoch_out_ep = {
	.transfer_type = USB_TRANSFER_ISOCHRONOUS,
	.direction = USB_DIRECTION_OUT,
	.interface_class = USB_CLASS_DIAGNOSTIC,
	.interface_subclass = 0x00,
	.interface_protocol = 0x01,
	.flags = 0
};
static const usb_endpoint_description_t data_intr_in_ep = {
	.transfer_type = USB_TRANSFER_INTERRUPT,
	.direction = USB_DIRECTION_IN,
	.interface_class = USB_CLASS_DIAGNOSTIC,
	.interface_subclass = 0x00,
	.interface_protocol = 0x01,
	.flags = 0
};
static const usb_endpoint_description_t data_intr_out_ep = {
	.transfer_type = USB_TRANSFER_INTERRUPT,
	.direction = USB_DIRECTION_OUT,
	.interface_class = USB_CLASS_DIAGNOSTIC,
	.interface_subclass = 0x00,
	.interface_protocol = 0x01,
	.flags = 0
};
static const usb_endpoint_description_t data_bulk_in_ep = {
	.transfer_type = USB_TRANSFER_BULK,
	.direction = USB_DIRECTION_IN,
	.interface_class = USB_CLASS_DIAGNOSTIC,
	.interface_subclass = 0x00,
	.interface_protocol = 0x01,
	.flags = 0
};
static const usb_endpoint_description_t data_bulk_out_ep = {
	.transfer_type = USB_TRANSFER_BULK,
	.direction = USB_DIRECTION_OUT,
	.interface_class = USB_CLASS_DIAGNOSTIC,
	.interface_subclass = 0x00,
	.interface_protocol = 0x01,
	.flags = 0
};
static const usb_endpoint_description_t data_isoch_in_ep = {
	.transfer_type = USB_TRANSFER_ISOCHRONOUS,
	.direction = USB_DIRECTION_IN,
	.interface_class = USB_CLASS_DIAGNOSTIC,
	.interface_subclass = 0x00,
	.interface_protocol = 0x01,
	.flags = 0
};
static const usb_endpoint_description_t data_isoch_out_ep = {
	.transfer_type = USB_TRANSFER_ISOCHRONOUS,
	.direction = USB_DIRECTION_OUT,
	.interface_class = USB_CLASS_DIAGNOSTIC,
	.interface_subclass = 0x00,
	.interface_protocol = 0x01,
	.flags = 0
};

static const usb_endpoint_description_t *diag_endpoints[] = {
	[USBDIAG_EP_BURST_INTR_IN] = &burst_intr_in_ep,
	[USBDIAG_EP_BURST_INTR_OUT] = &burst_intr_out_ep,
	[USBDIAG_EP_BURST_BULK_IN] = &burst_bulk_in_ep,
	[USBDIAG_EP_BURST_BULK_OUT] = &burst_bulk_out_ep,
	[USBDIAG_EP_BURST_ISOCH_IN] = &burst_isoch_in_ep,
	[USBDIAG_EP_BURST_ISOCH_OUT] = &burst_isoch_out_ep,
	[USBDIAG_EP_DATA_INTR_IN] = &data_intr_in_ep,
	[USBDIAG_EP_DATA_INTR_OUT] = &data_intr_out_ep,
	[USBDIAG_EP_DATA_BULK_IN] = &data_bulk_in_ep,
	[USBDIAG_EP_DATA_BULK_OUT] = &data_bulk_out_ep,
	[USBDIAG_EP_DATA_ISOCH_IN] = &data_isoch_in_ep,
	[USBDIAG_EP_DATA_ISOCH_OUT] = &data_isoch_out_ep,
	NULL
};

/** USB diagnostic driver ops. */
static const usb_driver_ops_t diag_driver_ops = {
	.device_add = device_add,
	.device_remove = device_remove,
	.device_gone = device_gone,
	.function_online = function_online,
	.function_offline = function_offline
};

/** USB diagnostic driver. */
static const usb_driver_t diag_driver = {
	.name = NAME,
	.ops = &diag_driver_ops,
	.endpoints = &diag_endpoints[1] /* EPs are indexed from 1. */
};

int main(int argc, char *argv[])
{
	printf(NAME ": USB diagnostic device driver.\n");

	log_init(NAME);

	return usb_driver_main(&diag_driver);
}

/**
 * @}
 */
