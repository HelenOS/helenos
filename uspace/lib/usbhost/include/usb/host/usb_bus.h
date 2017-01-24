/*
 * Copyright (c) 2011 Jan Vesely
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
/** @addtogroup libusbhost
 * @{
 */
/** @file
 * Device keeper structure and functions.
 *
 * Typical USB host controller needs to keep track of various settings for
 * each device that is connected to it.
 * State of toggle bit, device speed etc. etc.
 * This structure shall simplify the management.
 */
#ifndef LIBUSBHOST_HOST_USB_ENDPOINT_MANAGER_H
#define LIBUSBHOST_HOST_USB_ENDPOINT_MANAGER_H

#include <usb/host/endpoint.h>
#include <usb/usb.h>

#include <adt/list.h>
#include <fibril_synch.h>
#include <stdbool.h>


/** Bytes per second in FULL SPEED */
#define BANDWIDTH_TOTAL_USB11 (12000000 / 8)
/** 90% of total bandwidth is available for periodic transfers */
#define BANDWIDTH_AVAILABLE_USB11 ((BANDWIDTH_TOTAL_USB11 / 10) * 9)

//TODO: Implement
#define BANDWIDTH_AVAILABLE_USB20  1

typedef size_t (*bw_count_func_t)(usb_speed_t, usb_transfer_type_t, size_t, size_t);
typedef void (*ep_remove_callback_t)(endpoint_t *, void *);
typedef errno_t (*ep_add_callback_t)(endpoint_t *, void *);

/** Endpoint management structure */
typedef struct usb_bus {
	struct {
		usb_speed_t speed;      /**< Device speed */
		bool occupied;          /**< The address is in use. */
		list_t endpoint_list;   /**< Store endpoint_t instances */
	} devices[USB_ADDRESS_COUNT];
	/** Prevents races accessing lists */
	fibril_mutex_t guard;
	/** Size of the bandwidth pool */
	size_t free_bw;
	/** Use this function to count bw required by EP */
	bw_count_func_t bw_count;
	/** Maximum speed allowed. */
	usb_speed_t max_speed;
	/** The last reserved address */
	usb_address_t last_address;
} usb_bus_t;


size_t bandwidth_count_usb11(usb_speed_t speed, usb_transfer_type_t type,
    size_t size, size_t max_packet_size);
size_t bandwidth_count_usb20(usb_speed_t speed, usb_transfer_type_t type,
    size_t size, size_t max_packet_size);

errno_t usb_bus_init(usb_bus_t *instance,
    size_t available_bandwidth, bw_count_func_t bw_count, usb_speed_t max_speed);

errno_t usb_bus_register_ep(usb_bus_t *instance, endpoint_t *ep, size_t data_size);

errno_t usb_bus_unregister_ep(usb_bus_t *instance, endpoint_t *ep);

endpoint_t *usb_bus_find_ep(usb_bus_t *instance,
    usb_address_t address, usb_endpoint_t ep, usb_direction_t direction);

errno_t usb_bus_add_ep(usb_bus_t *instance,
    usb_address_t address, usb_endpoint_t endpoint, usb_direction_t direction,
    usb_transfer_type_t type, size_t max_packet_size, unsigned packets,
    size_t data_size, ep_add_callback_t callback, void *arg,
    usb_address_t tt_address, unsigned tt_port);

errno_t usb_bus_remove_ep(usb_bus_t *instance,
    usb_address_t address, usb_endpoint_t endpoint, usb_direction_t direction,
    ep_remove_callback_t callback, void *arg);

errno_t usb_bus_reset_toggle(usb_bus_t *instance, usb_target_t target, bool all);

errno_t usb_bus_remove_address(usb_bus_t *instance,
    usb_address_t address, ep_remove_callback_t callback, void *arg);

errno_t usb_bus_request_address(usb_bus_t *instance,
    usb_address_t *address, bool strict, usb_speed_t speed);

errno_t usb_bus_get_speed(usb_bus_t *instance,
    usb_address_t address, usb_speed_t *speed);
#endif
/**
 * @}
 */
