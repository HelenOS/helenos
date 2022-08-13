/*
 * SPDX-FileCopyrightText: 2013 Jan Vesely
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
/** @addtogroup drvusbohci
 * @{
 */
/** @file
 * @brief OHCI driver
 */
#ifndef DRV_OHCI_OHCI_RH_H
#define DRV_OHCI_OHCI_RH_H

#include <assert.h>
#include <stdint.h>

#include <usb/usb.h>
#include <usb/classes/hub.h>
#include <usb/host/usb_transfer_batch.h>
#include <usbvirt/virthub_base.h>

#include "ohci_regs.h"

enum {
	OHCI_MAX_PORTS = 15,
};

typedef struct {
	/** Virtual hub instance */
	virthub_base_t base;
	/** OHCI device registers */
	ohci_regs_t *registers;
	/** Number of downstream ports, OHCI limits this to 15 */
	unsigned port_count;
	/** USB hub descriptor describing the OHCI root hub */
	struct {
		usb_hub_descriptor_header_t header;
		uint8_t rempow[STATUS_BYTES(OHCI_MAX_PORTS) * 2];
	} __attribute__((packed)) hub_descriptor;
	/** A hacky way to emulate interrupts over polling. See ehci_rh. */
	endpoint_t *status_change_endpoint;
	fibril_mutex_t *guard;
} ohci_rh_t;

errno_t ohci_rh_init(ohci_rh_t *, ohci_regs_t *, fibril_mutex_t *, const char *);
errno_t ohci_rh_schedule(ohci_rh_t *instance, usb_transfer_batch_t *batch);
errno_t ohci_rh_interrupt(ohci_rh_t *instance);

/** Get OHCI rh address.
 *
 * @param instance UHCI rh instance.
 * @return USB address assigned to the hub.
 * Wrapper for virtual hub address
 */
static inline usb_address_t ohci_rh_get_address(ohci_rh_t *instance)
{
	assert(instance);
	return virthub_base_get_address(&instance->base);
}
#endif
/**
 * @}
 */
