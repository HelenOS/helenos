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
/** @addtogroup usb
 * @{
 */
/** @file
 * @brief UHCI driver
 */
#include <errno.h>
#include <usb/debug.h>

#include "transfer_descriptor.h"
#include "utils/malloc32.h"

void transfer_descriptor_init(transfer_descriptor_t *instance,
    int error_count, size_t size, bool toggle, bool isochronous, bool low_speed,
    usb_target_t target, int pid, void *buffer, transfer_descriptor_t *next)
{
	assert(instance);

	instance->next = 0
	    | LINK_POINTER_VERTICAL_FLAG
	    | ((next != NULL) ? addr_to_phys(next) : LINK_POINTER_TERMINATE_FLAG);

	instance->status = 0
	  | ((error_count & TD_STATUS_ERROR_COUNT_MASK) << TD_STATUS_ERROR_COUNT_POS)
		| (low_speed ? TD_STATUS_LOW_SPEED_FLAG : 0)
	  | TD_STATUS_ERROR_ACTIVE;

	assert(size < 1024);
	instance->device = 0
		| (((size - 1) & TD_DEVICE_MAXLEN_MASK) << TD_DEVICE_MAXLEN_POS)
		| (toggle ? TD_DEVICE_DATA_TOGGLE_ONE_FLAG : 0)
		| ((target.address & TD_DEVICE_ADDRESS_MASK) << TD_DEVICE_ADDRESS_POS)
		| ((target.endpoint & TD_DEVICE_ENDPOINT_MASK) << TD_DEVICE_ENDPOINT_POS)
		| ((pid & TD_DEVICE_PID_MASK) << TD_DEVICE_PID_POS);

	instance->buffer_ptr = 0;

	if (size) {
		instance->buffer_ptr = (uintptr_t)addr_to_phys(buffer);
	}

	usb_log_info("Created TD: %X:%X:%X:%X(%p).\n",
		instance->next, instance->status, instance->device,
	  instance->buffer_ptr, buffer);
}
/*----------------------------------------------------------------------------*/
int transfer_descriptor_status(transfer_descriptor_t *instance)
{
	assert(instance);

	if ((instance->status & TD_STATUS_ERROR_STALLED) != 0)
		return ESTALL;

	if ((instance->status & TD_STATUS_ERROR_CRC) != 0)
		return EBADCHECKSUM;

	if ((instance->status & TD_STATUS_ERROR_BUFFER) != 0)
		return EAGAIN;

	if ((instance->status & TD_STATUS_ERROR_BABBLE) != 0)
		return EIO;

	if ((instance->status & TD_STATUS_ERROR_NAK) != 0)
		return EAGAIN;

	if ((instance->status & TD_STATUS_ERROR_BIT_STUFF) != 0)
		return EAGAIN;

	return EOK;
}
/**
 * @}
 */
