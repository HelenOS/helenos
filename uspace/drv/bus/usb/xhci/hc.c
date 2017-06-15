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
#include "debug.h"
#include "hc.h"

int xhci_hc_gen_irq_code(irq_code_t *code, const hw_res_list_parsed_t *hw_res)
{
	assert(code);
	assert(hw_res);

	usb_log_debug("Gen IRQ code, got %zu IRQs, %zu DMA chs, %zu mem rngs, %zu IO rngs",
	    hw_res->irqs.count,
	    hw_res->dma_channels.count,
	    hw_res->mem_ranges.count,
	    hw_res->io_ranges.count);
	
	if (hw_res->irqs.count != 1
		|| hw_res->dma_channels.count != 0
		|| hw_res->mem_ranges.count != 1
		|| hw_res->io_ranges.count != 0) {
		usb_log_debug("Unexpected HW resources, bailing out.");
		return EINVAL;
	}

	addr_range_t mmio_range = hw_res->mem_ranges.ranges[0];

	usb_log_debug("Memory mapped regs at %p (size %zu), IRQ %d.\n",
	    RNGABSPTR(mmio_range), RNGSZ(mmio_range), hw_res->irqs.irqs[0]);

	xhci_cap_regs_t *cap_regs = NULL;
	int ret = pio_enable_range(&mmio_range, (void **)&cap_regs);
	if (ret != EOK)
		return ret;

	xhci_dump_cap_regs(cap_regs);

	/*
	 * XHCI uses an Interrupter mechanism. Possibly, we want to set it up here.
	 */
	code->rangecount = 0;
	code->ranges = NULL;
	code->cmdcount = 0;
	code->cmds = NULL;

	return EOK;
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


/**
 * @}
 */
