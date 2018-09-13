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

/** @addtogroup drvusbehci
 * @{
 */
/** @file
 * @brief EHCI driver
 */

#include <assert.h>
#include <stdlib.h>
#include <usb/host/bandwidth.h>
#include <usb/debug.h>

#include "ehci_bus.h"
#include "ehci_batch.h"
#include "hc.h"

/**
 * Callback to set toggle on ED.
 *
 * @param[in] hcd_ep hcd endpoint structure
 * @param[in] toggle new value of toggle bit
 */
void ehci_ep_toggle_reset(endpoint_t *ep)
{
	ehci_endpoint_t *instance = ehci_endpoint_get(ep);
	if (qh_toggle_from_td(instance->qh))
		usb_log_warning("EP(%p): Resetting toggle bit for transfer "
		    "directed EP", instance);
	qh_toggle_set(instance->qh, 0);
}

static int ehci_device_enumerate(device_t *dev)
{
	ehci_bus_t *bus = (ehci_bus_t *) dev->bus;
	return usb2_bus_device_enumerate(&bus->helper, dev);
}

static void ehci_device_gone(device_t *dev)
{
	ehci_bus_t *bus = (ehci_bus_t *) dev->bus;
	usb2_bus_device_gone(&bus->helper, dev);
}

/** Creates new hcd endpoint representation.
 */
static endpoint_t *ehci_endpoint_create(device_t *dev,
    const usb_endpoint_descriptors_t *desc)
{
	assert(dev);

	ehci_endpoint_t *ehci_ep = malloc(sizeof(ehci_endpoint_t));
	if (ehci_ep == NULL)
		return NULL;

	endpoint_init(&ehci_ep->base, dev, desc);

	if (dma_buffer_alloc(&ehci_ep->dma_buffer, sizeof(qh_t)))
		return NULL;

	ehci_ep->qh = ehci_ep->dma_buffer.virt;

	link_initialize(&ehci_ep->eplist_link);
	link_initialize(&ehci_ep->pending_link);
	return &ehci_ep->base;
}

/** Disposes hcd endpoint structure
 *
 * @param[in] hcd driver using this instance.
 * @param[in] ep endpoint structure.
 */
static void ehci_endpoint_destroy(endpoint_t *ep)
{
	assert(ep);
	ehci_endpoint_t *instance = ehci_endpoint_get(ep);

	dma_buffer_free(&instance->dma_buffer);
	free(instance);
}

static int ehci_register_ep(endpoint_t *ep)
{
	bus_t *bus_base = endpoint_get_bus(ep);
	ehci_bus_t *bus = (ehci_bus_t *) bus_base;
	ehci_endpoint_t *ehci_ep = ehci_endpoint_get(ep);

	const int err = usb2_bus_endpoint_register(&bus->helper, ep);
	if (err)
		return err;

	qh_init(ehci_ep->qh, ep);
	hc_enqueue_endpoint(bus->hc, ep);
	endpoint_set_online(ep, &bus->hc->guard);
	return EOK;
}

static void ehci_unregister_ep(endpoint_t *ep)
{
	bus_t *bus_base = endpoint_get_bus(ep);
	ehci_bus_t *bus = (ehci_bus_t *) bus_base;
	hc_t *hc = bus->hc;
	assert(bus);
	assert(ep);

	usb2_bus_endpoint_unregister(&bus->helper, ep);
	hc_dequeue_endpoint(hc, ep);
	/*
	 * Now we can be sure the active transfer will not be completed,
	 * as it's out of the schedule, and HC acknowledged it.
	 */

	ehci_endpoint_t *ehci_ep = ehci_endpoint_get(ep);

	fibril_mutex_lock(&hc->guard);
	endpoint_set_offline_locked(ep);
	list_remove(&ehci_ep->pending_link);
	usb_transfer_batch_t *const batch = ep->active_batch;
	endpoint_deactivate_locked(ep);
	fibril_mutex_unlock(&hc->guard);

	if (batch) {
		batch->error = EINTR;
		batch->transferred_size = 0;
		usb_transfer_batch_finish(batch);
	}
}

static usb_transfer_batch_t *ehci_create_batch(endpoint_t *ep)
{
	ehci_transfer_batch_t *batch = ehci_transfer_batch_create(ep);
	return &batch->base;
}

static void ehci_destroy_batch(usb_transfer_batch_t *batch)
{
	ehci_transfer_batch_destroy(ehci_transfer_batch_get(batch));
}

static const bus_ops_t ehci_bus_ops = {
	.interrupt = ehci_hc_interrupt,
	.status = ehci_hc_status,

	.device_enumerate = ehci_device_enumerate,
	.device_gone = ehci_device_gone,

	.endpoint_destroy = ehci_endpoint_destroy,
	.endpoint_create = ehci_endpoint_create,
	.endpoint_register = ehci_register_ep,
	.endpoint_unregister = ehci_unregister_ep,

	.batch_create = ehci_create_batch,
	.batch_destroy = ehci_destroy_batch,
	.batch_schedule = ehci_hc_schedule,
};

int ehci_bus_init(ehci_bus_t *bus, hc_t *hc)
{
	assert(hc);
	assert(bus);

	bus_t *bus_base = (bus_t *) bus;
	bus_init(bus_base, sizeof(device_t));
	bus_base->ops = &ehci_bus_ops;

	usb2_bus_helper_init(&bus->helper, &bandwidth_accounting_usb2);

	bus->hc = hc;

	return EOK;
}

/**
 * @}
 */
