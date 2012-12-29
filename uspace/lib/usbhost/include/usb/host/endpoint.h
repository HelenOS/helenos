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
 *
 */
#ifndef LIBUSBHOST_HOST_ENDPOINT_H
#define LIBUSBHOST_HOST_ENDPOINT_H

#include <stdbool.h>
#include <adt/list.h>
#include <fibril_synch.h>
#include <usb/usb.h>

/** Host controller side endpoint structure. */
typedef struct endpoint {
	/** Part of linked list. */
	link_t link;
	/** USB address. */
	usb_address_t address;
	/** USB endpoint number. */
	usb_endpoint_t endpoint;
	/** Communication direction. */
	usb_direction_t direction;
	/** USB transfer type. */
	usb_transfer_type_t transfer_type;
	/** Communication speed. */
	usb_speed_t speed;
	/** Maximum size of data packets. */
	size_t max_packet_size;
	/** Necessary bandwidth. */
	size_t bandwidth;
	/** Value of the toggle bit. */
	unsigned toggle:1;
	/** True if there is a batch using this scheduled for this endpoint. */
	volatile bool active;
	/** Protects resources and active status changes. */
	fibril_mutex_t guard;
	/** Signals change of active status. */
	fibril_condvar_t avail;
	/** Optional device specific data. */
	struct {
		/** Device specific data. */
		void *data;
		/** Callback to get the value of toggle bit. */
		int (*toggle_get)(void *);
		/** Callback to set the value of toggle bit. */
		void (*toggle_set)(void *, int);
	} hc_data;
} endpoint_t;

endpoint_t * endpoint_create(usb_address_t address, usb_endpoint_t endpoint,
    usb_direction_t direction, usb_transfer_type_t type, usb_speed_t speed,
    size_t max_packet_size, size_t bw);
void endpoint_destroy(endpoint_t *instance);

void endpoint_set_hc_data(endpoint_t *instance,
    void *data, int (*toggle_get)(void *), void (*toggle_set)(void *, int));
void endpoint_clear_hc_data(endpoint_t *instance);

void endpoint_use(endpoint_t *instance);
void endpoint_release(endpoint_t *instance);

int endpoint_toggle_get(endpoint_t *instance);
void endpoint_toggle_set(endpoint_t *instance, int toggle);

/** list_get_instance wrapper.
 * @param item Pointer to link member.
 * @return Pointer to endpoint_t structure.
 */
static inline endpoint_t * endpoint_get_instance(link_t *item)
{
	return item ? list_get_instance(item, endpoint_t, link) : NULL;
}
#endif
/**
 * @}
 */
