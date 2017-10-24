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
static endpoint_t *ohci_endpoint_create(bus_t *bus)
{
	assert(bus);

	ohci_endpoint_t *ohci_ep = malloc(sizeof(ohci_endpoint_t));
	if (ohci_ep == NULL)
		return NULL;

	endpoint_init(&ohci_ep->base, bus);

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


static int ohci_register_ep(bus_t *bus_base, endpoint_t *ep, const usb_endpoint_desc_t *desc)
{
	ohci_bus_t *bus = (ohci_bus_t *) bus_base;
	ohci_endpoint_t *ohci_ep = ohci_endpoint_get(ep);

	const int err = bus->parent_ops.register_endpoint(bus_base, ep, desc);
	if (err)
		return err;

	ed_init(ohci_ep->ed, ep, ohci_ep->td);
	hc_enqueue_endpoint(bus->hc, ep);

	return EOK;
}

static int ohci_unregister_ep(bus_t *bus_base, endpoint_t *ep)
{
	ohci_bus_t *bus = (ohci_bus_t *) bus_base;
	assert(bus);
	assert(ep);

	const int err = bus->parent_ops.unregister_endpoint(bus_base, ep);
	if (err)
		return err;

	hc_dequeue_endpoint(bus->hc, ep);
	return EOK;
}

static usb_transfer_batch_t *ohci_bus_create_batch(bus_t *bus, endpoint_t *ep)
{
	ohci_transfer_batch_t *batch = ohci_transfer_batch_create(ep);
	return &batch->base;
}

static void ohci_bus_destroy_batch(usb_transfer_batch_t *batch)
{
	ohci_transfer_batch_destroy(ohci_transfer_batch_get(batch));
}

int ohci_bus_init(ohci_bus_t *bus, hc_t *hc)
{
	assert(hc);
	assert(bus);

	usb2_bus_init(&bus->base, BANDWIDTH_AVAILABLE_USB11, bandwidth_count_usb11);

	bus_ops_t *ops = &bus->base.base.ops;
	bus->parent_ops = *ops;
	ops->create_endpoint = ohci_endpoint_create;
	ops->destroy_endpoint = ohci_endpoint_destroy;
	ops->endpoint_set_toggle = ohci_ep_toggle_set;
	ops->endpoint_get_toggle = ohci_ep_toggle_get;

	ops->register_endpoint = ohci_register_ep;
	ops->unregister_endpoint = ohci_unregister_ep;

	ops->create_batch = ohci_bus_create_batch;
	ops->destroy_batch = ohci_bus_destroy_batch;

	bus->hc = hc;

	return EOK;
}

/**
 * @}
 */
