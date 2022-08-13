/*
 * SPDX-FileCopyrightText: 2011 Jan Vesely
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup drvusbohci
 * @{
 */
/** @file
 * @brief OHCI driver
 */

#include <assert.h>
#include <macros.h>
#include <mem.h>

#include <usb/usb.h>
#include <usb/host/utils/malloc32.h>
#include <usb/host/endpoint.h>
#include <usb/host/bus.h>

#include "mem_access.h"

#include "endpoint_descriptor.h"

/** USB direction to OHCI values translation table. */
static const uint32_t dir[] = {
	[USB_DIRECTION_IN] = ED_STATUS_D_IN,
	[USB_DIRECTION_OUT] = ED_STATUS_D_OUT,
	[USB_DIRECTION_BOTH] = ED_STATUS_D_TD,
};

/**
 * Initialize ED.
 *
 * @param instance ED structure to initialize.
 * @param ep Driver endpoint to use.
 * @param td TD to put in the list.
 *
 * If @param ep is NULL, dummy ED is initialized with only skip flag set.
 */
void ed_init(ed_t *instance, const endpoint_t *ep, const td_t *td)
{
	assert(instance);
	memset(instance, 0, sizeof(*instance));

	if (ep == NULL) {
		/*
		 * Mark as dead, used for dummy EDs at the beginning of
		 * endpoint lists.
		 */
		OHCI_MEM32_WR(instance->status, ED_STATUS_K_FLAG);
		return;
	}
	/* Non-dummy ED must have corresponding EP and TD assigned */
	assert(td);
	assert(ep);
	assert(ep->direction < ARRAY_SIZE(dir));

	/* Status: address, endpoint nr, direction mask and max packet size. */
	OHCI_MEM32_WR(instance->status,
	    ((ep->device->address & ED_STATUS_FA_MASK) << ED_STATUS_FA_SHIFT) |
	    ((ep->endpoint & ED_STATUS_EN_MASK) << ED_STATUS_EN_SHIFT) |
	    ((dir[ep->direction] & ED_STATUS_D_MASK) << ED_STATUS_D_SHIFT) |
	    ((ep->max_packet_size & ED_STATUS_MPS_MASK) << ED_STATUS_MPS_SHIFT));

	/* Low speed flag */
	if (ep->device->speed == USB_SPEED_LOW)
		OHCI_MEM32_SET(instance->status, ED_STATUS_S_FLAG);

	/* Isochronous format flag */
	// TODO: We need iTD instead of TD for iso transfers
	if (ep->transfer_type == USB_TRANSFER_ISOCHRONOUS)
		OHCI_MEM32_SET(instance->status, ED_STATUS_F_FLAG);

	/* Set TD to the list */
	const uintptr_t pa = addr_to_phys(td);
	OHCI_MEM32_WR(instance->td_head, pa & ED_TDHEAD_PTR_MASK);
	OHCI_MEM32_WR(instance->td_tail, pa & ED_TDTAIL_PTR_MASK);
}
/**
 * @}
 */
