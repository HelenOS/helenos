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
 * @brief Command sending functions.
 */

#include <errno.h>
#include <str_error.h>
#include <usb/debug.h>
#include <usb/host/utils/malloc32.h>
#include "commands.h"
#include "debug.h"
#include "hc.h"
#include "hw_struct/context.h"
#include "hw_struct/trb.h"

static inline int ring_doorbell(xhci_hc_t *hc, unsigned doorbell, unsigned target)
{
	assert(hc);
	uint32_t v = host2xhci(32, target & BIT_RRANGE(uint32_t, 7));
	pio_write_32(&hc->db_arry[doorbell], v);
	return EOK;
}

static inline int enqueue_trb(xhci_hc_t *hc, xhci_trb_t *trb,
			      unsigned doorbell, unsigned target)
{
	assert(hc);
	assert(trb);

	xhci_trb_ring_enqueue(&hc->command_ring, trb);
	ring_doorbell(hc, doorbell, target);

	xhci_dump_trb(trb);
	usb_log_debug2("HC(%p): Sent TRB", hc);

	return EOK;
}

static const char *trb_codes [] = {
#define TRBC(t) [XHCI_TRBC_##t] = #t
	TRBC(INVALID),
	TRBC(SUCCESS),
	TRBC(DATA_BUFFER_ERROR),
	TRBC(BABBLE_DETECTED_ERROR),
	TRBC(USB_TRANSACTION_ERROR),
	TRBC(TRB_ERROR),
	TRBC(STALL_ERROR),
	TRBC(RESOURCE_ERROR),
	TRBC(BANDWIDTH_ERROR),
	TRBC(NO_SLOTS_ERROR),
	TRBC(INVALID_STREAM_ERROR),
	TRBC(SLOT_NOT_ENABLED_ERROR),
	TRBC(EP_NOT_ENABLED_ERROR),
	TRBC(SHORT_PACKET),
	TRBC(RING_UNDERRUN),
	TRBC(RING_OVERRUN),
	TRBC(VF_EVENT_RING_FULL),
	TRBC(PARAMETER_ERROR),
	TRBC(BANDWIDTH_OVERRUN_ERROR),
	TRBC(CONTEXT_STATE_ERROR),
	TRBC(NO_PING_RESPONSE_ERROR),
	TRBC(EVENT_RING_FULL_ERROR),
	TRBC(INCOMPATIBLE_DEVICE_ERROR),
	TRBC(MISSED_SERVICE_ERROR),
	TRBC(COMMAND_RING_STOPPED),
	TRBC(COMMAND_ABORTED),
	TRBC(STOPPED),
	TRBC(STOPPED_LENGTH_INVALID),
	TRBC(STOPPED_SHORT_PACKET),
	TRBC(MAX_EXIT_LATENCY_TOO_LARGE_ERROR),
	[30] = "<reserved>",
	TRBC(ISOCH_BUFFER_OVERRUN),
	TRBC(EVENT_LOST_ERROR),
	TRBC(UNDEFINED_ERROR),
	TRBC(INVALID_STREAM_ID_ERROR),
	TRBC(SECONDARY_BANDWIDTH_ERROR),
	TRBC(SPLIT_TRANSACTION_ERROR),
	[XHCI_TRBC_MAX] = NULL
#undef TRBC
};

static void report_error(int code)
{
	if (code < XHCI_TRBC_MAX && trb_codes[code] != NULL)
		usb_log_error("Command resulted in error: %s.", trb_codes[code]);
	else
		usb_log_error("Command resulted in reserved or vendor specific error.");
}

int xhci_send_no_op_command(xhci_hc_t *hc)
{
	assert(hc);

	xhci_trb_t trb;
	memset(&trb, 0, sizeof(trb));

	trb.control = host2xhci(32, XHCI_TRB_TYPE_NO_OP_CMD << 10);

	return enqueue_trb(hc, &trb, 0, 0);
}

int xhci_send_enable_slot_command(xhci_hc_t *hc)
{
	assert(hc);

	xhci_trb_t trb;
	memset(&trb, 0, sizeof(trb));

	trb.control = host2xhci(32, XHCI_TRB_TYPE_ENABLE_SLOT_CMD << 10);
	trb.control |= host2xhci(32, XHCI_REG_RD(hc->xecp, XHCI_EC_SP_SLOT_TYPE) << 16);
	trb.control |= host2xhci(32, hc->command_ring.pcs);

	return enqueue_trb(hc, &trb, 0, 0);
}

int xhci_send_disable_slot_command(xhci_hc_t *hc, uint32_t slot_id)
{
	assert(hc);

	xhci_trb_t trb;
	memset(&trb, 0, sizeof(trb));

	trb.control = host2xhci(32, XHCI_TRB_TYPE_DISABLE_SLOT_CMD << 10);
	trb.control |= host2xhci(32, hc->command_ring.pcs);
	trb.control |= host2xhci(32, slot_id << 24);

	return enqueue_trb(hc, &trb, 0, 0);
}

int xhci_send_address_device_command(xhci_hc_t *hc, uint32_t slot_id,
				     xhci_input_ctx_t *ictx)
{
	assert(hc);
	assert(ictx);

	/**
	 * TODO: Requirements for this command:
	 *           dcbaa[slot_id] is properly sized and initialized
	 *           ictx has valids slot context and endpoint 0, all
	 *           other should be ignored at this point (see section 4.6.5).
	 */
	xhci_trb_t trb;
	memset(&trb, 0, sizeof(trb));

	uint64_t phys_addr = (uint64_t) addr_to_phys(ictx);
	trb.parameter = host2xhci(32, phys_addr & (~0xF));

	/**
	 * Note: According to section 6.4.3.4, we can set the 9th bit
	 *       of the control field of the trb (BSR) to 1 and then the xHC
	 *       will not issue the SET_ADDRESS request to the USB device.
	 *       This can be used to provide compatibility with legacy USB devices
	 *       that require their device descriptor to be read before such request.
	 */
	trb.control = host2xhci(32, XHCI_TRB_TYPE_ADDRESS_DEVICE_CMD << 10);
	trb.control |= host2xhci(32, hc->command_ring.pcs);
	trb.control |= host2xhci(32, slot_id << 24);

	return enqueue_trb(hc, &trb, 0, 0);
}

int xhci_send_configure_endpoint_command(xhci_hc_t *hc, uint32_t slot_id,
					 xhci_input_ctx_t *ictx)
{
	assert(hc);
	assert(ictx);

	xhci_trb_t trb;
	memset(&trb, 0, sizeof(trb));

	uint64_t phys_addr = (uint64_t) addr_to_phys(ictx);
	trb.parameter = host2xhci(32, phys_addr & (~0xF));

	trb.control = host2xhci(32, XHCI_TRB_TYPE_CONFIGURE_ENDPOINT_CMD << 10);
	trb.control |= host2xhci(32, hc->command_ring.pcs);
	trb.control |= host2xhci(32, slot_id << 24);

	return enqueue_trb(hc, &trb, 0, 0);
}

int xhci_send_evaluate_context_command(xhci_hc_t *hc, uint32_t slot_id,
				       xhci_input_ctx_t *ictx)
{
	assert(hc);
	assert(ictx);

	/**
	 * Note: All Drop Context flags of the input context shall be 0,
	 *       all Add Context flags shall be initialize to indicate IDs
	 *       of the contexts affected by the command.
	 *       Refer to sections 6.2.2.3 and 6.3.3.3 for further info.
	 */
	xhci_trb_t trb;
	memset(&trb, 0, sizeof(trb));

	uint64_t phys_addr = (uint64_t) addr_to_phys(ictx);
	trb.parameter = host2xhci(32, phys_addr & (~0xF));

	trb.control = host2xhci(32, XHCI_TRB_TYPE_EVALUATE_CONTEXT_CMD << 10);
	trb.control |= host2xhci(32, hc->command_ring.pcs);
	trb.control |= host2xhci(32, slot_id << 24);

	return enqueue_trb(hc, &trb, 0, 0);
}

int xhci_send_reset_endpoint_command(xhci_hc_t *hc, uint32_t slot_id, uint32_t ep_id, uint8_t tcs)
{
	assert(hc);

	/**
	 * Note: TCS can have values 0 or 1. If it is set to 0, see sectuon 4.5.8 for
	 *       information about this flag.
	 */
	xhci_trb_t trb;
	memset(&trb, 0, sizeof(trb));

	trb.control = host2xhci(32, XHCI_TRB_TYPE_RESET_ENDPOINT_CMD << 10);
	trb.control |= host2xhci(32, hc->command_ring.pcs);
	trb.control |= host2xhci(32, (tcs & 0x1) << 9);
	trb.control |= host2xhci(32, (ep_id & 0x5) << 16);
	trb.control |= host2xhci(32, slot_id << 24);

	return enqueue_trb(hc, &trb, 0, 0);
}

int xhci_send_stop_endpoint_command(xhci_hc_t *hc, uint32_t slot_id, uint32_t ep_id, uint8_t susp)
{
	assert(hc);

	xhci_trb_t trb;
	memset(&trb, 0, sizeof(trb));

	trb.control = host2xhci(32, XHCI_TRB_TYPE_STOP_ENDPOINT_CMD << 10);
	trb.control |= host2xhci(32, hc->command_ring.pcs);
	trb.control |= host2xhci(32, (ep_id & 0x5) << 16);
	trb.control |= host2xhci(32, (susp & 0x1) << 23);
	trb.control |= host2xhci(32, slot_id << 24);

	return enqueue_trb(hc, &trb, 0, 0);
}

int xhci_send_reset_device_command(xhci_hc_t *hc, uint32_t slot_id)
{
	assert(hc);

	xhci_trb_t trb;
	memset(&trb, 0, sizeof(trb));

	trb.control = host2xhci(32, XHCI_TRB_TYPE_RESET_DEVICE_CMD << 10);
	trb.control |= host2xhci(32, hc->command_ring.pcs);
	trb.control |= host2xhci(32, slot_id << 24);

	return enqueue_trb(hc, &trb, 0, 0);
}

int xhci_handle_command_completion(xhci_hc_t *hc, xhci_trb_t *trb)
{
	assert(hc);
	assert(trb);

	usb_log_debug("HC(%p) Command completed.", hc);
	xhci_dump_trb(trb);

	int code;
	uint32_t slot_id;
	xhci_trb_t *command;

	code = XHCI_DWORD_EXTRACT(trb->status, 31, 24);
	command = (xhci_trb_t *) XHCI_QWORD_EXTRACT(trb->parameter, 63, 4);
	slot_id = XHCI_DWORD_EXTRACT(trb->control, 31, 24);
	(void) slot_id;

	if (TRB_TYPE(*command) != XHCI_TRB_TYPE_NO_OP_CMD) {
		if (code != XHCI_TRBC_SUCCESS) {
			report_error(code);
			xhci_dump_trb(command);
		}
	}

	switch (TRB_TYPE(*command)) {
	case XHCI_TRB_TYPE_NO_OP_CMD:
		assert(code = XHCI_TRBC_TRB_ERROR);
		return EOK;
	case XHCI_TRB_TYPE_ENABLE_SLOT_CMD:
		return EOK;
	case XHCI_TRB_TYPE_DISABLE_SLOT_CMD:
		return EOK;
	case XHCI_TRB_TYPE_ADDRESS_DEVICE_CMD:
		return EOK;
	case XHCI_TRB_TYPE_CONFIGURE_ENDPOINT_CMD:
		return EOK;
	case XHCI_TRB_TYPE_EVALUATE_CONTEXT_CMD:
		return EOK;
	case XHCI_TRB_TYPE_RESET_ENDPOINT_CMD:
		return EOK;
	case XHCI_TRB_TYPE_STOP_ENDPOINT_CMD:
		// Note: If the endpoint was in the middle of a transfer, then the xHC
		//       will add a Transfer TRB before the Event TRB, research that and
		//       handle it appropriately!
		return EOK;
	case XHCI_TRB_TYPE_RESET_DEVICE_CMD:
		return EOK;
	default:
		usb_log_debug2("Unsupported command trb.");
		xhci_dump_trb(command);
		return ENAK;
	}
}


/**
 * @}
 */
