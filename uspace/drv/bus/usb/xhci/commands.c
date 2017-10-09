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

#define TRB_SET_TCS(trb, tcs)   (trb).control |= host2xhci(32, ((tcs &0x1) << 9))
#define TRB_SET_TYPE(trb, type) (trb).control |= host2xhci(32, (type) << 10)
#define TRB_SET_EP(trb, ep)     (trb).control |= host2xhci(32, ((ep) & 0x5) << 16)
#define TRB_SET_STREAM(trb, st) (trb).control |= host2xhci(32, ((st) & 0xFFFF) << 16)
#define TRB_SET_SUSP(trb, susp) (trb).control |= host2xhci(32, ((susp) & 0x1) << 23)
#define TRB_SET_SLOT(trb, slot) (trb).control |= host2xhci(32, (slot) << 24)

/**
 * TODO: Not sure about SCT and DCS (see section 6.4.3.9).
 */
#define TRB_SET_DEQUEUE_PTR(trb, dptr) (trb).parameter |= host2xhci(64, (dptr))
#define TRB_SET_ICTX(trb, phys) (trb).parameter |= host2xhci(64, phys_addr & (~0xF))

#define TRB_GET_CODE(trb) XHCI_DWORD_EXTRACT((trb).status, 31, 24)
#define TRB_GET_SLOT(trb) XHCI_DWORD_EXTRACT((trb).control, 31, 24)
#define TRB_GET_PHYS(trb) (XHCI_QWORD_EXTRACT((trb).parameter, 63, 4) << 4)

int xhci_init_commands(xhci_hc_t *hc)
{
	assert(hc);

	list_initialize(&hc->commands);
	return EOK;
}

void xhci_fini_commands(xhci_hc_t *hc)
{
	// Note: Untested.
	assert(hc);
}

int xhci_cmd_wait(xhci_cmd_t *cmd, suseconds_t timeout)
{
	int rv = EOK;

	fibril_mutex_lock(&cmd->completed_mtx);
	while (!cmd->completed) {
		usb_log_debug2("Waiting for event completion: going to sleep.");
		rv = fibril_condvar_wait_timeout(&cmd->completed_cv, &cmd->completed_mtx, timeout);

		usb_log_debug2("Waiting for event completion: woken: %s", str_error(rv));
		if (rv == ETIMEOUT)
			break;
	}
	fibril_mutex_unlock(&cmd->completed_mtx);

	return rv;
}

xhci_cmd_t *xhci_cmd_alloc(void)
{
	xhci_cmd_t *cmd = malloc(sizeof(xhci_cmd_t));
	xhci_cmd_init(cmd);

	usb_log_debug2("Allocating cmd on the heap. Don't forget to deallocate it!");
	return cmd;
}

void xhci_cmd_init(xhci_cmd_t *cmd)
{
	memset(cmd, 0, sizeof(*cmd));

	link_initialize(&cmd->link);

	fibril_mutex_initialize(&cmd->completed_mtx);
	fibril_condvar_initialize(&cmd->completed_cv);
}

void xhci_cmd_fini(xhci_cmd_t *cmd)
{
	list_remove(&cmd->link);
}

void xhci_cmd_free(xhci_cmd_t *cmd)
{
	xhci_cmd_fini(cmd);
	free(cmd);
}

static inline xhci_cmd_t *get_command(xhci_hc_t *hc, uint64_t phys)
{
	link_t *cmd_link = list_first(&hc->commands);

	while (cmd_link != NULL) {
		xhci_cmd_t *cmd = list_get_instance(cmd_link, xhci_cmd_t, link);

		if (cmd->trb_phys == phys)
			break;

		cmd_link = list_next(cmd_link, &hc->commands);
	}

	if (cmd_link != NULL) {
		list_remove(cmd_link);

		return list_get_instance(cmd_link, xhci_cmd_t, link);
	}

	return NULL;
}

static inline int enqueue_command(xhci_hc_t *hc, xhci_cmd_t *cmd, unsigned doorbell, unsigned target)
{
	assert(hc);
	assert(cmd);

	list_append(&cmd->link, &hc->commands);

	xhci_trb_ring_enqueue(&hc->command_ring, &cmd->trb, &cmd->trb_phys);
	hc_ring_doorbell(hc, doorbell, target);

	usb_log_debug2("HC(%p): Sent command:", hc);
	xhci_dump_trb(&cmd->trb);

	return EOK;
}

void xhci_stop_command_ring(xhci_hc_t *hc)
{
	assert(hc);

	XHCI_REG_SET(hc->op_regs, XHCI_OP_CS, 1);

	/**
	 * Note: There is a bug in qemu that checks CS only when CRCR_HI
	 *       is written, this (and the read/write in abort) ensures
	 *       the command rings stops.
	 */
	XHCI_REG_WR(hc->op_regs, XHCI_OP_CRCR_HI, XHCI_REG_RD(hc->op_regs, XHCI_OP_CRCR_HI));
}

void xhci_abort_command_ring(xhci_hc_t *hc)
{
	assert(hc);

	XHCI_REG_WR(hc->op_regs, XHCI_OP_CA, 1);
	XHCI_REG_WR(hc->op_regs, XHCI_OP_CRCR_HI, XHCI_REG_RD(hc->op_regs, XHCI_OP_CRCR_HI));
}

void xhci_start_command_ring(xhci_hc_t *hc)
{
	assert(hc);

	XHCI_REG_WR(hc->op_regs, XHCI_OP_CRR, 1);
	hc_ring_doorbell(hc, 0, 0);
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

int xhci_send_no_op_command(xhci_hc_t *hc, xhci_cmd_t *cmd)
{
	assert(hc);

	xhci_trb_clean(&cmd->trb);

	TRB_SET_TYPE(cmd->trb, XHCI_TRB_TYPE_NO_OP_CMD);

	return enqueue_command(hc, cmd, 0, 0);
}

int xhci_send_enable_slot_command(xhci_hc_t *hc, xhci_cmd_t *cmd)
{
	assert(hc);

	xhci_trb_clean(&cmd->trb);

	TRB_SET_TYPE(cmd->trb, XHCI_TRB_TYPE_ENABLE_SLOT_CMD);
	cmd->trb.control |= host2xhci(32, XHCI_REG_RD(hc->xecp, XHCI_EC_SP_SLOT_TYPE) << 16);

	return enqueue_command(hc, cmd, 0, 0);
}

int xhci_send_disable_slot_command(xhci_hc_t *hc, xhci_cmd_t *cmd)
{
	assert(hc);
	assert(cmd);

	xhci_trb_clean(&cmd->trb);

	TRB_SET_TYPE(cmd->trb, XHCI_TRB_TYPE_DISABLE_SLOT_CMD);
	TRB_SET_SLOT(cmd->trb, cmd->slot_id);

	return enqueue_command(hc, cmd, 0, 0);
}

int xhci_send_address_device_command(xhci_hc_t *hc, xhci_cmd_t *cmd, xhci_input_ctx_t *ictx)
{
	assert(hc);
	assert(cmd);
	assert(ictx);

	/**
	 * TODO: Requirements for this command:
	 *           dcbaa[slot_id] is properly sized and initialized
	 *           ictx has valids slot context and endpoint 0, all
	 *           other should be ignored at this point (see section 4.6.5).
	 */

	xhci_trb_clean(&cmd->trb);

	uint64_t phys_addr = (uint64_t) addr_to_phys(ictx);
	TRB_SET_ICTX(cmd->trb, phys_addr);

	/**
	 * Note: According to section 6.4.3.4, we can set the 9th bit
	 *       of the control field of the trb (BSR) to 1 and then the xHC
	 *       will not issue the SET_ADDRESS request to the USB device.
	 *       This can be used to provide compatibility with legacy USB devices
	 *       that require their device descriptor to be read before such request.
	 */
	TRB_SET_TYPE(cmd->trb, XHCI_TRB_TYPE_ADDRESS_DEVICE_CMD);
	TRB_SET_SLOT(cmd->trb, cmd->slot_id);

	return enqueue_command(hc, cmd, 0, 0);
}

int xhci_send_configure_endpoint_command(xhci_hc_t *hc, xhci_cmd_t *cmd, xhci_input_ctx_t *ictx)
{
	assert(hc);
	assert(cmd);
	assert(ictx);

	xhci_trb_clean(&cmd->trb);

	uint64_t phys_addr = (uint64_t) addr_to_phys(ictx);
	TRB_SET_ICTX(cmd->trb, phys_addr);

	TRB_SET_TYPE(cmd->trb, XHCI_TRB_TYPE_CONFIGURE_ENDPOINT_CMD);
	TRB_SET_SLOT(cmd->trb, cmd->slot_id);

	return enqueue_command(hc, cmd, 0, 0);
}

int xhci_send_evaluate_context_command(xhci_hc_t *hc, xhci_cmd_t *cmd, xhci_input_ctx_t *ictx)
{
	assert(hc);
	assert(cmd);
	assert(ictx);

	/**
	 * Note: All Drop Context flags of the input context shall be 0,
	 *       all Add Context flags shall be initialize to indicate IDs
	 *       of the contexts affected by the command.
	 *       Refer to sections 6.2.2.3 and 6.3.3.3 for further info.
	 */
	xhci_trb_clean(&cmd->trb);

	uint64_t phys_addr = (uint64_t) addr_to_phys(ictx);
	TRB_SET_ICTX(cmd->trb, phys_addr);

	TRB_SET_TYPE(cmd->trb, XHCI_TRB_TYPE_EVALUATE_CONTEXT_CMD);
	TRB_SET_SLOT(cmd->trb, cmd->slot_id);

	return enqueue_command(hc, cmd, 0, 0);
}

int xhci_send_reset_endpoint_command(xhci_hc_t *hc, xhci_cmd_t *cmd, uint32_t ep_id, uint8_t tcs)
{
	assert(hc);
	assert(cmd);

	/**
	 * Note: TCS can have values 0 or 1. If it is set to 0, see sectuon 4.5.8 for
	 *       information about this flag.
	 */
	xhci_trb_clean(&cmd->trb);

	TRB_SET_TYPE(cmd->trb, XHCI_TRB_TYPE_RESET_ENDPOINT_CMD);
	TRB_SET_TCS(cmd->trb, tcs);
	TRB_SET_EP(cmd->trb, ep_id);
	TRB_SET_SLOT(cmd->trb, cmd->slot_id);

	return enqueue_command(hc, cmd, 0, 0);
}

int xhci_send_stop_endpoint_command(xhci_hc_t *hc, xhci_cmd_t *cmd, uint32_t ep_id, uint8_t susp)
{
	assert(hc);
	assert(cmd);

	xhci_trb_clean(&cmd->trb);

	TRB_SET_TYPE(cmd->trb, XHCI_TRB_TYPE_STOP_ENDPOINT_CMD);
	TRB_SET_EP(cmd->trb, ep_id);
	TRB_SET_SUSP(cmd->trb, susp);
	TRB_SET_SLOT(cmd->trb, cmd->slot_id);

	return enqueue_command(hc, cmd, 0, 0);
}

int xhci_send_set_dequeue_ptr_command(xhci_hc_t *hc, xhci_cmd_t *cmd,
				      uintptr_t dequeue_ptr, uint16_t stream_id,
				      uint32_t ep_id)
{
	assert(hc);
	assert(cmd);

	xhci_trb_clean(&cmd->trb);

	TRB_SET_TYPE(cmd->trb, XHCI_TRB_TYPE_SET_TR_DEQUEUE_POINTER_CMD);
	TRB_SET_EP(cmd->trb, ep_id);
	TRB_SET_STREAM(cmd->trb, stream_id);
	TRB_SET_SLOT(cmd->trb, cmd->slot_id);
	TRB_SET_DEQUEUE_PTR(cmd->trb, dequeue_ptr);

	/**
	 * TODO: Set DCS (see section 4.6.10).
	 */

	return enqueue_command(hc, cmd, 0, 0);
}

int xhci_send_reset_device_command(xhci_hc_t *hc, xhci_cmd_t *cmd)
{
	assert(hc);
	assert(cmd);

	xhci_trb_clean(&cmd->trb);

	TRB_SET_TYPE(cmd->trb, XHCI_TRB_TYPE_RESET_DEVICE_CMD);
	TRB_SET_SLOT(cmd->trb, cmd->slot_id);

	return enqueue_command(hc, cmd, 0, 0);
}

int xhci_handle_command_completion(xhci_hc_t *hc, xhci_trb_t *trb)
{
	// TODO: Update dequeue ptrs.
	assert(hc);
	assert(trb);

	usb_log_debug("HC(%p) Command completed.", hc);

	int code;
	uint64_t phys;
	xhci_cmd_t *command;

	code = TRB_GET_CODE(*trb);
	phys = TRB_GET_PHYS(*trb);;
	command = get_command(hc, phys);
	if (command == NULL) {
		// TODO: STOP & ABORT may not have command structs in the list!
		usb_log_debug("No command struct for this completion event found.");

		if (code != XHCI_TRBC_SUCCESS)
			report_error(code);

		return EOK;
	}

	command->status = code;
	command->slot_id = TRB_GET_SLOT(*trb);

	usb_log_debug2("Completed command trb: %s", xhci_trb_str_type(TRB_TYPE(command->trb)));
	if (TRB_TYPE(command->trb) != XHCI_TRB_TYPE_NO_OP_CMD) {
		if (code != XHCI_TRBC_SUCCESS) {
			report_error(code);
			xhci_dump_trb(&command->trb);
		}
	}

	switch (TRB_TYPE(command->trb)) {
	case XHCI_TRB_TYPE_NO_OP_CMD:
		assert(code == XHCI_TRBC_TRB_ERROR);
		break;
	case XHCI_TRB_TYPE_ENABLE_SLOT_CMD:
		break;
	case XHCI_TRB_TYPE_DISABLE_SLOT_CMD:
		break;
	case XHCI_TRB_TYPE_ADDRESS_DEVICE_CMD:
		break;
	case XHCI_TRB_TYPE_CONFIGURE_ENDPOINT_CMD:
		break;
	case XHCI_TRB_TYPE_EVALUATE_CONTEXT_CMD:
		break;
	case XHCI_TRB_TYPE_RESET_ENDPOINT_CMD:
		break;
	case XHCI_TRB_TYPE_STOP_ENDPOINT_CMD:
		// Note: If the endpoint was in the middle of a transfer, then the xHC
		//       will add a Transfer TRB before the Event TRB, research that and
		//       handle it appropriately!
		break;
	case XHCI_TRB_TYPE_RESET_DEVICE_CMD:
		break;
	default:
		usb_log_debug2("Unsupported command trb: %s", xhci_trb_str_type(TRB_TYPE(command->trb)));

		command->completed = true;
		return ENAK;
	}

	fibril_mutex_lock(&command->completed_mtx);
	command->completed = true;
	fibril_condvar_broadcast(&command->completed_cv);
	fibril_mutex_unlock(&command->completed_mtx);

	return EOK;
}


/**
 * @}
 */
