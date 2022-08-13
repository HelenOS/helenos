/*
 * SPDX-FileCopyrightText: 2013 Jan Vesely
 * SPDX-FileCopyrightText: 2018 Ondrej Hlavaty
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
/** @addtogroup drvusbehci
 * @{
 */
/** @file
 * @brief EHCI driver
 */
#ifndef DRV_EHCI_EHCI_RH_H
#define DRV_EHCI_EHCI_RH_H

#include <assert.h>
#include <stdint.h>

#include <usb/usb.h>
#include <usb/classes/hub.h>
#include <usb/host/usb_transfer_batch.h>
#include <usbvirt/virthub_base.h>

#include "ehci_regs.h"

enum {
	EHCI_MAX_PORTS = 15,
};

typedef struct {
	/** Virtual hub instance */
	virthub_base_t base;
	/** EHCI device registers */
	ehci_regs_t *registers;
	/** Number of downstream ports, EHCI limits this to 15 */
	unsigned port_count;
	/** USB hub descriptor describing the EHCI root hub */
	struct {
		usb_hub_descriptor_header_t header;
		uint8_t rempow[STATUS_BYTES(EHCI_MAX_PORTS) * 2];
	} __attribute__((packed)) hub_descriptor;
	bool reset_flag[EHCI_MAX_PORTS];
	bool resume_flag[EHCI_MAX_PORTS];

	/* HC guard */
	fibril_mutex_t *guard;

	/*
	 * This is sort of hacky, but better than duplicating functionality.
	 * We cannot simply store a pointer to a transfer in-progress, in order
	 * to allow it to be aborted. We can however store a reference to the
	 * Status Change Endpoint. Note that this is mixing two worlds together
	 * - otherwise, the RH is "a device" and have no clue about HC, apart
	 * from accessing its registers.
	 */
	endpoint_t *status_change_endpoint;

} ehci_rh_t;

errno_t ehci_rh_init(ehci_rh_t *instance, ehci_caps_regs_t *caps,
    ehci_regs_t *regs, fibril_mutex_t *guard, const char *name);
errno_t ehci_rh_schedule(ehci_rh_t *instance, usb_transfer_batch_t *batch);
errno_t ehci_rh_interrupt(ehci_rh_t *instance);

/** Get EHCI rh address.
 *
 * @param instance UHCI rh instance.
 * @return USB address assigned to the hub.
 * Wrapper for virtual hub address
 */
static inline usb_address_t ehci_rh_get_address(ehci_rh_t *instance)
{
	assert(instance);
	return virthub_base_get_address(&instance->base);
}
#endif
/**
 * @}
 */
