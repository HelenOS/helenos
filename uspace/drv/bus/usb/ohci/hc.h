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

/** @addtogroup drvusbohci
 * @{
 */
/** @file
 * @brief OHCI host controller driver structure
 */

#ifndef DRV_OHCI_HC_H
#define DRV_OHCI_HC_H

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

#include "ohci_regs.h"
#include "ohci_rh.h"
#include "endpoint_list.h"
#include "hw_struct/hcca.h"

/** Main OHCI driver structure */
typedef struct hc {
	/** Memory mapped I/O registers area */
	ohci_regs_t *registers;
	/** Host controller communication area structure */
	hcca_t *hcca;

	/** Transfer schedules */
	endpoint_list_t lists[4];
	/** List of active transfers */
	list_t pending_batches;

	/** Fibril for periodic checks if interrupts can't be used */
	fid_t interrupt_emulator;

	/** Guards schedule and endpoint manipulation */
	fibril_mutex_t guard;

	/** interrupts available */
	bool hw_interrupts;

	/** USB hub emulation structure */
	ohci_rh_t rh;
} hc_t;

extern errno_t hc_init(hc_t *, const hw_res_list_parsed_t *, bool);
extern void hc_fini(hc_t *);

extern void hc_enqueue_endpoint(hc_t *, const endpoint_t *);
extern void hc_dequeue_endpoint(hc_t *, const endpoint_t *);

errno_t ohci_hc_gen_irq_code(irq_code_t *code, const hw_res_list_parsed_t *hw_res, int *irq);

extern void ohci_hc_interrupt(hcd_t *, uint32_t);
extern errno_t ohci_hc_status(hcd_t *, uint32_t *);
extern errno_t ohci_hc_schedule(hcd_t *, usb_transfer_batch_t *);

#endif

/**
 * @}
 */
