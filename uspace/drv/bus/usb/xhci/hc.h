/*
 * Copyright (c) 2017 Ondrej Hlavaty
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

/** @addtogroup drvusbxhci
 * @{
 */
/** @file
 * @brief The host controller data bookkeeping.
 */

#ifndef XHCI_HC_H
#define XHCI_HC_H

#include <usb/host/usb_transfer_batch.h>
#include "hw_struct/regs.h"
#include "hw_struct/context.h"
#include "scratchpad.h"
#include "trb_ring.h"
#include "rh.h"
#include "bus.h"

typedef struct xhci_virt_device_ctx {
	xhci_device_ctx_t *dev_ctx;
	xhci_trb_ring_t *trs[XHCI_EP_COUNT];
} xhci_virt_device_ctx_t;

typedef struct xhci_hc {
	/* MMIO range */
	addr_range_t mmio_range;
	void *base;

	/* Mapped register sets */
	xhci_cap_regs_t *cap_regs;
	xhci_op_regs_t *op_regs;
	xhci_rt_regs_t *rt_regs;
	xhci_doorbell_t *db_arry;
	xhci_extcap_t *xecp;		/**< First extended capability */
	xhci_legsup_t *legsup;		/**< Legacy support capability */

	/* Structures in allocated memory */
	xhci_trb_ring_t command_ring;
	xhci_event_ring_t event_ring;
	uint64_t *dcbaa;
	xhci_virt_device_ctx_t *dcbaa_virt;
	xhci_scratchpad_t *scratchpad;

	/* Root hub emulation */
	xhci_rh_t rh;

	/* Bus bookkeeping */
	xhci_bus_t bus;

	/* Cached capabilities */
	unsigned max_slots;
	bool ac64;

	/* Command list */
	list_t commands;
	list_t transfers;
} xhci_hc_t;

int hc_init_mmio(xhci_hc_t *, const hw_res_list_parsed_t *);
int hc_init_memory(xhci_hc_t *);
int hc_claim(xhci_hc_t *, ddf_dev_t *);
int hc_irq_code_gen(irq_code_t *, xhci_hc_t *, const hw_res_list_parsed_t *);
int hc_start(xhci_hc_t *, bool);
int hc_schedule(xhci_hc_t *hc, usb_transfer_batch_t *batch);
int hc_status(xhci_hc_t *, uint32_t *);
void hc_interrupt(xhci_hc_t *, uint32_t);
void hc_fini(xhci_hc_t *);
int hc_ring_doorbell(xhci_hc_t *, unsigned, unsigned);

#endif

/**
 * @}
 */
