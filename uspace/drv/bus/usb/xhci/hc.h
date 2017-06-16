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

#include <usb/host/ddf_helpers.h>
#include "hw_struct/regs.h"

typedef struct xhci_hc {
	xhci_cap_regs_t *cap_regs;
	xhci_op_regs_t *op_regs;
	xhci_rt_regs_t *rt_regs;
	xhci_doorbell_t *db_arry;
} xhci_hc_t;

extern const ddf_hc_driver_t xhci_ddf_hc_driver;

int xhci_hc_init(hcd_t *, const hw_res_list_parsed_t *, bool irq);
int xhci_hc_gen_irq_code(irq_code_t *, const hw_res_list_parsed_t *);
int xhci_hc_status(hcd_t *, uint32_t *);
int xhci_hc_schedule(hcd_t *, usb_transfer_batch_t *);
void xhci_hc_interrupt(hcd_t *, uint32_t);
void xhci_hc_fini(hcd_t *);


/**
 * @}
 */
