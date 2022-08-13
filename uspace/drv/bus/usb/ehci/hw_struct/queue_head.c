/*
 * SPDX-FileCopyrightText: 2013 Jan Vesely
 * SPDX-FileCopyrightText: 2018 Ondrej Hlavaty
 *
 * SPDX-License-Identifier: BSD-3-Clause
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
#if 0
	EHCI_MEM32_WR(instance->current, LINK_POINTER_TERM);
#endif
	EHCI_MEM32_WR(instance->next, LINK_POINTER_TERM);
	EHCI_MEM32_WR(instance->alternate, LINK_POINTER_TERM);
	if (ep == NULL) {
		/*
		 * Mark as halted and list head,
		 * used by endpoint lists as dummy
		 */
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
		/*
		 * Let BULK and INT use queue head managed toggle,
		 * CONTROL needs special toggle handling anyway
		 */
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

	/*
	 * The rest of the fields are transfer working area, it should be ok to
	 * leave it NULL
	 */
}

/**
 * @}
 */
