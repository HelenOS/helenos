/*
 * SPDX-FileCopyrightText: 2014 Jan Vesely
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
#include <errno.h>
#include <macros.h>
#include <mem.h>

#include <usb/usb.h>

#include "mem_access.h"
#include "transfer_descriptor.h"

errno_t td_error(const td_t *td)
{
	assert(td);
	const uint32_t status = EHCI_MEM32_RD(td->status);
	if (status & TD_STATUS_HALTED_FLAG) {
		if (status & TD_STATUS_TRANS_ERR_FLAG)
			return EIO;
		if (status & TD_STATUS_BABBLE_FLAG)
			return EIO;
		if (status & TD_STATUS_BUFF_ERROR_FLAG)
			return EOVERFLOW;
		return ESTALL;
	}
	if (status & TD_STATUS_ACTIVE_FLAG)
		return EBUSY;
	return EOK;
}

/** USB direction to EHCI TD values translation table */
static const uint32_t dir[] = {
	[USB_DIRECTION_IN] = TD_STATUS_PID_IN,
	[USB_DIRECTION_OUT] = TD_STATUS_PID_OUT,
	[USB_DIRECTION_BOTH] = TD_STATUS_PID_SETUP,
};

#include <usb/debug.h>

/**
 * Initialize EHCI TD.
 * @param instance TD structure to initialize.
 * @param next_phys Next TD in ED list.
 * @param direction Used to determine PID, BOTH means setup PID.
 * @param buffer Pointer to the first byte of transferred data.
 * @param size Size of the buffer.
 * @param toggle Toggle bit value, use 0/1 to set explicitly,
 *        any other value means that ED toggle will be used.
 */
void td_init(td_t *instance, uintptr_t next_phys, uintptr_t buffer,
    usb_direction_t direction, size_t size, int toggle, bool ioc)
{
	assert(instance);
	memset(instance, 0, sizeof(td_t));
	/* Set PID and Total size */
	assert((size & TD_STATUS_TOTAL_MASK) == size);
	EHCI_MEM32_WR(instance->status,
	    ((dir[direction] & TD_STATUS_PID_MASK) << TD_STATUS_PID_SHIFT) |
	    ((size & TD_STATUS_TOTAL_MASK) << TD_STATUS_TOTAL_SHIFT) |
	    (ioc ? TD_STATUS_IOC_FLAG : 0));

	if (toggle == 0 || toggle == 1) {
		EHCI_MEM32_SET(instance->status,
		    toggle ? TD_STATUS_TOGGLE_FLAG : 0);
	}

	if (buffer != 0) {
		assert(size != 0);
		for (unsigned i = 0; (i < ARRAY_SIZE(instance->buffer_pointer)) &&
		    size; ++i) {
			const uintptr_t offset = buffer & TD_BUFFER_POINTER_OFFSET_MASK;
			assert(offset == 0 || i == 0);
			const size_t this_size = min(size, 4096 - offset);
			EHCI_MEM32_WR(instance->buffer_pointer[i], buffer);
			size -= this_size;
			buffer += this_size;
		}
	}

	EHCI_MEM32_WR(instance->next, next_phys ?
	    LINK_POINTER_TD(next_phys) : LINK_POINTER_TERM);

	EHCI_MEM32_WR(instance->alternate, LINK_POINTER_TERM);
	EHCI_MEM32_SET(instance->status, TD_STATUS_ACTIVE_FLAG);
	write_barrier();
}
/**
 * @}
 */
