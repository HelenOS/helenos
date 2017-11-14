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
#include "commands.h"
#include "debug.h"
#include "hc.h"
#include "hw_struct/context.h"
#include "hw_struct/trb.h"

#define TRB_SET_TCS(trb, tcs)   (trb).control |= host2xhci(32, ((tcs &0x1) << 9))
#define TRB_SET_TYPE(trb, type) (trb).control |= host2xhci(32, (type) << 10)
#define TRB_SET_DC(trb, dc)     (trb).control |= host2xhci(32, (dc) << 9)
#define TRB_SET_EP(trb, ep)     (trb).control |= host2xhci(32, ((ep) & 0x5) << 16)
#define TRB_SET_STREAM(trb, st) (trb).control |= host2xhci(32, ((st) & 0xFFFF) << 16)
#define TRB_SET_SUSP(trb, susp) (trb).control |= host2xhci(32, ((susp) & 0x1) << 23)
#define TRB_SET_SLOT(trb, slot) (trb).control |= host2xhci(32, (slot) << 24)
#define TRB_SET_DEV_SPEED(trb, speed)	(trb).control |= host2xhci(32, (speed & 0xF) << 16)

/**
 * TODO: Not sure about SCT and DCS (see section 6.4.3.9).
 */
#define TRB_SET_DEQUEUE_PTR(trb, dptr) (trb).parameter |= host2xhci(64, (dptr))
#define TRB_SET_ICTX(trb, phys) (trb).parameter |= host2xhci(64, (phys) & (~0xF))

#define TRB_GET_CODE(trb) XHCI_DWORD_EXTRACT((trb).status, 31, 24)
#define TRB_GET_SLOT(trb) XHCI_DWORD_EXTRACT((trb).control, 31, 24)
#define TRB_GET_PHYS(trb) (XHCI_QWORD_EXTRACT((trb).parameter, 63, 4) << 4)

/* Control functions */

int xhci_init_commands(xhci_hc_t *hc)
{
	assert(hc);

	list_initialize(&hc->commands);

	fibril_mutex_initialize(&hc->commands_mtx);

	return EOK;
}

void xhci_fini_commands(xhci_hc_t *hc)
{
	// Note: Untested.
	assert(hc);
}

void xhci_cmd_init(xhci_cmd_t *cmd, xhci_cmd_type_t type)
{
	memset(cmd, 0, sizeof(*cmd));

	link_initialize(&cmd->_header.link);

	fibril_mutex_initialize(&cmd->_header.completed_mtx);
	fibril_condvar_initialize(&cmd->_header.completed_cv);

	cmd->_header.cmd = type;
	cmd->_header.timeout = XHCI_DEFAULT_TIMEOUT;
}

void xhci_cmd_fini(xhci_cmd_t *cmd)
{
	list_remove(&cmd->_header.link);

	dma_buffer_free(&cmd->input_ctx);
	dma_buffer_free(&cmd->bandwidth_ctx);

	if (cmd->_header.async) {
		free(cmd);
	}
}

static inline xhci_cmd_t *get_command(xhci_hc_t *hc, uint64_t phys)
{
	fibril_mutex_lock(&hc->commands_mtx);

	link_t *cmd_link = list_first(&hc->commands);

	while (cmd_link != NULL) {
		xhci_cmd_t *cmd = list_get_instance(cmd_link, xhci_cmd_t, _header.link);

		if (cmd->_header.trb_phys == phys)
			break;

		cmd_link = list_next(cmd_link, &hc->commands);
	}

	if (cmd_link != NULL) {
		list_remove(cmd_link);
		fibril_mutex_unlock(&hc->commands_mtx);

		return list_get_instance(cmd_link, xhci_cmd_t, _header.link);
	}

	fibril_mutex_unlock(&hc->commands_mtx);
	return NULL;
}

static inline int enqueue_command(xhci_hc_t *hc, xhci_cmd_t *cmd, unsigned doorbell, unsigned target)
{
	assert(hc);
	assert(cmd);

	fibril_mutex_lock(&hc->commands_mtx);
	list_append(&cmd->_header.link, &hc->commands);
	fibril_mutex_unlock(&hc->commands_mtx);

	xhci_trb_ring_enqueue(&hc->command_ring, &cmd->_header.trb, &cmd->_header.trb_phys);
	hc_ring_doorbell(hc, doorbell, target);

	usb_log_debug2("HC(%p): Sent command:", hc);
	xhci_dump_trb(&cmd->_header.trb);

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

int xhci_handle_command_completion(xhci_hc_t *hc, xhci_trb_t *trb)
{
	// TODO: Update dequeue ptrs.
	assert(hc);
	assert(trb);

	usb_log_debug2("HC(%p) Command completed.", hc);

	int code;
	uint64_t phys;
	xhci_cmd_t *command;

	code = TRB_GET_CODE(*trb);
	phys = TRB_GET_PHYS(*trb);;
	command = get_command(hc, phys);
	if (command == NULL) {
		// TODO: STOP & ABORT may not have command structs in the list!
		usb_log_warning("No command struct for this completion event found.");

		if (code != XHCI_TRBC_SUCCESS)
			report_error(code);

		return EOK;
	}

	/* Semantics of NO_OP_CMD is that success is marked as a TRB error. */
	if (command->_header.cmd == XHCI_CMD_NO_OP && code == XHCI_TRBC_TRB_ERROR)
		code = XHCI_TRBC_SUCCESS;

	command->status = code;
	command->slot_id = TRB_GET_SLOT(*trb);

	usb_log_debug2("Completed command trb: %s", xhci_trb_str_type(TRB_TYPE(command->_header.trb)));

	if (code != XHCI_TRBC_SUCCESS) {
		report_error(code);
		xhci_dump_trb(&command->_header.trb);
	}

	switch (TRB_TYPE(command->_header.trb)) {
	case XHCI_TRB_TYPE_NO_OP_CMD:
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
		usb_log_debug2("Unsupported command trb: %s", xhci_trb_str_type(TRB_TYPE(command->_header.trb)));

		command->_header.completed = true;
		return ENAK;
	}

	fibril_mutex_lock(&command->_header.completed_mtx);
	command->_header.completed = true;
	fibril_condvar_broadcast(&command->_header.completed_cv);
	fibril_mutex_unlock(&command->_header.completed_mtx);

	if (command->_header.async) {
		/* Free the command and other DS upon completion. */
		xhci_cmd_fini(command);
	}

	return EOK;
}

/* Command-issuing functions */

static int no_op_cmd(xhci_hc_t *hc, xhci_cmd_t *cmd)
{
	assert(hc);

	xhci_trb_clean(&cmd->_header.trb);

	TRB_SET_TYPE(cmd->_header.trb, XHCI_TRB_TYPE_NO_OP_CMD);

	return enqueue_command(hc, cmd, 0, 0);
}

static int enable_slot_cmd(xhci_hc_t *hc, xhci_cmd_t *cmd)
{
	assert(hc);

	xhci_trb_clean(&cmd->_header.trb);

	TRB_SET_TYPE(cmd->_header.trb, XHCI_TRB_TYPE_ENABLE_SLOT_CMD);
	cmd->_header.trb.control |= host2xhci(32, XHCI_REG_RD(hc->xecp, XHCI_EC_SP_SLOT_TYPE) << 16);

	return enqueue_command(hc, cmd, 0, 0);
}

static int disable_slot_cmd(xhci_hc_t *hc, xhci_cmd_t *cmd)
{
	assert(hc);
	assert(cmd);

	xhci_trb_clean(&cmd->_header.trb);

	TRB_SET_TYPE(cmd->_header.trb, XHCI_TRB_TYPE_DISABLE_SLOT_CMD);
	TRB_SET_SLOT(cmd->_header.trb, cmd->slot_id);

	return enqueue_command(hc, cmd, 0, 0);
}

static int address_device_cmd(xhci_hc_t *hc, xhci_cmd_t *cmd)
{
	assert(hc);
	assert(cmd);
	assert(dma_buffer_is_set(&cmd->input_ctx));

	/**
	 * TODO: Requirements for this command:
	 *           dcbaa[slot_id] is properly sized and initialized
	 *           ictx has valids slot context and endpoint 0, all
	 *           other should be ignored at this point (see section 4.6.5).
	 */

	xhci_trb_clean(&cmd->_header.trb);

	TRB_SET_ICTX(cmd->_header.trb, cmd->input_ctx.phys);

	/**
	 * Note: According to section 6.4.3.4, we can set the 9th bit
	 *       of the control field of the trb (BSR) to 1 and then the xHC
	 *       will not issue the SET_ADDRESS request to the USB device.
	 *       This can be used to provide compatibility with legacy USB devices
	 *       that require their device descriptor to be read before such request.
	 */
	TRB_SET_TYPE(cmd->_header.trb, XHCI_TRB_TYPE_ADDRESS_DEVICE_CMD);
	TRB_SET_SLOT(cmd->_header.trb, cmd->slot_id);

	return enqueue_command(hc, cmd, 0, 0);
}

static int configure_endpoint_cmd(xhci_hc_t *hc, xhci_cmd_t *cmd)
{
	assert(hc);
	assert(cmd);

	xhci_trb_clean(&cmd->_header.trb);

	if (!cmd->deconfigure) {
		/* If the DC flag is on, input context is not evaluated. */
		assert(dma_buffer_is_set(&cmd->input_ctx));

		TRB_SET_ICTX(cmd->_header.trb, cmd->input_ctx.phys);
	}

	TRB_SET_TYPE(cmd->_header.trb, XHCI_TRB_TYPE_CONFIGURE_ENDPOINT_CMD);
	TRB_SET_SLOT(cmd->_header.trb, cmd->slot_id);
	TRB_SET_DC(cmd->_header.trb, cmd->deconfigure);

	return enqueue_command(hc, cmd, 0, 0);
}

static int evaluate_context_cmd(xhci_hc_t *hc, xhci_cmd_t *cmd)
{
	assert(hc);
	assert(cmd);
	assert(dma_buffer_is_set(&cmd->input_ctx));

	/**
	 * Note: All Drop Context flags of the input context shall be 0,
	 *       all Add Context flags shall be initialize to indicate IDs
	 *       of the contexts affected by the command.
	 *       Refer to sections 6.2.2.3 and 6.3.3.3 for further info.
	 */
	xhci_trb_clean(&cmd->_header.trb);

	TRB_SET_ICTX(cmd->_header.trb, cmd->input_ctx.phys);

	TRB_SET_TYPE(cmd->_header.trb, XHCI_TRB_TYPE_EVALUATE_CONTEXT_CMD);
	TRB_SET_SLOT(cmd->_header.trb, cmd->slot_id);

	return enqueue_command(hc, cmd, 0, 0);
}

static int reset_endpoint_cmd(xhci_hc_t *hc, xhci_cmd_t *cmd)
{
	assert(hc);
	assert(cmd);

	/**
	 * Note: TCS can have values 0 or 1. If it is set to 0, see sectuon 4.5.8 for
	 *       information about this flag.
	 */
	xhci_trb_clean(&cmd->_header.trb);

	TRB_SET_TYPE(cmd->_header.trb, XHCI_TRB_TYPE_RESET_ENDPOINT_CMD);
	TRB_SET_TCS(cmd->_header.trb, cmd->tcs);
	TRB_SET_EP(cmd->_header.trb, cmd->endpoint_id);
	TRB_SET_SLOT(cmd->_header.trb, cmd->slot_id);

	return enqueue_command(hc, cmd, 0, 0);
}

static int stop_endpoint_cmd(xhci_hc_t *hc, xhci_cmd_t *cmd)
{
	assert(hc);
	assert(cmd);

	xhci_trb_clean(&cmd->_header.trb);

	TRB_SET_TYPE(cmd->_header.trb, XHCI_TRB_TYPE_STOP_ENDPOINT_CMD);
	TRB_SET_EP(cmd->_header.trb, cmd->endpoint_id);
	TRB_SET_SUSP(cmd->_header.trb, cmd->susp);
	TRB_SET_SLOT(cmd->_header.trb, cmd->slot_id);

	return enqueue_command(hc, cmd, 0, 0);
}

static int set_tr_dequeue_pointer_cmd(xhci_hc_t *hc, xhci_cmd_t *cmd)
{
	assert(hc);
	assert(cmd);

	xhci_trb_clean(&cmd->_header.trb);

	TRB_SET_TYPE(cmd->_header.trb, XHCI_TRB_TYPE_SET_TR_DEQUEUE_POINTER_CMD);
	TRB_SET_EP(cmd->_header.trb, cmd->endpoint_id);
	TRB_SET_STREAM(cmd->_header.trb, cmd->stream_id);
	TRB_SET_SLOT(cmd->_header.trb, cmd->slot_id);
	TRB_SET_DEQUEUE_PTR(cmd->_header.trb, cmd->dequeue_ptr);

	/**
	 * TODO: Set DCS (see section 4.6.10).
	 */

	return enqueue_command(hc, cmd, 0, 0);
}

static int reset_device_cmd(xhci_hc_t *hc, xhci_cmd_t *cmd)
{
	assert(hc);
	assert(cmd);

	xhci_trb_clean(&cmd->_header.trb);

	TRB_SET_TYPE(cmd->_header.trb, XHCI_TRB_TYPE_RESET_DEVICE_CMD);
	TRB_SET_SLOT(cmd->_header.trb, cmd->slot_id);

	return enqueue_command(hc, cmd, 0, 0);
}

static int get_port_bandwidth_cmd(xhci_hc_t *hc, xhci_cmd_t *cmd)
{
	assert(hc);
	assert(cmd);

	xhci_trb_clean(&cmd->_header.trb);

	TRB_SET_ICTX(cmd->_header.trb, cmd->bandwidth_ctx.phys);

	TRB_SET_TYPE(cmd->_header.trb, XHCI_TRB_TYPE_GET_PORT_BANDWIDTH_CMD);
	TRB_SET_SLOT(cmd->_header.trb, cmd->slot_id);
	TRB_SET_DEV_SPEED(cmd->_header.trb, cmd->device_speed);

	return enqueue_command(hc, cmd, 0, 0);
}

/* The table of command-issuing functions. */

typedef int (*cmd_handler) (xhci_hc_t *hc, xhci_cmd_t *cmd);

static cmd_handler cmd_handlers [] = {
	[XHCI_CMD_ENABLE_SLOT] = enable_slot_cmd,
	[XHCI_CMD_DISABLE_SLOT] = disable_slot_cmd,
	[XHCI_CMD_ADDRESS_DEVICE] = address_device_cmd,
	[XHCI_CMD_CONFIGURE_ENDPOINT] = configure_endpoint_cmd,
	[XHCI_CMD_EVALUATE_CONTEXT] = evaluate_context_cmd,
	[XHCI_CMD_RESET_ENDPOINT] = reset_endpoint_cmd,
	[XHCI_CMD_STOP_ENDPOINT] = stop_endpoint_cmd,
	[XHCI_CMD_SET_TR_DEQUEUE_POINTER] = set_tr_dequeue_pointer_cmd,
	[XHCI_CMD_RESET_DEVICE] = reset_device_cmd,
	// TODO: Force event (optional normative, for VMM, section 4.6.12).
	[XHCI_CMD_FORCE_EVENT] = NULL,
	// TODO: Negotiate bandwidth (optional normative, section 4.6.13).
	[XHCI_CMD_NEGOTIATE_BANDWIDTH] = NULL,
	// TODO: Set latency tolerance value (optional normative, section 4.6.14).
	[XHCI_CMD_SET_LATENCY_TOLERANCE_VALUE] = NULL,
	// TODO: Get port bandwidth (mandatory, but needs root hub implementation, section 4.6.15).
	[XHCI_CMD_GET_PORT_BANDWIDTH] = get_port_bandwidth_cmd,
	// TODO: Force header (mandatory, but needs root hub implementation, section 4.6.16).
	[XHCI_CMD_FORCE_HEADER] = NULL,
	[XHCI_CMD_NO_OP] = no_op_cmd
};

static int wait_for_cmd_completion(xhci_cmd_t *cmd)
{
	int rv = EOK;

	fibril_mutex_lock(&cmd->_header.completed_mtx);
	while (!cmd->_header.completed) {
		usb_log_debug2("Waiting for event completion: going to sleep.");
		rv = fibril_condvar_wait_timeout(&cmd->_header.completed_cv, &cmd->_header.completed_mtx, cmd->_header.timeout);

		usb_log_debug2("Waiting for event completion: woken: %s", str_error(rv));
		if (rv == ETIMEOUT) {
			break;
		}
	}
	fibril_mutex_unlock(&cmd->_header.completed_mtx);

	return rv;
}

/** Issue command and block the current fibril until it is completed or timeout
 *  expires. Nothing is deallocated. Caller should always execute `xhci_cmd_fini`.
 */
int xhci_cmd_sync(xhci_hc_t *hc, xhci_cmd_t *cmd)
{
	assert(hc);
	assert(cmd);

	int err;

	if (!cmd_handlers[cmd->_header.cmd]) {
		/* Handler not implemented. */
		return ENOTSUP;
	}

	if ((err = cmd_handlers[cmd->_header.cmd](hc, cmd))) {
		/* Command could not be issued. */
		return err;
	}

	if ((err = wait_for_cmd_completion(cmd))) {
		/* Timeout expired or command failed. */
		return err;
	}

	return cmd->status == XHCI_TRBC_SUCCESS ? EOK : EINVAL;
}

/** Does the same thing as `xhci_cmd_sync` and executes `xhci_cmd_fini`. This
 *  is a useful shorthand for issuing commands without out parameters.
 */
int xhci_cmd_sync_fini(xhci_hc_t *hc, xhci_cmd_t *cmd)
{
	const int err = xhci_cmd_sync(hc, cmd);
	xhci_cmd_fini(cmd);

	return err;
}

/** Does the same thing as `xhci_cmd_sync_fini` without blocking the current
 *  fibril. The command is copied to stack memory and `fini` is called upon its completion.
 */
int xhci_cmd_async_fini(xhci_hc_t *hc, xhci_cmd_t *stack_cmd)
{
	assert(hc);
	assert(stack_cmd);

	/* Save the command for later. */
	xhci_cmd_t *heap_cmd = (xhci_cmd_t *) malloc(sizeof(xhci_cmd_t));
	if (!heap_cmd) {
		return ENOMEM;
	}

	/* TODO: Is this good for the mutex and the condvar? */
	memcpy(heap_cmd, stack_cmd, sizeof(xhci_cmd_t));
	heap_cmd->_header.async = true;

	/* Issue the command. */
	int err;

	if (!cmd_handlers[heap_cmd->_header.cmd]) {
		/* Handler not implemented. */
		err = ENOTSUP;
		goto err_heap_cmd;
	}

	if ((err = cmd_handlers[heap_cmd->_header.cmd](hc, heap_cmd))) {
		/* Command could not be issued. */
		goto err_heap_cmd;
	}

	return EOK;

err_heap_cmd:
	free(heap_cmd);
	return err;
}

/**
 * @}
 */
