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

/**
 * Callback to reset toggle on ED.
 *
 * @param[in] hcd_ep hcd endpoint structure
 * @param[in] toggle new value of toggle bit
 */
void ohci_ep_toggle_reset(endpoint_t *ep)
{
	ohci_endpoint_t *instance = ohci_endpoint_get(ep);
	assert(instance);
	assert(instance->ed);
	ed_toggle_set(instance->ed, 0);
}

static int ohci_device_enumerate(device_t *dev)
{
	ohci_bus_t *bus = (ohci_bus_t *) dev->bus;
	return usb2_bus_device_enumerate(&bus->helper, dev);
}

static void ohci_device_gone(device_t *dev)
{
	ohci_bus_t *bus = (ohci_bus_t *) dev->bus;
	usb2_bus_device_gone(&bus->helper, dev);
}

/** Creates new hcd endpoint representation.
 */
static endpoint_t *ohci_endpoint_create(device_t *dev,
    const usb_endpoint_descriptors_t *desc)
{
	assert(dev);

	ohci_endpoint_t *ohci_ep = calloc(1, sizeof(ohci_endpoint_t));
	if (ohci_ep == NULL)
		return NULL;

	endpoint_init(&ohci_ep->base, dev, desc);

	const errno_t err = dma_buffer_alloc(&ohci_ep->dma_buffer,
	    sizeof(ed_t) + 2 * sizeof(td_t));
	if (err) {
		free(ohci_ep);
		return NULL;
	}

	ohci_ep->ed = ohci_ep->dma_buffer.virt;

	ohci_ep->tds[0] = (td_t *) ohci_ep->ed + 1;
	ohci_ep->tds[1] = ohci_ep->tds[0] + 1;

	link_initialize(&ohci_ep->eplist_link);
	link_initialize(&ohci_ep->pending_link);
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

	dma_buffer_free(&instance->dma_buffer);
	free(instance);
}


static int ohci_register_ep(endpoint_t *ep)
{
	bus_t *bus_base = endpoint_get_bus(ep);
	ohci_bus_t *bus = (ohci_bus_t *) bus_base;
	ohci_endpoint_t *ohci_ep = ohci_endpoint_get(ep);

	const int err = usb2_bus_endpoint_register(&bus->helper, ep);
	if (err)
		return err;

	ed_init(ohci_ep->ed, ep, ohci_ep->tds[0]);
	hc_enqueue_endpoint(bus->hc, ep);
	endpoint_set_online(ep, &bus->hc->guard);

	return EOK;
}

static void ohci_unregister_ep(endpoint_t *ep)
{
	ohci_bus_t *const bus = (ohci_bus_t *) endpoint_get_bus(ep);
	hc_t *const hc = bus->hc;
	assert(ep);

	usb2_bus_endpoint_unregister(&bus->helper, ep);
	hc_dequeue_endpoint(bus->hc, ep);

	/*
	 * Now we can be sure the active transfer will not be completed,
	 * as it's out of the schedule, and HC acknowledged it.
	 */

	ohci_endpoint_t *ohci_ep = ohci_endpoint_get(ep);

	fibril_mutex_lock(&hc->guard);
	endpoint_set_offline_locked(ep);
	list_remove(&ohci_ep->pending_link);
	usb_transfer_batch_t *const batch = ep->active_batch;
	endpoint_deactivate_locked(ep);
	fibril_mutex_unlock(&hc->guard);

	if (batch) {
		batch->error = EINTR;
		batch->transferred_size = 0;
		usb_transfer_batch_finish(batch);
	}
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
	.interrupt = ohci_hc_interrupt,
	.status = ohci_hc_status,

	.device_enumerate = ohci_device_enumerate,
	.device_gone = ohci_device_gone,

	.endpoint_destroy = ohci_endpoint_destroy,
	.endpoint_create = ohci_endpoint_create,
	.endpoint_register = ohci_register_ep,
	.endpoint_unregister = ohci_unregister_ep,

	.batch_create = ohci_create_batch,
	.batch_destroy = ohci_destroy_batch,
	.batch_schedule = ohci_hc_schedule,
};


int ohci_bus_init(ohci_bus_t *bus, hc_t *hc)
{
	assert(hc);
	assert(bus);

	bus_t *bus_base = (bus_t *) bus;
	bus_init(bus_base, sizeof(device_t));
	bus_base->ops = &ohci_bus_ops;

	usb2_bus_helper_init(&bus->helper, &bandwidth_accounting_usb11);

	bus->hc = hc;

	return EOK;
}

/**
 * @}
 */
