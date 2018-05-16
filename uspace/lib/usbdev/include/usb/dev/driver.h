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
