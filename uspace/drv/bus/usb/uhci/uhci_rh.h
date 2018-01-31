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
