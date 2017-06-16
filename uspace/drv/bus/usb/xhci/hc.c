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

#include <errno.h>
#include <usb/debug.h>
#include <usb/host/ddf_helpers.h>
#include "debug.h"
#include "hc.h"

const ddf_hc_driver_t xhci_ddf_hc_driver = {
	.hc_speed = USB_SPEED_SUPER,
	.init = xhci_hc_init,
	.fini = xhci_hc_fini,
	.name = "XHCI-PCI",
	.ops = {
		.schedule       = xhci_hc_schedule,
		.irq_hook       = xhci_hc_interrupt,
		.status_hook    = xhci_hc_status,
	}
};

int xhci_hc_init(hcd_t *hcd, const hw_res_list_parsed_t *hw_res, bool irq)
{
	int err;

	assert(hcd);
	assert(hcd_get_driver_data(hcd) == NULL);

	if (hw_res->mem_ranges.count != 1) {
		usb_log_debug("Unexpected MMIO area, bailing out.");
		return EINVAL;
	}

	xhci_hc_t *hc = malloc(sizeof(xhci_hc_t));
	if (!hc)
		return ENOMEM;

	addr_range_t mmio_range = hw_res->mem_ranges.ranges[0];

	usb_log_debug("MMIO area at %p (size %zu), IRQ %d.\n",
	    RNGABSPTR(mmio_range), RNGSZ(mmio_range), hw_res->irqs.irqs[0]);

	if ((err = pio_enable_range(&mmio_range, (void **)&hc->cap_regs)))
		goto err_hc;

	xhci_dump_cap_regs(hc->cap_regs);

	uintptr_t base = (uintptr_t) hc->cap_regs;

	hc->op_regs = (xhci_op_regs_t *) (base + XHCI_REG_RD(hc->cap_regs, XHCI_CAP_LENGTH));
	hc->rt_regs = (xhci_rt_regs_t *) (base + XHCI_REG_RD(hc->cap_regs, XHCI_CAP_RTSOFF));
	hc->db_arry = (xhci_doorbell_t *) (base + XHCI_REG_RD(hc->cap_regs, XHCI_CAP_DBOFF));

	usb_log_debug("Initialized MMIO reg areas:");
	usb_log_debug("\tCapability regs: %p", hc->cap_regs);
	usb_log_debug("\tOperational regs: %p", hc->op_regs);
	usb_log_debug("\tRuntime regs: %p", hc->rt_regs);
	usb_log_debug("\tDoorbell array base: %p", hc->db_arry);

	xhci_dump_state(hc);

	hcd_set_implementation(hcd, hc, &xhci_ddf_hc_driver.ops);

	// TODO: check if everything fits into the mmio_area

	return EOK;

err_hc:
	free(hc);
	return err;
}

int xhci_hc_status(hcd_t *hcd, uint32_t *status)
{
	usb_log_info("status");
	return ENOTSUP;
}

int xhci_hc_schedule(hcd_t *hcd, usb_transfer_batch_t *batch)
{
	usb_log_info("schedule");
	return ENOTSUP;
}

void xhci_hc_interrupt(hcd_t *hcd, uint32_t status)
{
	usb_log_info("Interrupted!");
}

void xhci_hc_fini(hcd_t *hcd)
{
	assert(hcd);
	usb_log_info("Finishing");

	xhci_hc_t *hc = hcd_get_driver_data(hcd);
	free(hc);
	hcd_set_implementation(hcd, NULL, NULL);
}


/**
 * @}
 */
