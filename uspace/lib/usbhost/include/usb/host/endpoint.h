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
 * Endpoint structure is tightly coupled to the bus. The bus controls the
 * life-cycle of endpoint. In order to keep endpoints lightweight, operations
 * on endpoints are part of the bus structure.
 *
 */
#ifndef LIBUSBHOST_HOST_ENDPOINT_H
#define LIBUSBHOST_HOST_ENDPOINT_H

#include <adt/list.h>
#include <atomic.h>
#include <fibril_synch.h>
#include <stdbool.h>
#include <usb/usb.h>
#include <usb/host/bus.h>

typedef struct bus bus_t;
typedef struct device device_t;
typedef struct usb_transfer_batch usb_transfer_batch_t;

/** Host controller side endpoint structure. */
typedef struct endpoint {
	/** Part of linked list. */
	link_t link;
	/** USB device */
	device_t *device;
	/** Reference count. */
	atomic_t refcnt;
	/** Enpoint number */
	usb_endpoint_t endpoint;
	/** Communication direction. */
	usb_direction_t direction;
	/** USB transfer type. */
	usb_transfer_type_t transfer_type;
	/** Maximum size of data packets. */
	size_t max_packet_size;
	/** Additional opportunities per uframe */
	unsigned packets;
	/** Reserved bandwidth. */
	size_t bandwidth;
	/** Value of the toggle bit. */
	unsigned toggle:1;
	/** The currently active transfer batch. Write using methods, read under guard. */
	usb_transfer_batch_t *active_batch;
	/** Protects resources and active status changes. */
	fibril_mutex_t guard;
	/** Signals change of active status. */
	fibril_condvar_t avail;

	/* This structure is meant to be extended by overriding. */
} endpoint_t;

extern void endpoint_init(endpoint_t *, device_t *, const usb_endpoint_desc_t *);

extern void endpoint_add_ref(endpoint_t *);
extern void endpoint_del_ref(endpoint_t *);

/* Pay atention to synchronization of batch access wrt to aborting & finishing from another fibril. */

/* Set currently active batch. The common case is to activate in the same
 * critical section as scheduling to HW.
 */
extern void endpoint_activate_locked(endpoint_t *, usb_transfer_batch_t *);

/* Deactivate the endpoint, allowing others to activate it again. Batch shall
 * already have an error set. */
extern void endpoint_deactivate_locked(endpoint_t *);

/* Abort the currenty active batch. */
void endpoint_abort(endpoint_t *);

/* Manage the toggle bit */
extern int endpoint_toggle_get(endpoint_t *);
extern void endpoint_toggle_set(endpoint_t *, bool);

/* Calculate bandwidth */
ssize_t endpoint_count_bw(endpoint_t *, size_t);

static inline bus_t *endpoint_get_bus(endpoint_t *ep)
{
	return ep->device->bus;
}

/** list_get_instance wrapper.
 *
 * @param item Pointer to link member.
 *
 * @return Pointer to endpoint_t structure.
 *
 */
static inline endpoint_t * endpoint_get_instance(link_t *item)
{
	return item ? list_get_instance(item, endpoint_t, link) : NULL;
}

#endif

/**
 * @}
 */
