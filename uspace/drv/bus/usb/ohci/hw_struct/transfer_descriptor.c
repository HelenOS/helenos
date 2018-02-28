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
 * @brief OHCI driver
 */

#include <assert.h>
#include <mem.h>

#include <usb/usb.h>
#include <usb/host/utils/malloc32.h>

#include "completion_codes.h"
#include "mem_access.h"
#include "transfer_descriptor.h"

/** USB direction to OHCI TD values translation table */
static const uint32_t dir[] = {
	[USB_DIRECTION_IN] = TD_STATUS_DP_IN,
	[USB_DIRECTION_OUT] = TD_STATUS_DP_OUT,
	[USB_DIRECTION_BOTH] = TD_STATUS_DP_SETUP,
};

/**
 * Initialize OHCI TD.
 * @param instance TD structure to initialize.
 * @param next Next TD in ED list.
 * @param direction Used to determine PID, BOTH means setup PID.
 * @param buffer Pointer to the first byte of transferred data.
 * @param size Size of the buffer.
 * @param toggle Toggle bit value, use 0/1 to set explicitly,
 *        any other value means that ED toggle will be used.
 */
void td_init(td_t *instance, const td_t *next,
    usb_direction_t direction, const void *buffer, size_t size, int toggle)
{
	assert(instance);
	memset(instance, 0, sizeof(td_t));
	/* Set PID and Error code */
	OHCI_MEM32_WR(instance->status,
	    ((dir[direction] & TD_STATUS_DP_MASK) << TD_STATUS_DP_SHIFT)
	    | ((CC_NOACCESS2 & TD_STATUS_CC_MASK) << TD_STATUS_CC_SHIFT));

	if (toggle == 0 || toggle == 1) {
		/* Set explicit toggle bit */
		OHCI_MEM32_SET(instance->status, TD_STATUS_T_USE_TD_FLAG);
		OHCI_MEM32_SET(instance->status, toggle ? TD_STATUS_T_FLAG : 0);
	}

	/* Allow less data on input. */
	if (direction == USB_DIRECTION_IN) {
		OHCI_MEM32_SET(instance->status, TD_STATUS_ROUND_FLAG);
	}

	if (buffer != NULL) {
		assert(size != 0);
		OHCI_MEM32_WR(instance->cbp, addr_to_phys(buffer));
		OHCI_MEM32_WR(instance->be, addr_to_phys(buffer + size - 1));
	}

	td_set_next(instance, next);
}

void td_set_next(td_t *instance, const td_t *next)
{
	OHCI_MEM32_WR(instance->next, addr_to_phys(next) & TD_NEXT_PTR_MASK);
}

/**
 * @}
 */

