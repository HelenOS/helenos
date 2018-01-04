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

/** @addtogroup drvusbuhcihc
 * @{
 */
/** @file
 * @brief UHCI driver
 */

#include <assert.h>
#include <errno.h>

#include <usb/debug.h>
#include <usb/usb.h>
#include <usb/host/utils/malloc32.h>

#include "link_pointer.h"
#include "transfer_descriptor.h"

/** Initialize Transfer Descriptor
 *
 * @param[in] instance Memory place to initialize.
 * @param[in] err_count Number of retries hc should attempt.
 * @param[in] size Size of data source.
 * @param[in] toggle Value of toggle bit.
 * @param[in] iso True if TD represents Isochronous transfer.
 * @param[in] low_speed Target device's speed.
 * @param[in] target Address and endpoint receiving the transfer.
 * @param[in] pid Packet identification (SETUP, IN or OUT).
 * @param[in] buffer Source of data.
 * @param[in] next Net TD in transaction.
 * @return Error code.
 *
 * Uses a mix of supplied and default values.
 * Implicit values:
 *  - all TDs have vertical flag set (makes transfers to endpoints atomic)
 *  - in the error field only active it is set
 *  - if the packet uses PID_IN and is not isochronous SPD is set
 *
 * Dumps 8 bytes of buffer if PID_SETUP is used.
 */
void td_init(td_t *instance, int err_count, size_t size, bool toggle, bool iso,
    bool low_speed, usb_target_t target, usb_packet_id pid, const void *buffer,
    const td_t *next)
{
	assert(instance);
	assert(size < 1024);
	assert((pid == USB_PID_SETUP) || (pid == USB_PID_IN)
	    || (pid == USB_PID_OUT));

	const uint32_t next_pa = addr_to_phys(next);
	assert((next_pa & LINK_POINTER_ADDRESS_MASK) == next_pa);

	instance->next = 0
	    | LINK_POINTER_VERTICAL_FLAG
	    | (next_pa ? next_pa : LINK_POINTER_TERMINATE_FLAG);

	instance->status = 0
	    | ((err_count & TD_STATUS_ERROR_COUNT_MASK)
	        << TD_STATUS_ERROR_COUNT_POS)
	    | (low_speed ? TD_STATUS_LOW_SPEED_FLAG : 0)
	    | (iso ? TD_STATUS_ISOCHRONOUS_FLAG : 0)
	    | TD_STATUS_ERROR_ACTIVE;

	if (pid == USB_PID_IN && !iso) {
		instance->status |= TD_STATUS_SPD_FLAG;
	}

	instance->device = 0
	    | (((size - 1) & TD_DEVICE_MAXLEN_MASK) << TD_DEVICE_MAXLEN_POS)
	    | (toggle ? TD_DEVICE_DATA_TOGGLE_ONE_FLAG : 0)
	    | ((target.address & TD_DEVICE_ADDRESS_MASK)
	        << TD_DEVICE_ADDRESS_POS)
	    | ((target.endpoint & TD_DEVICE_ENDPOINT_MASK)
	        << TD_DEVICE_ENDPOINT_POS)
	    | ((pid & TD_DEVICE_PID_MASK) << TD_DEVICE_PID_POS);

	instance->buffer_ptr = addr_to_phys(buffer);

	usb_log_debug2("Created TD(%p): %X:%X:%X:%X(%p).\n",
	    instance, instance->next, instance->status, instance->device,
	    instance->buffer_ptr, buffer);
	td_print_status(instance);
	if (pid == USB_PID_SETUP) {
		usb_log_debug2("SETUP BUFFER: %s\n",
		    usb_debug_str_buffer(buffer, 8, 8));
	}
}

/** Convert TD status into standard error code
 *
 * @param[in] instance TD structure to use.
 * @return Error code.
 */
errno_t td_status(const td_t *instance)
{
	assert(instance);

	/* This is hc internal error it should never be reported. */
	if ((instance->status & TD_STATUS_ERROR_BIT_STUFF) != 0)
		return EIO;

	/* CRC or timeout error, like device not present or bad data,
	 * it won't be reported unless err count reached zero */
	if ((instance->status & TD_STATUS_ERROR_CRC) != 0)
		return EBADCHECKSUM;

	/* HC does not end transactions on these, it should never be reported */
	if ((instance->status & TD_STATUS_ERROR_NAK) != 0)
		return EAGAIN;

	/* Buffer overrun or underrun */
	if ((instance->status & TD_STATUS_ERROR_BUFFER) != 0)
		return ERANGE;

	/* Device babble is something serious */
	if ((instance->status & TD_STATUS_ERROR_BABBLE) != 0)
		return EIO;

	/* Stall might represent err count reaching zero or stall response from
	 * the device. If err count reached zero, one of the above is reported*/
	if ((instance->status & TD_STATUS_ERROR_STALLED) != 0)
		return ESTALL;

	return EOK;
}

/** Print values in status field (dw1) in a human readable way.
 *
 * @param[in] instance TD structure to use.
 */
void td_print_status(const td_t *instance)
{
	assert(instance);
	const uint32_t s = instance->status;
	usb_log_debug2("TD(%p) status(%#" PRIx32 "):%s %d,%s%s%s%s%s%s%s%s%s%s%s %zu.\n",
	    instance, instance->status,
	    (s & TD_STATUS_SPD_FLAG) ? " SPD," : "",
	    (s >> TD_STATUS_ERROR_COUNT_POS) & TD_STATUS_ERROR_COUNT_MASK,
	    (s & TD_STATUS_LOW_SPEED_FLAG) ? " LOW SPEED," : "",
	    (s & TD_STATUS_ISOCHRONOUS_FLAG) ? " ISOCHRONOUS," : "",
	    (s & TD_STATUS_IOC_FLAG) ? " IOC," : "",
	    (s & TD_STATUS_ERROR_ACTIVE) ? " ACTIVE," : "",
	    (s & TD_STATUS_ERROR_STALLED) ? " STALLED," : "",
	    (s & TD_STATUS_ERROR_BUFFER) ? " BUFFER," : "",
	    (s & TD_STATUS_ERROR_BABBLE) ? " BABBLE," : "",
	    (s & TD_STATUS_ERROR_NAK) ? " NAK," : "",
	    (s & TD_STATUS_ERROR_CRC) ? " CRC/TIMEOUT," : "",
	    (s & TD_STATUS_ERROR_BIT_STUFF) ? " BIT_STUFF," : "",
	    (s & TD_STATUS_ERROR_RESERVED) ? " RESERVED," : "",
	    td_act_size(instance)
	);
}
/**
 * @}
 */
