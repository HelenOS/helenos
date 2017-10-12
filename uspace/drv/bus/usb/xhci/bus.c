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
/**  @addtogroup libusbhost
 * @{
 */
/** @file
 * HC Endpoint management.
 */

#include <usb/host/endpoint.h>
#include <usb/debug.h>

#include <assert.h>
#include <errno.h>
#include <macros.h>
#include <stdbool.h>

#include "bus.h"
#include "endpoint.h"

/** Ops receive generic bus_t pointer. */
static inline xhci_bus_t *bus_to_xhci_bus(bus_t *bus_base)
{
	assert(bus_base);
	return (xhci_bus_t *) bus_base;
}

static endpoint_t *create_endpoint(bus_t *base)
{
	xhci_bus_t *bus = bus_to_xhci_bus(base);

	xhci_endpoint_t *ep = malloc(sizeof(xhci_endpoint_t));
	if (!ep)
		return NULL;

	if (xhci_endpoint_init(ep, bus)) {
		free(ep);
		return NULL;
	}

	return &ep->base;
}

static void destroy_endpoint(endpoint_t *ep)
{
	xhci_endpoint_t *xhci_ep = xhci_endpoint_get(ep);

	xhci_endpoint_fini(xhci_ep);
	free(xhci_ep);
}

/** TODO: Implement missing ops. */
static const bus_ops_t xhci_bus_ops = {
	.create_endpoint = create_endpoint,
	.destroy_endpoint = destroy_endpoint,

	.register_endpoint = NULL,
	.release_endpoint = NULL,
	.find_endpoint = NULL,

	.request_address = NULL,
	.get_speed = NULL,
	.release_address = NULL,
	.reset_toggle = NULL,

	.count_bw = NULL,

	.endpoint_get_toggle = NULL,
	.endpoint_set_toggle = NULL,
};

int xhci_bus_init(xhci_bus_t *bus, hcd_t *hcd)
{
	assert(bus);

	bus_init(&bus->base, hcd);

	bus->base.ops = xhci_bus_ops;
	return EOK;
}
/**
 * @}
 */
