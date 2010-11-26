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

/** @addtogroup libusb usb
 * @{
 */
/** @file
 * @brief HC driver and hub driver.
 */
#ifndef LIBUSB_HCDHUBD_H_
#define LIBUSB_HCDHUBD_H_

#include <adt/list.h>
#include <driver.h>
#include <usb/usb.h>

/** Endpoint properties. */
typedef struct {
	/** Endpoint number. */
	usb_endpoint_t endpoint;
	/** Transfer type. */
	usb_transfer_type_t transfer_type;
	/** Endpoint direction. */
	usb_direction_t direction;
	/** Data toggle bit. */
	int data_toggle;
} usb_hc_endpoint_info_t;

/** Information about attached USB device. */
typedef struct {
	/** Assigned address. */
	usb_address_t address;
	/** Number of endpoints. */
	size_t endpoint_count;
	/** Endpoint properties. */
	usb_hc_endpoint_info_t *endpoints;
	/** Link to other attached devices of the same HC. */
	link_t link;
} usb_hcd_attached_device_info_t;

/** Information about attached hub. */
typedef struct {
	/** Number of ports. */
	size_t port_count;
	/** General device info. */
	usb_hcd_attached_device_info_t *device;
	/** Link to other hubs. */
	link_t link;
} usb_hcd_hub_info_t;

/** Host controller device. */
typedef struct usb_hc_device usb_hc_device_t;

/** Callback for OUT transfers. */
typedef void (*usb_hcd_transfer_callback_out_t)
    (usb_hc_device_t *, usb_transaction_outcome_t, void *);

/** Callback for IN transfers. */
typedef void (*usb_hcd_transfer_callback_in_t)
    (usb_hc_device_t *, size_t, usb_transaction_outcome_t, void *);


/** Transfer functions provided by each USB host controller driver. */
typedef struct {
	/** OUT transfer.
	 * @param hc Host controller that shall enqueue the transfer.
	 * @param dev Target device.
	 * @param ep Target endpoint.
	 * @param buffer Buffer to be sent.
	 * @param size Buffer size.
	 * @param callback Callback after transfer was processed by hardware.
	 * @param arg Callback argument.
	 */
	int (*transfer_out)(usb_hc_device_t *hc,
	    usb_hcd_attached_device_info_t *dev, usb_hc_endpoint_info_t *ep,
	    void *buffer, size_t size,
	    usb_hcd_transfer_callback_out_t callback, void *arg);

	/** SETUP transfer. */
	int (*transfer_setup)(usb_hc_device_t *,
	    usb_hcd_attached_device_info_t *, usb_hc_endpoint_info_t *,
	    void *, size_t,
	    usb_hcd_transfer_callback_out_t, void *);

	/** IN transfer. */
	int (*transfer_in)(usb_hc_device_t *,
	    usb_hcd_attached_device_info_t *, usb_hc_endpoint_info_t *,
	    void *, size_t,
	    usb_hcd_transfer_callback_in_t, void *);
} usb_hcd_transfer_ops_t;

struct usb_hc_device {
	/** Transfer operations. */
	usb_hcd_transfer_ops_t *transfer_ops;

	/** Custom HC data. */
	void *data;

	/** Generic device entry (read-only). */
	device_t *generic;

	/** List of attached devices. */
	link_t attached_devices;

	/** List of hubs operating from this HC. */
	link_t hubs;

	/** Link to other driven HCs. */
	link_t link;
};

/** Host controller driver. */
typedef struct {
	/** Driver name. */
	const char *name;
	/** Callback when device (host controller) is added. */
	int (*add_hc)(usb_hc_device_t *device);
} usb_hc_driver_t;


int usb_hcd_main(usb_hc_driver_t *);
int usb_hcd_add_root_hub(usb_hc_device_t *dev);


/*
 * Functions to be used by drivers within same task as the HC driver.
 * This will probably include only hub drivers.
 */


int usb_hc_async_interrupt_out(usb_hc_device_t *, usb_target_t,
    void *, size_t, usb_handle_t *);
int usb_hc_async_interrupt_in(usb_hc_device_t *, usb_target_t,
    void *, size_t, size_t *, usb_handle_t *);

int usb_hc_async_control_write_setup(usb_hc_device_t *, usb_target_t,
    void *, size_t, usb_handle_t *);
int usb_hc_async_control_write_data(usb_hc_device_t *, usb_target_t,
    void *, size_t, usb_handle_t *);
int usb_hc_async_control_write_status(usb_hc_device_t *, usb_target_t,
    usb_handle_t *);

int usb_hc_async_control_read_setup(usb_hc_device_t *, usb_target_t,
    void *, size_t, usb_handle_t *);
int usb_hc_async_control_read_data(usb_hc_device_t *, usb_target_t,
    void *, size_t, size_t *, usb_handle_t *);
int usb_hc_async_control_read_status(usb_hc_device_t *, usb_target_t,
    usb_handle_t *);

int usb_hc_async_wait_for(usb_handle_t);

int usb_hc_add_child_device(device_t *, const char *, const char *);

#endif
