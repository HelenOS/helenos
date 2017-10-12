/*
 * Copyright (c) 2017 Ondrej Hlavaty <aearsis@eideo.cz>
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
 * Virtual base for usb bus implementations.
 *
 * The purpose of this structure is to keep information about connected devices
 * and enpoints, manage available bandwidth and the toggle bit flipping.
 *
 * The generic implementation is provided for USB 1 and 2 in usb2_bus.c. Some
 * details in [OUE]HCI are solved through overriding some functions. XHCI does
 * not need the bookkeeping functionality, because addresses are managed by HC
 * itself.
 */
#ifndef LIBUSBHOST_HOST_BUS_H
#define LIBUSBHOST_HOST_BUS_H

#include <usb/usb.h>

#include <fibril_synch.h>
#include <stdbool.h>

typedef struct hcd hcd_t;
typedef struct endpoint endpoint_t;
typedef struct bus bus_t;

typedef struct {
	endpoint_t *(*create_endpoint)(bus_t *);
	int (*register_endpoint)(bus_t *, endpoint_t *);
	int (*release_endpoint)(bus_t *, endpoint_t *);
	endpoint_t *(*find_endpoint)(bus_t *, usb_target_t, usb_direction_t);

	int (*request_address)(bus_t *, usb_address_t*, bool, usb_speed_t);
	int (*get_speed)(bus_t *, usb_address_t, usb_speed_t *);
	int (*release_address)(bus_t *, usb_address_t);

	int (*reset_toggle)(bus_t *, usb_target_t, bool);

	size_t (*count_bw) (endpoint_t *, size_t);

	/* Endpoint ops, optional (have generic fallback) */
	void (*destroy_endpoint)(endpoint_t *);
	int (*endpoint_get_toggle)(endpoint_t *);
	void (*endpoint_set_toggle)(endpoint_t *, unsigned);
} bus_ops_t;

/** Endpoint management structure */
typedef struct bus {
	hcd_t *hcd;

	/* Synchronization of ops */
	fibril_mutex_t guard;

	/* Do not call directly, ops are synchronized. */
	bus_ops_t ops;

	/* This structure is meant to be extended by overriding. */
} bus_t;

void bus_init(bus_t *, hcd_t *hcd);

endpoint_t *bus_create_endpoint(bus_t *);
int bus_register_endpoint(bus_t *, endpoint_t *);
int bus_release_endpoint(bus_t *, endpoint_t *);
endpoint_t *bus_find_endpoint(bus_t *, usb_target_t, usb_direction_t);

size_t bus_count_bw(endpoint_t *, size_t);

int bus_request_address(bus_t *, usb_address_t *, bool, usb_speed_t);
int bus_get_speed(bus_t *, usb_address_t, usb_speed_t *);
int bus_release_address(bus_t *, usb_address_t);

int bus_reset_toggle(bus_t *, usb_target_t, bool);

#endif
/**
 * @}
 */
