/*
 * Copyright (c) 2013 Jan Vesely
 * Copyright (c) 2018 Ondrej Hlavaty
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
 * @brief EHCI driver
 */

#include <assert.h>
#include <usb/usb.h>
#include <mem.h>
#include <macros.h>
#include <usb/host/bus.h>

#include "mem_access.h"
#include "queue_head.h"

static const uint32_t speed[] = {
	[USB_SPEED_LOW] = QH_EP_CHAR_EPS_LS,
	[USB_SPEED_FULL] = QH_EP_CHAR_EPS_FS,
	[USB_SPEED_HIGH] = QH_EP_CHAR_EPS_HS,
};

void qh_init(qh_t *instance, const endpoint_t *ep)
{
	assert(instance);
	memset(instance, 0, sizeof(*instance));

	EHCI_MEM32_WR(instance->horizontal, LINK_POINTER_TERM);
//	EHCI_MEM32_WR(instance->current, LINK_POINTER_TERM);
	EHCI_MEM32_WR(instance->next, LINK_POINTER_TERM);
	EHCI_MEM32_WR(instance->alternate, LINK_POINTER_TERM);
	if (ep == NULL) {
		/* Mark as halted and list head,
		 * used by endpoint lists as dummy */
		EHCI_MEM32_WR(instance->ep_char, QH_EP_CHAR_H_FLAG);
		EHCI_MEM32_WR(instance->status, QH_STATUS_HALTED_FLAG);
		return;
	}
	assert(ep->device->speed < ARRAY_SIZE(speed));
	EHCI_MEM32_WR(instance->ep_char,
	    QH_EP_CHAR_ADDR_SET(ep->device->address) |
	    QH_EP_CHAR_EP_SET(ep->endpoint) |
	    speed[ep->device->speed] |
	    QH_EP_CHAR_MAX_LENGTH_SET(ep->max_packet_size));
	if (ep->transfer_type == USB_TRANSFER_CONTROL) {
		if (ep->device->speed != USB_SPEED_HIGH)
			EHCI_MEM32_SET(instance->ep_char, QH_EP_CHAR_C_FLAG);
		/* Let BULK and INT use queue head managed toggle,
		 * CONTROL needs special toggle handling anyway */
		EHCI_MEM32_SET(instance->ep_char, QH_EP_CHAR_DTC_FLAG);
	}
	uint32_t ep_cap = QH_EP_CAP_C_MASK_SET(3 << 2) |
	    QH_EP_CAP_MULTI_SET(ep->packets_per_uframe);
	if (usb_speed_is_11(ep->device->speed)) {
		assert(ep->device->tt.dev != NULL);
		ep_cap |=
		    QH_EP_CAP_TT_PORT_SET(ep->device->tt.port) |
		    QH_EP_CAP_TT_ADDR_SET(ep->device->tt.dev->address);
	}
	if (ep->transfer_type == USB_TRANSFER_INTERRUPT) {
		ep_cap |= QH_EP_CAP_S_MASK_SET(3);
	}

	// TODO Figure out how to correctly use CMASK and SMASK for LS/FS
	// INT transfers. Current values are just guesses
	EHCI_MEM32_WR(instance->ep_cap, ep_cap);

	/* The rest of the fields are transfer working area, it should be ok to
	 * leave it NULL */
}

/**
 * @}
 */
