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

#include <ddf/driver.h>
#include <ddf/interrupt.h>
#include <fibril.h>
#include <fibril_synch.h>
#include <adt/list.h>
#include <ddi.h>

#include <usb/usb.h>
#include <usb/host/hcd.h>

#include "ohci_batch.h"
#include "ohci_regs.h"
#include "root_hub.h"
#include "endpoint_list.h"
#include "hw_struct/hcca.h"

/** Main OHCI driver structure */
typedef struct hc {
	/** Generic USB hc driver */
	hcd_t *generic;

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

	/** USB hub emulation structure */
	rh_t rh;
} hc_t;

int hc_get_irq_code(irq_pio_range_t [], size_t, irq_cmd_t [], size_t,
    addr_range_t *);
int hc_register_irq_handler(ddf_dev_t *, addr_range_t *, int,
    interrupt_handler_t);
int hc_register_hub(hc_t *, ddf_fun_t *);
int hc_init(hc_t *, ddf_fun_t *, addr_range_t *, bool);

/** Safely dispose host controller internal structures
 *
 * @param[in] instance Host controller structure to use.
 */
static inline void hc_fini(hc_t *instance) { /* TODO: implement*/ };

void hc_enqueue_endpoint(hc_t *instance, const endpoint_t *ep);
void hc_dequeue_endpoint(hc_t *instance, const endpoint_t *ep);

void hc_interrupt(hc_t *instance, uint32_t status);
#endif
/**
 * @}
 */
