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
	/** interrupt transfer waiting for an actual interrupt to occur */
	usb_transfer_batch_t *unfinished_interrupt_transfer;
	bool reset_flag[EHCI_MAX_PORTS];
	bool resume_flag[EHCI_MAX_PORTS];
} ehci_rh_t;

errno_t ehci_rh_init(ehci_rh_t *instance, ehci_caps_regs_t *caps, ehci_regs_t *regs,
    const char *name);
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
