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

/** @addtogroup drvusbohci
 * @{
 */
/** @file
 * @brief OHCI driver
 */

#include <assert.h>
#include <stdlib.h>
#include <usb/host/utils/malloc32.h>
#include <usb/host/bandwidth.h>

#include "ohci_bus.h"
#include "ohci_batch.h"
#include "hc.h"

/** Callback to set toggle on ED.
 *
 * @param[in] hcd_ep hcd endpoint structure
 * @param[in] toggle new value of toggle bit
 */
static void ohci_ep_toggle_set(endpoint_t *ep, bool toggle)
{
	ohci_endpoint_t *instance = ohci_endpoint_get(ep);
	assert(instance);
	assert(instance->ed);
	ep->toggle = toggle;
	ed_toggle_set(instance->ed, toggle);
}

/** Callback to get value of toggle bit.
 *
 * @param[in] hcd_ep hcd endpoint structure
 * @return Current value of toggle bit.
 */
static bool ohci_ep_toggle_get(endpoint_t *ep)
{
	ohci_endpoint_t *instance = ohci_endpoint_get(ep);
	assert(instance);
	assert(instance->ed);
	return ed_toggle_get(instance->ed);
}

/** Creates new hcd endpoint representation.
 */
static endpoint_t *ohci_endpoint_create(device_t *dev, const usb_endpoint_descriptors_t *desc)
{
	assert(dev);

	ohci_endpoint_t *ohci_ep = malloc(sizeof(ohci_endpoint_t));
	if (ohci_ep == NULL)
		return NULL;

	endpoint_init(&ohci_ep->base, dev, desc);

	ohci_ep->ed = malloc32(sizeof(ed_t));
	if (ohci_ep->ed == NULL) {
		free(ohci_ep);
		return NULL;
	}

	ohci_ep->td = malloc32(sizeof(td_t));
	if (ohci_ep->td == NULL) {
		free32(ohci_ep->ed);
		free(ohci_ep);
		return NULL;
	}

	link_initialize(&ohci_ep->link);
	return &ohci_ep->base;
}

/** Disposes hcd endpoint structure
 *
 * @param[in] hcd driver using this instance.
 * @param[in] ep endpoint structure.
 */
static void ohci_endpoint_destroy(endpoint_t *ep)
{
	assert(ep);
	ohci_endpoint_t *instance = ohci_endpoint_get(ep);

	free32(instance->ed);
	free32(instance->td);
	free(instance);
}


static int ohci_register_ep(endpoint_t *ep)
{
	bus_t *bus_base = endpoint_get_bus(ep);
	ohci_bus_t *bus = (ohci_bus_t *) bus_base;
	ohci_endpoint_t *ohci_ep = ohci_endpoint_get(ep);

	const int err = usb2_bus_ops.endpoint_register(ep);
	if (err)
		return err;

	ed_init(ohci_ep->ed, ep, ohci_ep->td);
	hc_enqueue_endpoint(bus->hc, ep);

	return EOK;
}

static int ohci_unregister_ep(endpoint_t *ep)
{
	ohci_bus_t *bus = (ohci_bus_t *) endpoint_get_bus(ep);
	assert(ep);

	const int err = usb2_bus_ops.endpoint_unregister(ep);
	if (err)
		return err;

	hc_dequeue_endpoint(bus->hc, ep);
	return EOK;
}

static usb_transfer_batch_t *ohci_create_batch(endpoint_t *ep)
{
	ohci_transfer_batch_t *batch = ohci_transfer_batch_create(ep);
	return &batch->base;
}

static void ohci_destroy_batch(usb_transfer_batch_t *batch)
{
	ohci_transfer_batch_destroy(ohci_transfer_batch_get(batch));
}

static const bus_ops_t ohci_bus_ops = {
	.parent = &usb2_bus_ops,

	.interrupt = ohci_hc_interrupt,
	.status = ohci_hc_status,

	.endpoint_destroy = ohci_endpoint_destroy,
	.endpoint_create = ohci_endpoint_create,
	.endpoint_register = ohci_register_ep,
	.endpoint_unregister = ohci_unregister_ep,
	.endpoint_count_bw = bandwidth_count_usb11,
	.endpoint_set_toggle = ohci_ep_toggle_set,
	.endpoint_get_toggle = ohci_ep_toggle_get,
	.batch_create = ohci_create_batch,
	.batch_destroy = ohci_destroy_batch,
	.batch_schedule = ohci_hc_schedule,
};


int ohci_bus_init(ohci_bus_t *bus, hc_t *hc)
{
	assert(hc);
	assert(bus);

	usb2_bus_t *usb2_bus = (usb2_bus_t *) bus;
	bus_t *bus_base = (bus_t *) bus;

	usb2_bus_init(usb2_bus, BANDWIDTH_AVAILABLE_USB11);
	bus_base->ops = &ohci_bus_ops;

	bus->hc = hc;

	return EOK;
}

/**
 * @}
 */
