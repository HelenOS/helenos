/*
 * Copyright (c) 2017 Jaroslav Jindrak
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
 * Scratchpad buffer array bookkeeping.
 */

#include <errno.h>
#include <usb/debug.h>
#include "hc.h"
#include "hw_struct/regs.h"
#include "scratchpad.h"

static inline unsigned xhci_scratchpad_count(xhci_hc_t *hc)
{
	unsigned lo, hi;

	lo = XHCI_REG_RD(hc->cap_regs, XHCI_CAP_MAX_SPBUF_LO);
	hi = XHCI_REG_RD(hc->cap_regs, XHCI_CAP_MAX_SPBUF_HI);

	return (hi << 5) | lo;
}

int xhci_scratchpad_alloc(xhci_hc_t *hc)
{
	const unsigned num_bufs = xhci_scratchpad_count(hc);

	if (!num_bufs)
		return EOK;

	if (dma_buffer_alloc(&hc->scratchpad_array, num_bufs * sizeof(uint64_t)))
		return ENOMEM;
	uint64_t *phys_array = hc->scratchpad_array.virt;

	hc->dcbaa[0] = host2xhci(64, hc->scratchpad_array.phys);

	hc->scratchpad_buffers = calloc(num_bufs, sizeof(dma_buffer_t));
	if (!hc->scratchpad_buffers)
		goto err_dma;

	for (unsigned i = 0; i < num_bufs; ++i) {
		dma_buffer_t *cur = &hc->scratchpad_buffers[i];
		if (dma_buffer_alloc(cur, PAGE_SIZE))
			goto err_buffers;

		memset(cur->virt, 0, PAGE_SIZE);
		phys_array[i] = host2xhci(64, cur->phys);
	}


	usb_log_debug2("Allocated %d scratchpad buffers.", num_bufs);

	return EOK;

err_buffers:
	for (unsigned i = 0; i < num_bufs; ++i)
		dma_buffer_free(&hc->scratchpad_buffers[i]);
	free(hc->scratchpad_buffers);
err_dma:
	hc->dcbaa[0] = 0;
	dma_buffer_free(&hc->scratchpad_array);
	return ENOMEM;
}

void xhci_scratchpad_free(xhci_hc_t *hc)
{
	const unsigned num_bufs = xhci_scratchpad_count(hc);

	if (!num_bufs)
		return;

	hc->dcbaa[0] = 0;
	dma_buffer_free(&hc->scratchpad_array);
	for (unsigned i = 0; i < num_bufs; ++i)
		dma_buffer_free(&hc->scratchpad_buffers[i]);
	free(hc->scratchpad_buffers);
	return;
}

/**
 * @}
 */
