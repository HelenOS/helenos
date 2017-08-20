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
#include <usb/host/utils/malloc32.h>
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
	unsigned num_bufs, allocated;
	xhci_scratchpad_t *bufs;

	num_bufs = xhci_scratchpad_count(hc);

	if (!num_bufs)
		return EOK;

	bufs = malloc32(sizeof(xhci_scratchpad_t));
	if (!bufs)
		return ENOMEM;

	allocated = 0;

	uint64_t *phys_array = malloc32(num_bufs * sizeof(uint64_t));
	if (phys_array == NULL)
		goto err_phys_array;

	uint64_t *virt_array = malloc32(num_bufs * sizeof(uint64_t));
	if (virt_array == NULL)
		goto err_virt_array;

	for (unsigned i = 0; i < num_bufs; ++i) {
		void *buf = malloc32(PAGE_SIZE);
		phys_array[i] = host2xhci(64, (uint64_t) addr_to_phys(buf));
		virt_array[i] = (uint64_t) buf;

		if (buf != NULL)
			++allocated;
		else
			goto err_page_alloc;

		memset(buf, 0, PAGE_SIZE);
	}

	bufs->phys_ptr = host2xhci(64, (uint64_t) addr_to_phys(phys_array));
	bufs->virt_ptr = (uint64_t) virt_array;
	bufs->phys_bck = (uint64_t) phys_array;

	hc->dcbaa[0] = bufs->phys_ptr;
	hc->scratchpad = bufs;

	usb_log_debug2("Allocated %d scratchpad buffers.", num_bufs);

	return EOK;

err_page_alloc:
	for (unsigned i = 0; i < allocated; ++i)
		free32((void *) virt_array[i]);
	free32(virt_array);

err_virt_array:
	free32(phys_array);

err_phys_array:
	free32(bufs);

	return ENOMEM;
}

void xhci_scratchpad_free(xhci_hc_t *hc)
{
	unsigned num_bufs;
	xhci_scratchpad_t *scratchpad;
	uint64_t *virt_array;

	num_bufs = xhci_scratchpad_count(hc);
	if (!num_bufs)
		return;

	scratchpad =  hc->scratchpad;
	virt_array = (uint64_t *) scratchpad->virt_ptr;

	for (unsigned i = 0; i < num_bufs; ++i)
		free32((void *) virt_array[i]);
	free32((void *) scratchpad->virt_ptr);
	free32((void *) scratchpad->phys_bck);

	hc->dcbaa[0] = 0;
	memset(&hc->dcbaa_virt[0], 0, sizeof(xhci_virt_device_ctx_t));

	return;
}

/**
 * @}
 */
