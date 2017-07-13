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
#include "hw_struct/trb.h"

static inline int ring_doorbell(xhci_hc_t *hc, unsigned doorbell, unsigned target)
{
	uint32_t v = host2xhci(32, target & BIT_RRANGE(uint32_t, 7));
	pio_write_32(&hc->db_arry[doorbell], v);
	return EOK;
}

static inline int enqueue_trb(xhci_hc_t *hc, xhci_trb_t *trb,
			      unsigned doorbell, unsigned target)
{
	xhci_trb_ring_enqueue(&hc->command_ring, trb);
	ring_doorbell(hc, doorbell, target);

	xhci_dump_trb(trb);
	usb_log_debug2("HC(%p): Sent TRB", hc);

	return EOK;
}

int xhci_send_no_op_command(xhci_hc_t *hc)
{
	xhci_trb_t trb;
	memset(&trb, 0, sizeof(trb));

	trb.control = host2xhci(32, XHCI_TRB_TYPE_NO_OP_CMD << 10);

	return enqueue_trb(hc, &trb, 0, 0);
}

int xhci_send_enable_slot_command(xhci_hc_t *hc)
{
	xhci_trb_t trb;
	memset(&trb, 0, sizeof(trb));

	trb.control = host2xhci(32, XHCI_TRB_TYPE_ENABLE_SLOT_CMD << 10);
	trb.control |= host2xhci(32, XHCI_REG_RD(hc->xecp, XHCI_EC_SP_SLOT_TYPE) << 16);
	trb.control |= host2xhci(32, hc->command_ring.pcs);

	return enqueue_trb(hc, &trb, 0, 0);
}

int xhci_send_disable_slot_command(xhci_hc_t *hc, uint32_t slot_id)
{
	xhci_trb_t trb;
	memset(&trb, 0, sizeof(trb));

	trb.control = host2xhci(32, XHCI_TRB_TYPE_DISABLE_SLOT_CMD << 10);
	trb.control |= host2xhci(32, hc->command_ring.pcs);
	trb.control |= host2xhci(32, slot_id << 24);

	return enqueue_trb(hc, &trb, 0, 0);
}

int xhci_handle_command_completion(xhci_hc_t *hc, xhci_trb_t *trb)
{
	usb_log_debug("HC(%p) Command completed.", hc);
	xhci_dump_trb(trb);

	int code;
	uint32_t slot_id;
	xhci_trb_t *command;

	code = XHCI_DWORD_EXTRACT(trb->status, 31, 24);
	command = (xhci_trb_t *) XHCI_QWORD_EXTRACT(trb->parameter, 63, 4);
	slot_id = XHCI_DWORD_EXTRACT(trb->control, 31, 24);
	(void) slot_id;

	switch (TRB_TYPE(*command)) {
	case XHCI_TRB_TYPE_NO_OP_CMD:
		assert(code == XHCI_TRBC_TRB_ERROR);
		break;
	case XHCI_TRB_TYPE_ENABLE_SLOT_CMD:
		// TODO: Call a device addition callback once it's implemented.
		break;
	case XHCI_TRB_TYPE_DISABLE_SLOT_CMD:
		if (code == XHCI_TRBC_SLOT_NOT_ENABLED_ERROR)
			usb_log_debug2("Slot ID to be disabled was not enabled.");
		// TODO: Call a device removal callback that will deallocate associated
		//       data structures once it's implemented.
		break;
	default:
		usb_log_debug2("Unsupported command trb.");
		xhci_dump_trb(command);
		break;
	}

	return EOK;
}


/**
 * @}
 */
