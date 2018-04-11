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

#ifndef DRV_UHCI_HW_STRUCT_TRANSFER_DESCRIPTOR_H
#define DRV_UHCI_HW_STRUCT_TRANSFER_DESCRIPTOR_H

#include <assert.h>
#include <usb/usb.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "link_pointer.h"

/** Transfer Descriptor, defined in UHCI design guide p. 26 */
typedef struct transfer_descriptor {
	/** Pointer to the next entity (TD or QH) */
	link_pointer_t next;

	/** Status doubleword */
	volatile uint32_t status;
#define TD_STATUS_RESERVED_MASK 0xc000f800
#define TD_STATUS_SPD_FLAG         (1 << 29)
#define TD_STATUS_ERROR_COUNT_POS 27
#define TD_STATUS_ERROR_COUNT_MASK 0x3
#define TD_STATUS_LOW_SPEED_FLAG   (1 << 26)
#define TD_STATUS_ISOCHRONOUS_FLAG (1 << 25)
#define TD_STATUS_IOC_FLAG         (1 << 24)

#define TD_STATUS_ERROR_ACTIVE    (1 << 23)
#define TD_STATUS_ERROR_STALLED   (1 << 22)
#define TD_STATUS_ERROR_BUFFER    (1 << 21)
#define TD_STATUS_ERROR_BABBLE    (1 << 20)
#define TD_STATUS_ERROR_NAK       (1 << 19)
#define TD_STATUS_ERROR_CRC       (1 << 18)
#define TD_STATUS_ERROR_BIT_STUFF (1 << 17)
#define TD_STATUS_ERROR_RESERVED  (1 << 16)
#define TD_STATUS_ERROR_POS 16
#define TD_STATUS_ERROR_MASK 0xff

#define TD_STATUS_ACTLEN_POS 0
#define TD_STATUS_ACTLEN_MASK 0x7ff

	/** Double word with USB device specific info */
	volatile uint32_t device;
#define TD_DEVICE_MAXLEN_POS 21
#define TD_DEVICE_MAXLEN_MASK 0x7ff
#define TD_DEVICE_RESERVED_FLAG        (1 << 20)
#define TD_DEVICE_DATA_TOGGLE_ONE_FLAG (1 << 19)
#define TD_DEVICE_ENDPOINT_POS 15
#define TD_DEVICE_ENDPOINT_MASK 0xf
#define TD_DEVICE_ADDRESS_POS 8
#define TD_DEVICE_ADDRESS_MASK 0x7f
#define TD_DEVICE_PID_POS 0
#define TD_DEVICE_PID_MASK 0xff

	/** Pointer(physical) to the beginning of the transaction's buffer */
	volatile uint32_t buffer_ptr;

	/* According to UHCI design guide, there is 16 bytes of
	 * data available here.
	 * According to Linux kernel the hardware does not care,
	 * memory just needs to be aligned. We don't use it anyway.
	 */
} __attribute__((packed, aligned(16))) td_t;


void td_init(td_t *instance, int error_count, size_t size, bool toggle,
    bool iso, bool low_speed, usb_target_t target, usb_packet_id pid,
    const void *buffer, const td_t *next);

errno_t td_status(const td_t *instance);

void td_print_status(const td_t *instance);

/** Helper function for parsing actual size out of TD.
 *
 * @param[in] instance TD structure to use.
 * @return Parsed actual size.
 */
static inline size_t td_act_size(const td_t *instance)
{
	assert(instance);
	const uint32_t s = instance->status;
	/* Actual size is encoded as n-1 (UHCI design guide p. 23) */
	return ((s >> TD_STATUS_ACTLEN_POS) + 1) & TD_STATUS_ACTLEN_MASK;
}

/** Check whether less than max data were received on SPD marked transfer.
 *
 * @param[in] instance TD structure to use.
 * @return True if data packet is short (less than max bytes and SPD set),
 * false otherwise.
 */
static inline bool td_is_short(const td_t *instance)
{
	const size_t act_size = td_act_size(instance);
	const size_t max_size =
	    ((instance->device >> TD_DEVICE_MAXLEN_POS) + 1) &
	    TD_DEVICE_MAXLEN_MASK;
	return
	    (instance->status | TD_STATUS_SPD_FLAG) && act_size < max_size;
}

/** Helper function for parsing value of toggle bit.
 *
 * @param[in] instance TD structure to use.
 * @return Toggle bit value.
 */
static inline int td_toggle(const td_t *instance)
{
	assert(instance);
	return (instance->device & TD_DEVICE_DATA_TOGGLE_ONE_FLAG) ? 1 : 0;
}

/** Helper function for parsing value of active bit
 *
 * @param[in] instance TD structure to use.
 * @return Active bit value.
 */
static inline bool td_is_active(const td_t *instance)
{
	assert(instance);
	return (instance->status & TD_STATUS_ERROR_ACTIVE) != 0;
}

/** Helper function for setting IOC bit.
 *
 * @param[in] instance TD structure to use.
 */
static inline void td_set_ioc(td_t *instance)
{
	assert(instance);
	instance->status |= TD_STATUS_IOC_FLAG;
}

#endif

/**
 * @}
 */
