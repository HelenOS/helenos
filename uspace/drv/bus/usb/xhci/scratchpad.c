/*
 * Copyright (c) 2018 Jaroslav Jindrak, Ondrej Hlavaty
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
#include <align.h>
#include "hw_struct/regs.h"
#include "scratchpad.h"

/**
 * Get the number of scratchpad buffers needed.
 */
static inline unsigned xhci_scratchpad_count(xhci_hc_t *hc)
{
	unsigned lo, hi;

	lo = XHCI_REG_RD(hc->cap_regs, XHCI_CAP_MAX_SPBUF_LO);
	hi = XHCI_REG_RD(hc->cap_regs, XHCI_CAP_MAX_SPBUF_HI);

	return (hi << 5) | lo;
}

/**
 * Allocate all scratchpad buffers, and configure the xHC.
 */
errno_t xhci_scratchpad_alloc(xhci_hc_t *hc)
{
	const unsigned num_bufs = xhci_scratchpad_count(hc);

	if (!num_bufs)
		return EOK;

	const unsigned array_size = ALIGN_UP(num_bufs * sizeof(uint64_t), PAGE_SIZE);
	const size_t size = array_size + num_bufs * PAGE_SIZE;

	if (dma_buffer_alloc(&hc->scratchpad_array, size))
		return ENOMEM;

	memset(hc->scratchpad_array.virt, 0, size);

	const char *base = hc->scratchpad_array.virt + array_size;
	uint64_t *array = hc->scratchpad_array.virt;

	for (unsigned i = 0; i < num_bufs; ++i) {
		array[i] = host2xhci(64, dma_buffer_phys(&hc->scratchpad_array,
		    base + i * PAGE_SIZE));
	}

	hc->dcbaa[0] = host2xhci(64, dma_buffer_phys_base(&hc->scratchpad_array));

	usb_log_debug("Allocated %d scratchpad buffers.", num_bufs);

	return EOK;
}

/**
 * Deallocate the scratchpads and deconfigure xHC.
 */
void xhci_scratchpad_free(xhci_hc_t *hc)
{
	const unsigned num_bufs = xhci_scratchpad_count(hc);

	if (!num_bufs)
		return;

	hc->dcbaa[0] = 0;
	dma_buffer_free(&hc->scratchpad_array);
	return;
}

/**
 * @}
 */
