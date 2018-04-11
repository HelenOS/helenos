/*
 * Copyright (c) 2018 Jaroslav Jindrak, Ondrej Hlavaty, Petr Manek, Michal Staruch, Jan Hrach
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

#define TRB_SET_TSP(trb, tsp)   (trb).control |= host2xhci(32, (((tsp) & 0x1) << 9))
#define TRB_SET_TYPE(trb, type) (trb).control |= host2xhci(32, (type) << 10)
#define TRB_SET_DC(trb, dc)     (trb).control |= host2xhci(32, (dc) << 9)
#define TRB_SET_EP(trb, ep)     (trb).control |= host2xhci(32, ((ep) & 0x5) << 16)
#define TRB_SET_STREAM(trb, st) (trb).control |= host2xhci(32, ((st) & 0xFFFF) << 16)
#define TRB_SET_SUSP(trb, susp) (trb).control |= host2xhci(32, ((susp) & 0x1) << 23)
#define TRB_SET_SLOT(trb, slot) (trb).control |= host2xhci(32, (slot) << 24)
#define TRB_SET_DEV_SPEED(trb, speed)	(trb).control |= host2xhci(32, (speed & 0xF) << 16)
#define TRB_SET_DEQUEUE_PTR(trb, dptr) (trb).parameter |= host2xhci(64, (dptr))
#define TRB_SET_ICTX(trb, phys) (trb).parameter |= host2xhci(64, (phys) & (~0xF))

#define TRB_GET_CODE(trb) XHCI_DWORD_EXTRACT((trb).status, 31, 24)
#define TRB_GET_SLOT(trb) XHCI_DWORD_EXTRACT((trb).control, 31, 24)
#define TRB_GET_PHYS(trb) (XHCI_QWORD_EXTRACT((trb).parameter, 63, 4) << 4)

/* Control functions */

static xhci_cmd_ring_t *get_cmd_ring(xhci_hc_t *hc)
{
	assert(hc);
	return &hc->cr;
}

/**
 * Initialize the command subsystem. Allocates the comand ring.
 *
 * Does not configure the CR pointer to the hardware, because the xHC will be
 * reset before starting.
 */
errno_t xhci_init_commands(xhci_hc_t *hc)
{
	xhci_cmd_ring_t *cr = get_cmd_ring(hc);
	errno_t err;

	if ((err = xhci_trb_ring_init(&cr->trb_ring, 0)))
		return err;

	fibril_mutex_initialize(&cr->guard);
	fibril_condvar_initialize(&cr->state_cv);
	fibril_condvar_initialize(&cr->stopped_cv);

	list_initialize(&cr->cmd_list);

	return EOK;
}

/**
 * Finish the command subsystem. Stops the hardware from running commands, then
 * deallocates the ring.
 */
void xhci_fini_commands(xhci_hc_t *hc)
{
	assert(hc);
	xhci_stop_command_ring(hc);

	xhci_cmd_ring_t *cr = get_cmd_ring(hc);

	fibril_mutex_lock(&cr->guard);
	xhci_trb_ring_fini(&cr->trb_ring);
	fibril_mutex_unlock(&cr->guard);
}

/**
 * Initialize a command structure for the given command.
 */
void xhci_cmd_init(xhci_cmd_t *cmd, xhci_cmd_type_t type)
{
	memset(cmd, 0, sizeof(*cmd));

	link_initialize(&cmd->_header.link);

	fibril_mutex_initialize(&cmd->_header.completed_mtx);
	fibril_condvar_initialize(&cmd->_header.completed_cv);

	cmd->_header.cmd = type;
}

/**
 * Finish the command structure. Some command invocation includes allocating
 * a context structure. To have the convenience in calling commands, this
 * method deallocates all resources.
 */
void xhci_cmd_fini(xhci_cmd_t *cmd)
{
	list_remove(&cmd->_header.link);

	dma_buffer_free(&cmd->input_ctx);
	dma_buffer_free(&cmd->bandwidth_ctx);

	if (cmd->_header.async) {
		free(cmd);
	}
}

/**
 * Find a command issued by TRB at @c phys inside the command list.
 *
 * Call with guard locked only.
 */
static inline xhci_cmd_t *find_command(xhci_hc_t *hc, uint64_t phys)
{
	xhci_cmd_ring_t *cr = get_cmd_ring(hc);
	assert(fibril_mutex_is_locked(&cr->guard));

	link_t *cmd_link = list_first(&cr->cmd_list);

	while (cmd_link != NULL) {
		xhci_cmd_t *cmd = list_get_instance(cmd_link, xhci_cmd_t,
		    _header.link);

		if (cmd->_header.trb_phys == phys)
			break;

		cmd_link = list_next(cmd_link, &cr->cmd_list);
	}

	return cmd_link ?
	    list_get_instance(cmd_link, xhci_cmd_t, _header.link) :
	    NULL;
}

static void cr_set_state(xhci_cmd_ring_t *cr, xhci_cr_state_t state)
{
	assert(fibril_mutex_is_locked(&cr->guard));

	cr->state = state;
	if (state == XHCI_CR_STATE_OPEN || state == XHCI_CR_STATE_CLOSED)
		fibril_condvar_broadcast(&cr->state_cv);
}

static errno_t wait_for_ring_open(xhci_cmd_ring_t *cr)
{
	assert(fibril_mutex_is_locked(&cr->guard));

	while (true) {
		switch (cr->state) {
		case XHCI_CR_STATE_CHANGING:
		case XHCI_CR_STATE_FULL:
			fibril_condvar_wait(&cr->state_cv, &cr->guard);
			break;
		case XHCI_CR_STATE_OPEN:
			return EOK;
		case XHCI_CR_STATE_CLOSED:
			return ENAK;
		}
	}
}

/**
 * Enqueue a command on the TRB ring. Ring the doorbell to initiate processing.
 * Register the command as waiting for completion inside the command list.
 */
static inline errno_t enqueue_command(xhci_hc_t *hc, xhci_cmd_t *cmd)
{
	xhci_cmd_ring_t *cr = get_cmd_ring(hc);
	assert(cmd);

	fibril_mutex_lock(&cr->guard);

	if (wait_for_ring_open(cr)) {
		fibril_mutex_unlock(&cr->guard);
		return ENAK;
	}

	usb_log_debug("Sending command %s",
	    xhci_trb_str_type(TRB_TYPE(cmd->_header.trb)));

	list_append(&cmd->_header.link, &cr->cmd_list);

	errno_t err = EOK;
	while (err == EOK) {
		err = xhci_trb_ring_enqueue(&cr->trb_ring,
		    &cmd->_header.trb, &cmd->_header.trb_phys);
		if (err != EAGAIN)
			break;

		cr_set_state(cr, XHCI_CR_STATE_FULL);
		err = wait_for_ring_open(cr);
	}

	if (err == EOK)
		hc_ring_doorbell(hc, 0, 0);

	fibril_mutex_unlock(&cr->guard);

	return err;
}

/**
 * Stop the command ring. Stop processing commands, block issuing new ones.
 * Wait until hardware acknowledges it is stopped.
 */
void xhci_stop_command_ring(xhci_hc_t *hc)
{
	xhci_cmd_ring_t *cr = get_cmd_ring(hc);

	fibril_mutex_lock(&cr->guard);

	// Prevent others from starting CR again.
	cr_set_state(cr, XHCI_CR_STATE_CLOSED);

	XHCI_REG_SET(hc->op_regs, XHCI_OP_CS, 1);

	while (XHCI_REG_RD(hc->op_regs, XHCI_OP_CRR))
		fibril_condvar_wait(&cr->stopped_cv, &cr->guard);

	fibril_mutex_unlock(&cr->guard);
}

/**
 * Mark the command ring as stopped. NAK new commands, abort running, do not
 * touch the HC as it's probably broken.
 */
void xhci_nuke_command_ring(xhci_hc_t *hc)
{
	xhci_cmd_ring_t *cr = get_cmd_ring(hc);
	fibril_mutex_lock(&cr->guard);
	// Prevent others from starting CR again.
	cr_set_state(cr, XHCI_CR_STATE_CLOSED);

	XHCI_REG_SET(hc->op_regs, XHCI_OP_CS, 1);
	fibril_mutex_unlock(&cr->guard);
}

/**
 * Mark the command ring as working again.
 */
void xhci_start_command_ring(xhci_hc_t *hc)
{
	xhci_cmd_ring_t *cr = get_cmd_ring(hc);
	fibril_mutex_lock(&cr->guard);
	// Prevent others from starting CR again.
	cr_set_state(cr, XHCI_CR_STATE_OPEN);
	fibril_mutex_unlock(&cr->guard);
}

/**
 * Abort currently processed command. Note that it is only aborted when the
 * command is "blocking" - see section 4.6.1.2 of xHCI spec.
 */
static void abort_command_ring(xhci_hc_t *hc)
{
	XHCI_REG_SET(hc->op_regs, XHCI_OP_CA, 1);
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

/**
 * Report an error according to command completion code.
 */
static void report_error(int code)
{
	if (code < XHCI_TRBC_MAX && trb_codes[code] != NULL)
		usb_log_error("Command resulted in error: %s.", trb_codes[code]);
	else
		usb_log_error("Command resulted in reserved or "
		    "vendor specific error.");
}

/**
 * Handle a command completion. Feed the fibril waiting for result.
 *
 * @param trb The COMMAND_COMPLETION TRB found in event ring.
 */
errno_t xhci_handle_command_completion(xhci_hc_t *hc, xhci_trb_t *trb)
{
	xhci_cmd_ring_t *cr = get_cmd_ring(hc);
	assert(trb);

	fibril_mutex_lock(&cr->guard);

	int code = TRB_GET_CODE(*trb);

	if (code == XHCI_TRBC_COMMAND_RING_STOPPED) {
		/* This can either mean that the ring is being stopped, or
		 * a command was aborted. In either way, wake threads waiting
		 * on stopped_cv.
		 *
		 * Note that we need to hold mutex, because we must be sure the
		 * requesting thread is waiting inside the CV.
		 */
		usb_log_debug("Command ring stopped.");
		fibril_condvar_broadcast(&cr->stopped_cv);
		fibril_mutex_unlock(&cr->guard);
		return EOK;
	}

	const uint64_t phys = TRB_GET_PHYS(*trb);
	xhci_trb_ring_update_dequeue(&cr->trb_ring, phys);

	if (cr->state == XHCI_CR_STATE_FULL)
		cr_set_state(cr, XHCI_CR_STATE_OPEN);

	xhci_cmd_t *command = find_command(hc, phys);
	if (command == NULL) {
		usb_log_error("No command struct for completion event found.");

		if (code != XHCI_TRBC_SUCCESS)
			report_error(code);

		return EOK;
	}

	list_remove(&command->_header.link);

	/* Semantics of NO_OP_CMD is that success is marked as a TRB error. */
	if (command->_header.cmd == XHCI_CMD_NO_OP && code == XHCI_TRBC_TRB_ERROR)
		code = XHCI_TRBC_SUCCESS;

	command->status = code;
	command->slot_id = TRB_GET_SLOT(*trb);

	usb_log_debug("Completed command %s",
	    xhci_trb_str_type(TRB_TYPE(command->_header.trb)));

	if (code != XHCI_TRBC_SUCCESS) {
		report_error(code);
		xhci_dump_trb(&command->_header.trb);
	}

	fibril_mutex_unlock(&cr->guard);

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

static errno_t no_op_cmd(xhci_hc_t *hc, xhci_cmd_t *cmd)
{
	assert(hc);

	xhci_trb_clean(&cmd->_header.trb);

	TRB_SET_TYPE(cmd->_header.trb, XHCI_TRB_TYPE_NO_OP_CMD);

	return enqueue_command(hc, cmd);
}

static errno_t enable_slot_cmd(xhci_hc_t *hc, xhci_cmd_t *cmd)
{
	assert(hc);

	xhci_trb_clean(&cmd->_header.trb);

	TRB_SET_TYPE(cmd->_header.trb, XHCI_TRB_TYPE_ENABLE_SLOT_CMD);
	cmd->_header.trb.control |=
	    host2xhci(32, XHCI_REG_RD(hc->xecp, XHCI_EC_SP_SLOT_TYPE) << 16);

	return enqueue_command(hc, cmd);
}

static errno_t disable_slot_cmd(xhci_hc_t *hc, xhci_cmd_t *cmd)
{
	assert(hc);
	assert(cmd);

	xhci_trb_clean(&cmd->_header.trb);

	TRB_SET_TYPE(cmd->_header.trb, XHCI_TRB_TYPE_DISABLE_SLOT_CMD);
	TRB_SET_SLOT(cmd->_header.trb, cmd->slot_id);

	return enqueue_command(hc, cmd);
}

static errno_t address_device_cmd(xhci_hc_t *hc, xhci_cmd_t *cmd)
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

	const uintptr_t phys = dma_buffer_phys_base(&cmd->input_ctx);
	TRB_SET_ICTX(cmd->_header.trb, phys);

	/**
	 * Note: According to section 6.4.3.4, we can set the 9th bit
	 *       of the control field of the trb (BSR) to 1 and then the xHC
	 *       will not issue the SET_ADDRESS request to the USB device.
	 *       This can be used to provide compatibility with legacy USB devices
	 *       that require their device descriptor to be read before such request.
	 */
	TRB_SET_TYPE(cmd->_header.trb, XHCI_TRB_TYPE_ADDRESS_DEVICE_CMD);
	TRB_SET_SLOT(cmd->_header.trb, cmd->slot_id);

	return enqueue_command(hc, cmd);
}

static errno_t configure_endpoint_cmd(xhci_hc_t *hc, xhci_cmd_t *cmd)
{
	assert(hc);
	assert(cmd);

	xhci_trb_clean(&cmd->_header.trb);

	if (!cmd->deconfigure) {
		/* If the DC flag is on, input context is not evaluated. */
		assert(dma_buffer_is_set(&cmd->input_ctx));

		const uintptr_t phys = dma_buffer_phys_base(&cmd->input_ctx);
		TRB_SET_ICTX(cmd->_header.trb, phys);
	}

	TRB_SET_TYPE(cmd->_header.trb, XHCI_TRB_TYPE_CONFIGURE_ENDPOINT_CMD);
	TRB_SET_SLOT(cmd->_header.trb, cmd->slot_id);
	TRB_SET_DC(cmd->_header.trb, cmd->deconfigure);

	return enqueue_command(hc, cmd);
}

static errno_t evaluate_context_cmd(xhci_hc_t *hc, xhci_cmd_t *cmd)
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

	const uintptr_t phys = dma_buffer_phys_base(&cmd->input_ctx);
	TRB_SET_ICTX(cmd->_header.trb, phys);

	TRB_SET_TYPE(cmd->_header.trb, XHCI_TRB_TYPE_EVALUATE_CONTEXT_CMD);
	TRB_SET_SLOT(cmd->_header.trb, cmd->slot_id);

	return enqueue_command(hc, cmd);
}

static errno_t reset_endpoint_cmd(xhci_hc_t *hc, xhci_cmd_t *cmd)
{
	assert(hc);
	assert(cmd);

	xhci_trb_clean(&cmd->_header.trb);

	TRB_SET_TYPE(cmd->_header.trb, XHCI_TRB_TYPE_RESET_ENDPOINT_CMD);
	TRB_SET_TSP(cmd->_header.trb, cmd->tsp);
	TRB_SET_EP(cmd->_header.trb, cmd->endpoint_id);
	TRB_SET_SLOT(cmd->_header.trb, cmd->slot_id);

	return enqueue_command(hc, cmd);
}

static errno_t stop_endpoint_cmd(xhci_hc_t *hc, xhci_cmd_t *cmd)
{
	assert(hc);
	assert(cmd);

	xhci_trb_clean(&cmd->_header.trb);

	TRB_SET_TYPE(cmd->_header.trb, XHCI_TRB_TYPE_STOP_ENDPOINT_CMD);
	TRB_SET_EP(cmd->_header.trb, cmd->endpoint_id);
	TRB_SET_SUSP(cmd->_header.trb, cmd->susp);
	TRB_SET_SLOT(cmd->_header.trb, cmd->slot_id);

	return enqueue_command(hc, cmd);
}

static errno_t set_tr_dequeue_pointer_cmd(xhci_hc_t *hc, xhci_cmd_t *cmd)
{
	assert(hc);
	assert(cmd);

	xhci_trb_clean(&cmd->_header.trb);

	TRB_SET_TYPE(cmd->_header.trb, XHCI_TRB_TYPE_SET_TR_DEQUEUE_POINTER_CMD);
	TRB_SET_EP(cmd->_header.trb, cmd->endpoint_id);
	TRB_SET_STREAM(cmd->_header.trb, cmd->stream_id);
	TRB_SET_SLOT(cmd->_header.trb, cmd->slot_id);
	TRB_SET_DEQUEUE_PTR(cmd->_header.trb, cmd->dequeue_ptr);

	return enqueue_command(hc, cmd);
}

static errno_t reset_device_cmd(xhci_hc_t *hc, xhci_cmd_t *cmd)
{
	assert(hc);
	assert(cmd);

	xhci_trb_clean(&cmd->_header.trb);

	TRB_SET_TYPE(cmd->_header.trb, XHCI_TRB_TYPE_RESET_DEVICE_CMD);
	TRB_SET_SLOT(cmd->_header.trb, cmd->slot_id);

	return enqueue_command(hc, cmd);
}

static errno_t get_port_bandwidth_cmd(xhci_hc_t *hc, xhci_cmd_t *cmd)
{
	assert(hc);
	assert(cmd);

	xhci_trb_clean(&cmd->_header.trb);

	const uintptr_t phys = dma_buffer_phys_base(&cmd->input_ctx);
	TRB_SET_ICTX(cmd->_header.trb, phys);

	TRB_SET_TYPE(cmd->_header.trb, XHCI_TRB_TYPE_GET_PORT_BANDWIDTH_CMD);
	TRB_SET_SLOT(cmd->_header.trb, cmd->slot_id);
	TRB_SET_DEV_SPEED(cmd->_header.trb, cmd->device_speed);

	return enqueue_command(hc, cmd);
}

/* The table of command-issuing functions. */

typedef errno_t (*cmd_handler) (xhci_hc_t *hc, xhci_cmd_t *cmd);

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
	[XHCI_CMD_FORCE_EVENT] = NULL,
	[XHCI_CMD_NEGOTIATE_BANDWIDTH] = NULL,
	[XHCI_CMD_SET_LATENCY_TOLERANCE_VALUE] = NULL,
	[XHCI_CMD_GET_PORT_BANDWIDTH] = get_port_bandwidth_cmd,
	[XHCI_CMD_FORCE_HEADER] = NULL,
	[XHCI_CMD_NO_OP] = no_op_cmd
};

/**
 * Try to abort currently processed command. This is tricky, because
 * calling fibril is not necessarily the one which issued the blocked command.
 * Also, the trickiness intensifies by the fact that stopping a CR is denoted by
 * event, which is again handled in different fibril. but, once we go to sleep
 * on waiting for that event, another fibril may wake up and try to abort the
 * blocked command.
 *
 * So, we mark the command ring as being restarted, wait for it to stop, and
 * then start it again. If there was a blocked command, it will be satisfied by
 * COMMAND_ABORTED event.
 */
static errno_t try_abort_current_command(xhci_hc_t *hc)
{
	xhci_cmd_ring_t *cr = get_cmd_ring(hc);

	fibril_mutex_lock(&cr->guard);

	if (cr->state == XHCI_CR_STATE_CLOSED) {
		fibril_mutex_unlock(&cr->guard);
		return ENAK;
	}

	if (cr->state == XHCI_CR_STATE_CHANGING) {
		fibril_mutex_unlock(&cr->guard);
		return EOK;
	}

	usb_log_error("Timeout while waiting for command: "
	    "aborting current command.");

	cr_set_state(cr, XHCI_CR_STATE_CHANGING);

	abort_command_ring(hc);

	fibril_condvar_wait_timeout(&cr->stopped_cv, &cr->guard,
	    XHCI_CR_ABORT_TIMEOUT);

	if (XHCI_REG_RD(hc->op_regs, XHCI_OP_CRR)) {
		/* 4.6.1.2, implementation note
		 * Assume there are larger problems with HC and
		 * reset it.
		 */
		usb_log_error("Command didn't abort.");

		cr_set_state(cr, XHCI_CR_STATE_CLOSED);

		// TODO: Reset HC completely.
		// Don't forget to somehow complete all commands with error.

		fibril_mutex_unlock(&cr->guard);
		return ENAK;
	}

	cr_set_state(cr, XHCI_CR_STATE_OPEN);

	fibril_mutex_unlock(&cr->guard);

	usb_log_error("Command ring stopped. Starting again.");
	hc_ring_doorbell(hc, 0, 0);

	return EOK;
}

/**
 * Wait, until the command is completed. The completion is triggered by
 * COMMAND_COMPLETION event. As we do not want to rely on HW completing the
 * command in timely manner, we timeout. Note that we can't just return an
 * error after the timeout pass - it may be other command blocking the ring,
 * and ours can be completed afterwards. Therefore, it is not guaranteed that
 * this function will return in XHCI_COMMAND_TIMEOUT. It will continue waiting
 * until COMMAND_COMPLETION event arrives.
 */
static errno_t wait_for_cmd_completion(xhci_hc_t *hc, xhci_cmd_t *cmd)
{
	errno_t rv = EOK;

	if (fibril_get_id() == hc->event_handler) {
		usb_log_error("Deadlock detected in waiting for command.");
		abort();
	}

	fibril_mutex_lock(&cmd->_header.completed_mtx);
	while (!cmd->_header.completed) {

		rv = fibril_condvar_wait_timeout(&cmd->_header.completed_cv,
		    &cmd->_header.completed_mtx, XHCI_COMMAND_TIMEOUT);

		/* The waiting timed out. Current command (not necessarily
		 * ours) is probably blocked.
		 */
		if (!cmd->_header.completed && rv == ETIMEOUT) {
			fibril_mutex_unlock(&cmd->_header.completed_mtx);

			rv = try_abort_current_command(hc);
			if (rv)
				return rv;

			fibril_mutex_lock(&cmd->_header.completed_mtx);
		}
	}
	fibril_mutex_unlock(&cmd->_header.completed_mtx);

	return rv;
}

/**
 * Issue command and block the current fibril until it is completed or timeout
 * expires. Nothing is deallocated. Caller should always execute `xhci_cmd_fini`.
 */
errno_t xhci_cmd_sync(xhci_hc_t *hc, xhci_cmd_t *cmd)
{
	assert(hc);
	assert(cmd);

	errno_t err;

	if (!cmd_handlers[cmd->_header.cmd]) {
		/* Handler not implemented. */
		return ENOTSUP;
	}

	if ((err = cmd_handlers[cmd->_header.cmd](hc, cmd))) {
		/* Command could not be issued. */
		return err;
	}

	if ((err = wait_for_cmd_completion(hc, cmd))) {
		/* Command failed. */
		return err;
	}

	switch (cmd->status) {
	case XHCI_TRBC_SUCCESS:
		return EOK;
	case XHCI_TRBC_USB_TRANSACTION_ERROR:
		return ESTALL;
	case XHCI_TRBC_RESOURCE_ERROR:
	case XHCI_TRBC_BANDWIDTH_ERROR:
	case XHCI_TRBC_NO_SLOTS_ERROR:
		return ELIMIT;
	case XHCI_TRBC_SLOT_NOT_ENABLED_ERROR:
		return ENOENT;
	default:
		return EINVAL;
	}
}

/**
 * Does the same thing as `xhci_cmd_sync` and executes `xhci_cmd_fini`. This
 * is a useful shorthand for issuing commands without out parameters.
 */
errno_t xhci_cmd_sync_fini(xhci_hc_t *hc, xhci_cmd_t *cmd)
{
	const errno_t err = xhci_cmd_sync(hc, cmd);
	xhci_cmd_fini(cmd);

	return err;
}

/**
 * Does the same thing as `xhci_cmd_sync_fini` without blocking the current
 * fibril. The command is copied to stack memory and `fini` is called upon its completion.
 */
errno_t xhci_cmd_async_fini(xhci_hc_t *hc, xhci_cmd_t *stack_cmd)
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
	errno_t err;

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
