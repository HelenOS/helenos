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

#include <adt/list.h>
#include <fibril_synch.h>
#include <usb/usb.h>

#include <usb/host/endpoint.h>

/** Bytes per second in FULL SPEED */
#define BANDWIDTH_TOTAL_USB11 (12000000 / 8)
/** 90% of total bandwidth is available for periodic transfers */
#define BANDWIDTH_AVAILABLE_USB11 ((BANDWIDTH_TOTAL_USB11 / 10) * 9)
/** 16 addresses per list */
#define ENDPOINT_LIST_COUNT 8

/** Endpoint management structure */
typedef struct usb_endpoint_manager {
	/** Store endpoint_t instances */
	list_t endpoint_lists[ENDPOINT_LIST_COUNT];
	/** Prevents races accessing lists */
	fibril_mutex_t guard;
	/** Size of the bandwidth pool */
	size_t free_bw;
	/** Use this function to count bw required by EP */
	size_t (*bw_count)(usb_speed_t, usb_transfer_type_t, size_t, size_t);
} usb_endpoint_manager_t;

size_t bandwidth_count_usb11(usb_speed_t speed, usb_transfer_type_t type,
    size_t size, size_t max_packet_size);

int usb_endpoint_manager_init(usb_endpoint_manager_t *instance,
    size_t available_bandwidth,
    size_t (*bw_count)(usb_speed_t, usb_transfer_type_t, size_t, size_t));

void usb_endpoint_manager_reset_eps_if_need(usb_endpoint_manager_t *instance,
    usb_target_t target, const uint8_t data[8]);

int usb_endpoint_manager_register_ep(
    usb_endpoint_manager_t *instance, endpoint_t *ep, size_t data_size);
int usb_endpoint_manager_unregister_ep(
    usb_endpoint_manager_t *instance, endpoint_t *ep);
endpoint_t * usb_endpoint_manager_find_ep(usb_endpoint_manager_t *instance,
    usb_address_t address, usb_endpoint_t ep, usb_direction_t direction);

int usb_endpoint_manager_add_ep(usb_endpoint_manager_t *instance,
    usb_address_t address, usb_endpoint_t endpoint, usb_direction_t direction,
    usb_transfer_type_t type, usb_speed_t speed, size_t max_packet_size,
    size_t data_size, int (*callback)(endpoint_t *, void *), void *arg);

int usb_endpoint_manager_remove_ep(usb_endpoint_manager_t *instance,
    usb_address_t address, usb_endpoint_t endpoint, usb_direction_t direction,
    void (*callback)(endpoint_t *, void *), void *arg);

void usb_endpoint_manager_remove_address(usb_endpoint_manager_t *instance,
    usb_address_t address, void (*callback)(endpoint_t *, void *), void *arg);
#endif
/**
 * @}
 */
