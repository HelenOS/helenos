/*
 * Copyright (c) 2018 Ondrej Hlavaty, Petr Manek, Michal Staruch
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
 * xHCI bus interface.
 */

#include <usb/descriptor.h>

#include "hc.h"
#include "device.h"
#include "endpoint.h"
#include "transfers.h"

#include "bus.h"


static const bus_ops_t xhci_bus_ops = {
	.interrupt = hc_interrupt,
	.status = hc_status,

	.device_enumerate = xhci_device_enumerate,
	.device_gone = xhci_device_gone,
	.device_online = xhci_device_online,
	.device_offline = xhci_device_offline,

	.endpoint_create = xhci_endpoint_create,
	.endpoint_destroy = xhci_endpoint_destroy,
	.endpoint_register = xhci_endpoint_register,
	.endpoint_unregister = xhci_endpoint_unregister,

	.batch_schedule = xhci_transfer_schedule,
	.batch_create = xhci_transfer_create,
	.batch_destroy = xhci_transfer_destroy,
};

/** Initialize XHCI bus.
 * @param[in] bus Allocated XHCI bus to initialize.
 * @param[in] hc Associated host controller, which manages the bus.
 *
 * @return Error code.
 */
errno_t xhci_bus_init(xhci_bus_t *bus, xhci_hc_t *hc)
{
	assert(bus);

	bus_init(&bus->base, sizeof(xhci_device_t));

	bus->devices_by_slot = calloc(hc->max_slots, sizeof(xhci_device_t *));
	if (!bus->devices_by_slot)
		return ENOMEM;

	bus->hc = hc;
	bus->base.ops = &xhci_bus_ops;
	return EOK;
}

/** Finalize XHCI bus.
 * @param[in] bus XHCI bus to finalize.
 */
void xhci_bus_fini(xhci_bus_t *bus)
{
	// FIXME: Ensure there are no more devices?
	free(bus->devices_by_slot);
	// FIXME: Something else we forgot?
}

/**
 * @}
 */
