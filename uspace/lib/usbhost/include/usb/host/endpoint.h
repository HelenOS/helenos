/*
 * Copyright (c) 2011 Jan Vesely
 * Copyright (c) 2018 Ondrej Hlavaty
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
#include <sys/time.h>
#include <usb/usb.h>
#include <usb/host/bus.h>
#include <usbhc_iface.h>

typedef struct bus bus_t;
typedef struct device device_t;
typedef struct transfer_request transfer_request_t;
typedef struct usb_transfer_batch usb_transfer_batch_t;

/**
 * Host controller side endpoint structure.
 *
 * This structure, though reference-counted, is very fragile. It is responsible
 * for synchronizing transfer batch scheduling and completion.
 *
 * To avoid situations, in which two locks must be obtained to schedule/finish
 * a transfer, the endpoint inherits a lock from the outside. Because the
 * concrete instance of mutex can be unknown at the time of initialization,
 * the HC shall pass the right lock at the time of onlining the endpoint.
 *
 * The fields used for scheduling (online, active_batch) are to be used only
 * under that guard and by functions designed for this purpose. The driver can
 * also completely avoid using this mechanism, in which case it is on its own in
 * question of transfer aborting.
 *
 * Relevant information can be found in the documentation of HelenOS xHCI
 * project.
 */
typedef struct endpoint {
	/** USB device */
	device_t *device;
	/** Reference count. */
	atomic_t refcnt;

	/** An inherited guard */
	fibril_mutex_t *guard;
	/** Whether it's allowed to schedule on this endpoint */
	bool online;
	/** The currently active transfer batch. */
	usb_transfer_batch_t *active_batch;
	/** Signals change of active status. */
	fibril_condvar_t avail;

	/** Endpoint number */
	usb_endpoint_t endpoint;
	/** Communication direction. */
	usb_direction_t direction;
	/** USB transfer type. */
	usb_transfer_type_t transfer_type;
	/** Maximum size of one packet */
	size_t max_packet_size;

	/** Maximum size of one transfer */
	size_t max_transfer_size;

	/* Policies for transfer buffers */
	/** A hint for optimal performance. */
	dma_policy_t transfer_buffer_policy;
	/** Enforced by the library. */
	dma_policy_t required_transfer_buffer_policy;

	/**
	 * Number of packets that can be sent in one service interval
	 * (not necessarily uframe, despite its name)
	 */
	unsigned packets_per_uframe;

	/* This structure is meant to be extended by overriding. */
} endpoint_t;

extern void endpoint_init(endpoint_t *, device_t *,
    const usb_endpoint_descriptors_t *);

extern void endpoint_add_ref(endpoint_t *);
extern void endpoint_del_ref(endpoint_t *);

extern void endpoint_set_online(endpoint_t *, fibril_mutex_t *);
extern void endpoint_set_offline_locked(endpoint_t *);

extern void endpoint_wait_timeout_locked(endpoint_t *ep, suseconds_t);
extern int endpoint_activate_locked(endpoint_t *, usb_transfer_batch_t *);
extern void endpoint_deactivate_locked(endpoint_t *);

int endpoint_send_batch(endpoint_t *, const transfer_request_t *);

static inline bus_t *endpoint_get_bus(endpoint_t *ep)
{
	device_t *const device = ep->device;
	return device ? device->bus : NULL;
}

#endif

/**
 * @}
 */
