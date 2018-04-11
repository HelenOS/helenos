/*
 * Copyright (c) 2018 Ondrej Hlavaty, Michal Staruch, Jaroslav Jindrak
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
 * TRB-related structures of the xHC.
 *
 * This file contains all the types of TRB and the TRB ring handling.
 */

#ifndef XHCI_TRB_H
#define XHCI_TRB_H

#include "common.h"
#include <libarch/barrier.h>

/**
 * TRB types: section 6.4.6, table 139
 */
enum xhci_trb_type {
	XHCI_TRB_TYPE_RESERVED = 0,

// Transfer ring:
	XHCI_TRB_TYPE_NORMAL,
	XHCI_TRB_TYPE_SETUP_STAGE,
	XHCI_TRB_TYPE_DATA_STAGE,
	XHCI_TRB_TYPE_STATUS_STAGE,
	XHCI_TRB_TYPE_ISOCH,
	XHCI_TRB_TYPE_LINK,
	XHCI_TRB_TYPE_EVENT_DATA,
	XHCI_TRB_TYPE_NO_OP,

// Command ring:
	XHCI_TRB_TYPE_ENABLE_SLOT_CMD,
	XHCI_TRB_TYPE_DISABLE_SLOT_CMD,
	XHCI_TRB_TYPE_ADDRESS_DEVICE_CMD,
	XHCI_TRB_TYPE_CONFIGURE_ENDPOINT_CMD,
	XHCI_TRB_TYPE_EVALUATE_CONTEXT_CMD,
	XHCI_TRB_TYPE_RESET_ENDPOINT_CMD,
	XHCI_TRB_TYPE_STOP_ENDPOINT_CMD,
	XHCI_TRB_TYPE_SET_TR_DEQUEUE_POINTER_CMD,
	XHCI_TRB_TYPE_RESET_DEVICE_CMD,
	XHCI_TRB_TYPE_FORCE_EVENT_CMD,
	XHCI_TRB_TYPE_NEGOTIATE_BANDWIDTH_CMD,
	XHCI_TRB_TYPE_SET_LATENCY_TOLERANCE_VALUE_CMD,
	XHCI_TRB_TYPE_GET_PORT_BANDWIDTH_CMD,
	XHCI_TRB_TYPE_FORCE_HEADER_CMD,
	XHCI_TRB_TYPE_NO_OP_CMD,
// Reserved: 24-31

// Event ring:
	XHCI_TRB_TYPE_TRANSFER_EVENT = 32,
	XHCI_TRB_TYPE_COMMAND_COMPLETION_EVENT,
	XHCI_TRB_TYPE_PORT_STATUS_CHANGE_EVENT,
	XHCI_TRB_TYPE_BANDWIDTH_REQUEST_EVENT,
	XHCI_TRB_TYPE_DOORBELL_EVENT,
	XHCI_TRB_TYPE_HOST_CONTROLLER_EVENT,
	XHCI_TRB_TYPE_DEVICE_NOTIFICATION_EVENT,
	XHCI_TRB_TYPE_MFINDEX_WRAP_EVENT,

	XHCI_TRB_TYPE_MAX
};

/**
 * TRB template: section 4.11.1
 */
typedef struct xhci_trb {
	xhci_qword_t parameter;
	xhci_dword_t status;
	xhci_dword_t control;
} __attribute__((packed)) __attribute__((aligned(16))) xhci_trb_t;

#define TRB_TYPE(trb)           XHCI_DWORD_EXTRACT((trb).control, 15, 10)
#define TRB_CYCLE(trb)          XHCI_DWORD_EXTRACT((trb).control, 0, 0)
#define TRB_LINK_TC(trb)        XHCI_DWORD_EXTRACT((trb).control, 1, 1)
#define TRB_IOC(trb)            XHCI_DWORD_EXTRACT((trb).control, 5, 5)
#define TRB_EVENT_DATA(trb)		XHCI_DWORD_EXTRACT((trb).control, 2, 2)

#define TRB_TRANSFER_LENGTH(trb)	XHCI_DWORD_EXTRACT((trb).status, 23, 0)
#define TRB_COMPLETION_CODE(trb)	XHCI_DWORD_EXTRACT((trb).status, 31, 24)

#define TRB_LINK_SET_TC(trb, val) \
	xhci_dword_set_bits(&(trb).control, val, 1, 1)
#define TRB_SET_CYCLE(trb, val) \
	xhci_dword_set_bits(&(trb).control, val, 0, 0)

#define TRB_CTRL_SET_SETUP_WLENGTH(trb, val) \
	xhci_qword_set_bits(&(trb).parameter, val, 63, 48)
#define TRB_CTRL_SET_SETUP_WINDEX(trb, val) \
	xhci_qword_set_bits(&(trb).parameter, val, 47, 32)
#define TRB_CTRL_SET_SETUP_WVALUE(trb, val) \
	xhci_qword_set_bits(&(trb).parameter, val, 31, 16)
#define TRB_CTRL_SET_SETUP_BREQ(trb, val) \
	xhci_qword_set_bits(&(trb).parameter, val, 15, 8)
#define TRB_CTRL_SET_SETUP_BMREQTYPE(trb, val) \
	xhci_qword_set_bits(&(trb).parameter, val, 7, 0)

#define TRB_CTRL_SET_TD_SIZE(trb, val) \
	xhci_dword_set_bits(&(trb).status, val, 21, 17)
#define TRB_CTRL_SET_XFER_LEN(trb, val) \
	xhci_dword_set_bits(&(trb).status, val, 16, 0)

#define TRB_CTRL_SET_ENT(trb, val) \
	xhci_dword_set_bits(&(trb).control, val, 1, 1)
#define TRB_CTRL_SET_ISP(trb, val) \
	xhci_dword_set_bits(&(trb).control, val, 2, 2)
#define TRB_CTRL_SET_NS(trb, val) \
	xhci_dword_set_bits(&(trb).control, val, 3, 3)
#define TRB_CTRL_SET_CHAIN(trb, val) \
	xhci_dword_set_bits(&(trb).control, val, 4, 4)
#define TRB_CTRL_SET_IOC(trb, val) \
	xhci_dword_set_bits(&(trb).control, val, 5, 5)
#define TRB_CTRL_SET_IDT(trb, val) \
	xhci_dword_set_bits(&(trb).control, val, 6, 6)

#define TRB_CTRL_SET_TRB_TYPE(trb, val) \
	xhci_dword_set_bits(&(trb).control, val, 15, 10)
#define TRB_CTRL_SET_DIR(trb, val) \
	xhci_dword_set_bits(&(trb).control, val, 16, 16)
#define TRB_CTRL_SET_TRT(trb, val) \
	xhci_dword_set_bits(&(trb).control, val, 17, 16)

#define TRB_ISOCH_SET_TBC(trb, val) \
	xhci_dword_set_bits(&(trb).control, val, 8, 7)
#define TRB_ISOCH_SET_TLBPC(trb, val) \
	xhci_dword_set_bits(&(trb).control, val, 19, 16)
#define TRB_ISOCH_SET_FRAMEID(trb, val) \
	xhci_dword_set_bits(&(trb).control, val, 30, 20)
#define TRB_ISOCH_SET_SIA(trb, val) \
	xhci_dword_set_bits(&(trb).control, val, 31, 31)

/**
 * The Chain bit is valid only in specific TRB types.
 */
static inline bool xhci_trb_is_chained(xhci_trb_t *trb)
{
	const int type = TRB_TYPE(*trb);
	const bool chain_bit = XHCI_DWORD_EXTRACT(trb->control, 4, 4);

	return chain_bit &&
	    (type == XHCI_TRB_TYPE_NORMAL ||
	    type == XHCI_TRB_TYPE_DATA_STAGE ||
	    type == XHCI_TRB_TYPE_STATUS_STAGE ||
	    type == XHCI_TRB_TYPE_ISOCH);
}

static inline void xhci_trb_link_fill(xhci_trb_t *trb, uintptr_t next_phys)
{
	// TRBs require 16-byte alignment
	assert((next_phys & 0xf) == 0);

	xhci_dword_set_bits(&trb->control, XHCI_TRB_TYPE_LINK, 15, 10);
	xhci_qword_set(&trb->parameter, next_phys);
}

static inline void xhci_trb_copy_to_pio(xhci_trb_t *dst, xhci_trb_t *src)
{
	/*
	 * As we do not know, whether our architecture is capable of copying 16
	 * bytes atomically, let's copy the fields one by one.
	 */
	dst->parameter = src->parameter;
	dst->status = src->status;

	write_barrier();

	dst->control = src->control;
}

static inline void xhci_trb_clean(xhci_trb_t *trb)
{
	memset(trb, 0, sizeof(*trb));
}

/**
 * Event Ring Segment Table: section 6.5
 */
typedef struct xhci_erst_entry {
	xhci_qword_t rs_base_ptr;       /* 64B aligned */
	xhci_dword_t size;              /* only low 16 bits, the rest is RsvdZ */
	xhci_dword_t _reserved;
} xhci_erst_entry_t;

static inline void xhci_fill_erst_entry(xhci_erst_entry_t *entry,
    uintptr_t phys, int segments)
{
	xhci_qword_set(&entry->rs_base_ptr, phys);
	xhci_dword_set_bits(&entry->size, segments, 16, 0);
}

typedef enum xhci_trb_completion_code {
	XHCI_TRBC_INVALID = 0,
	XHCI_TRBC_SUCCESS,
	XHCI_TRBC_DATA_BUFFER_ERROR,
	XHCI_TRBC_BABBLE_DETECTED_ERROR,
	XHCI_TRBC_USB_TRANSACTION_ERROR,
	XHCI_TRBC_TRB_ERROR,
	XHCI_TRBC_STALL_ERROR,
	XHCI_TRBC_RESOURCE_ERROR,
	XHCI_TRBC_BANDWIDTH_ERROR,
	XHCI_TRBC_NO_SLOTS_ERROR,
	XHCI_TRBC_INVALID_STREAM_ERROR,
	XHCI_TRBC_SLOT_NOT_ENABLED_ERROR,
	XHCI_TRBC_EP_NOT_ENABLED_ERROR,
	XHCI_TRBC_SHORT_PACKET,
	XHCI_TRBC_RING_UNDERRUN,
	XHCI_TRBC_RING_OVERRUN,
	XHCI_TRBC_VF_EVENT_RING_FULL,
	XHCI_TRBC_PARAMETER_ERROR,
	XHCI_TRBC_BANDWIDTH_OVERRUN_ERROR,
	XHCI_TRBC_CONTEXT_STATE_ERROR,
	XHCI_TRBC_NO_PING_RESPONSE_ERROR,
	XHCI_TRBC_EVENT_RING_FULL_ERROR,
	XHCI_TRBC_INCOMPATIBLE_DEVICE_ERROR,
	XHCI_TRBC_MISSED_SERVICE_ERROR,
	XHCI_TRBC_COMMAND_RING_STOPPED,
	XHCI_TRBC_COMMAND_ABORTED,
	XHCI_TRBC_STOPPED,
	XHCI_TRBC_STOPPED_LENGTH_INVALID,
	XHCI_TRBC_STOPPED_SHORT_PACKET,
	XHCI_TRBC_MAX_EXIT_LATENCY_TOO_LARGE_ERROR,
	/* 30 reserved */
	XHCI_TRBC_ISOCH_BUFFER_OVERRUN = 31,
	XHCI_TRBC_EVENT_LOST_ERROR,
	XHCI_TRBC_UNDEFINED_ERROR,
	XHCI_TRBC_INVALID_STREAM_ID_ERROR,
	XHCI_TRBC_SECONDARY_BANDWIDTH_ERROR,
	XHCI_TRBC_SPLIT_TRANSACTION_ERROR,
	XHCI_TRBC_MAX
	/**
	 *  37 - 191 reserved
	 * 192 - 223 vendor defined error
	 * 224 - 255 vendor defined info
	 */
} xhci_trb_completion_code_t;

#endif
