/*
 * Copyright (c) 2011 Jan Vesely
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
 * @brief EHCI host controller driver structure
 */
#ifndef DRV_EHCI_HC_H
#define DRV_EHCI_HC_H

#include <adt/list.h>
#include <ddi.h>
#include <ddf/driver.h>
#include <device/hw_res_parsed.h>
#include <fibril.h>
#include <fibril_synch.h>
#include <stdbool.h>
#include <stdint.h>

#include <usb/host/hcd.h>
#include <usb/host/endpoint.h>
#include <usb/host/usb_transfer_batch.h>

#include "ehci_regs.h"
#include "ehci_rh.h"
#include "hw_struct/link_pointer.h"
#include "endpoint_list.h"

/** Main EHCI driver structure */
typedef struct hc {
	/** Memory mapped CAPS register area */
	ehci_caps_regs_t *caps;
	/** Memory mapped I/O registers area */
	ehci_regs_t *registers;

	/** Iso transfer list */
	link_pointer_t *periodic_list_base;

	/** CONTROL and BULK schedules */
	endpoint_list_t async_list;

	/** INT schedule */
	endpoint_list_t int_list;

	/** List of active transfers */
	list_t pending_batches;

	/** Guards schedule and endpoint manipulation */
	fibril_mutex_t guard;

	/** Wait for hc to restart async chedule */
	fibril_condvar_t async_doorbell;

	/** USB hub emulation structure */
	ehci_rh_t rh;
} hc_t;

errno_t hc_init(hc_t *instance, const hw_res_list_parsed_t *hw_res, bool interrupts);
void hc_fini(hc_t *instance);

void hc_enqueue_endpoint(hc_t *instance, const endpoint_t *ep);
void hc_dequeue_endpoint(hc_t *instance, const endpoint_t *ep);

errno_t ehci_hc_gen_irq_code(irq_code_t *code, const hw_res_list_parsed_t *hw_res, int *irq);

void ehci_hc_interrupt(hcd_t *hcd, uint32_t status);
errno_t ehci_hc_status(hcd_t *hcd, uint32_t *status);
errno_t ehci_hc_schedule(hcd_t *hcd, usb_transfer_batch_t *batch);
#endif
/**
 * @}
 */
