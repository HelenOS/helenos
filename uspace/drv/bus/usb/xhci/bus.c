/*
 * SPDX-FileCopyrightText: 2018 Ondrej Hlavaty, Petr Manek, Michal Staruch
 *
 * SPDX-License-Identifier: BSD-3-Clause
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
