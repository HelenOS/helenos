/*
 * Copyright (c) 2017 Petr Manek
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

/** @addtogroup drvusbxhci
 * @{
 */
/** @file
 * @brief The host controller endpoint management.
 */

#include <usb/host/endpoint.h>

#include <errno.h>

#include "bus.h"
#include "endpoint.h"

int xhci_endpoint_init(xhci_endpoint_t *xhci_ep, xhci_bus_t *xhci_bus)
{
	assert(xhci_ep);
	assert(xhci_bus);

	bus_t *bus = &xhci_bus->base;
	endpoint_t *ep = &xhci_ep->base;

	endpoint_init(ep, bus);
	xhci_ep->device = NULL;

	usb_log_debug("Endpoint %d:%d initialized.", ep->target.address, ep->target.endpoint);

	return EOK;
}

void xhci_endpoint_fini(xhci_endpoint_t *xhci_ep)
{
	assert(xhci_ep);

	/* FIXME: Tear down TR's? */

	endpoint_t *ep = &xhci_ep->base;

	usb_log_debug("Endpoint %d:%d destroyed.", ep->target.address, ep->target.endpoint);
}

int xhci_device_init(xhci_device_t *dev, xhci_bus_t *bus, usb_address_t address)
{
	memset(&dev->endpoints, 0, sizeof(dev->endpoints));
	dev->active_endpoint_count = 0;
	dev->address = address;
	dev->slot_id = 0;

	usb_log_debug("Device %d initialized.", dev->address);
	return EOK;
}

void xhci_device_fini(xhci_device_t *dev)
{
	// TODO: Check that all endpoints are dead.
	usb_log_debug("Device %d destroyed.", dev->address);
}

int xhci_device_add_endpoint(xhci_device_t *dev, xhci_endpoint_t *ep)
{
	assert(dev->address == ep->base.target.address);
	assert(!dev->endpoints[ep->base.target.endpoint]);
	assert(!ep->device);

	ep->device = dev;
	dev->endpoints[ep->base.target.endpoint] = ep;
	++dev->active_endpoint_count;
	return EOK;
}

int xhci_device_remove_endpoint(xhci_device_t *dev, xhci_endpoint_t *ep)
{
	assert(dev->address == ep->base.target.address);
	assert(dev->endpoints[ep->base.target.endpoint]);
	assert(dev == ep->device);

	ep->device = NULL;
	dev->endpoints[ep->base.target.endpoint] = NULL;
	--dev->active_endpoint_count;
	return EOK;
}

xhci_endpoint_t * xhci_device_get_endpoint(xhci_device_t *dev, usb_endpoint_t ep)
{
	return dev->endpoints[ep];
}

/**
 * @}
 */
