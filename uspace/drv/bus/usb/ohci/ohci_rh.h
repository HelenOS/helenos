/*
 * Copyright (c) 2013 Jan Vesely
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
