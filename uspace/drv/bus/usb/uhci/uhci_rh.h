/*
 * SPDX-FileCopyrightText: 2013 Jan Vesely
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup drvusbuhci
 * @{
 */
/** @file
 * @brief UHCI host controller driver structure
 */
#ifndef DRV_UHCI_UHCI_RH_H
#define DRV_UHCI_UHCI_RH_H

#include <usbvirt/virthub_base.h>
#include <usb/host/usb_transfer_batch.h>
#include <usb/usb.h>

#include <stdbool.h>
#include <ddi.h>

/** Endpoint number for status change pipe. */
#define HUB_STATUS_CHANGE_PIPE   1

/** Virtual to UHCI hub connector */
typedef struct {
	/** Virtual hub software implementation */
	virthub_base_t base;
	/** UHCI root hub port io registers */
	ioport16_t *ports[2];
	/** Reset change indicator, it is not reported by regs */
	bool reset_changed[2];
} uhci_rh_t;

errno_t uhci_rh_init(uhci_rh_t *instance, ioport16_t *ports, const char *name);
errno_t uhci_rh_schedule(uhci_rh_t *instance, usb_transfer_batch_t *batch);

/** Get UHCI rh address.
 *
 * @param instance UHCI rh instance.
 * @return USB address assigned to the hub.
 * Wrapper for virtual hub address
 */
static inline usb_address_t uhci_rh_get_address(uhci_rh_t *instance)
{
	return virthub_base_get_address(&instance->base);
}

#endif
/**
 * @}
 */
