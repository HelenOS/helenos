/*
 * SPDX-FileCopyrightText: 2011 Vojtech Horky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libusbdev
 * @{
 */
/** @file
 * USB device driver framework.
 */

#ifndef LIBUSBDEV_DRIVER_H_
#define LIBUSBDEV_DRIVER_H_

#include <usb/dev/device.h>
#include <usb/dev/pipes.h>

/** USB driver ops. */
typedef struct {
	/** Callback when a new device was added to the system. */
	errno_t (*device_add)(usb_device_t *);
	/** Callback when a device is about to be removed from the system. */
	errno_t (*device_remove)(usb_device_t *);
	/** Callback when a device was removed from the system. */
	errno_t (*device_gone)(usb_device_t *);
	/** Callback asking the driver to online a specific function. */
	errno_t (*function_online)(ddf_fun_t *);
	/** Callback asking the driver to offline a specific function. */
	errno_t (*function_offline)(ddf_fun_t *);
} usb_driver_ops_t;

/** USB driver structure. */
typedef struct {
	/** Driver name.
	 * This name is copied to the generic driver name and must be exactly
	 * the same as the directory name where the driver executable resides.
	 */
	const char *name;
	/** Expected endpoints description.
	 * This description shall exclude default control endpoint (pipe zero)
	 * and must be NULL terminated.
	 * When only control endpoint is expected, you may set NULL directly
	 * without creating one item array containing NULL.
	 *
	 * When the driver expect single interrupt in endpoint,
	 * the initialization may look like this:
	 *
	 * @code
	 * static usb_endpoint_description_t poll_endpoint_description = {
	 * 	.transfer_type = USB_TRANSFER_INTERRUPT,
	 * 	.direction = USB_DIRECTION_IN,
	 * 	.interface_class = USB_CLASS_HUB,
	 * 	.interface_subclass = 0,
	 * 	.interface_protocol = 0,
	 * 	.flags = 0
	 * };
	 *
	 * static usb_endpoint_description_t *hub_endpoints[] = {
	 * 	&poll_endpoint_description,
	 * 	NULL
	 * };
	 *
	 * static usb_driver_t hub_driver = {
	 * 	.endpoints = hub_endpoints,
	 * 	...
	 * };
	 * @endcode
	 */
	const usb_endpoint_description_t **endpoints;
	/** Driver ops. */
	const usb_driver_ops_t *ops;
} usb_driver_t;

int usb_driver_main(const usb_driver_t *);

#endif
/**
 * @}
 */
