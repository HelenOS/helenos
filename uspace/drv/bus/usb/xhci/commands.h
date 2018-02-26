/*
 * Copyright (c) 2018 Jaroslav Jindrak, Petr Manek, Ondrej Hlavaty, Jan Hrach
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
 * Utility functions used to place TRBs onto the command ring.
 */

#ifndef XHCI_COMMANDS_H
#define XHCI_COMMANDS_H

#include <adt/list.h>
#include <stdbool.h>
#include <fibril_synch.h>
#include <usb/dma_buffer.h>
#include "hw_struct/trb.h"
#include "trb_ring.h"

#define XHCI_COMMAND_TIMEOUT       10000000
#define XHCI_CR_ABORT_TIMEOUT       5000000

typedef struct xhci_hc xhci_hc_t;
typedef struct xhci_input_ctx xhci_input_ctx_t;
typedef struct xhci_port_bandwidth_ctx xhci_port_bandwidth_ctx_t;

typedef enum xhci_cmd_type {
	XHCI_CMD_ENABLE_SLOT,
	XHCI_CMD_DISABLE_SLOT,
	XHCI_CMD_ADDRESS_DEVICE,
	XHCI_CMD_CONFIGURE_ENDPOINT,
	XHCI_CMD_EVALUATE_CONTEXT,
	XHCI_CMD_RESET_ENDPOINT,
	XHCI_CMD_STOP_ENDPOINT,
	XHCI_CMD_SET_TR_DEQUEUE_POINTER,
	XHCI_CMD_RESET_DEVICE,
	XHCI_CMD_FORCE_EVENT,
	XHCI_CMD_NEGOTIATE_BANDWIDTH,
	XHCI_CMD_SET_LATENCY_TOLERANCE_VALUE,
	XHCI_CMD_GET_PORT_BANDWIDTH,
	XHCI_CMD_FORCE_HEADER,
	XHCI_CMD_NO_OP,
} xhci_cmd_type_t;

typedef enum {
	XHCI_CR_STATE_CLOSED,	/**< Commands are rejected with ENAK. */
	XHCI_CR_STATE_OPEN,	/**< Commands are enqueued normally. */
	XHCI_CR_STATE_CHANGING,	/**< Commands wait until state changes. */
	XHCI_CR_STATE_FULL,	/**< Commands wait until something completes. */
} xhci_cr_state_t;

typedef struct xhci_command_ring {
	xhci_trb_ring_t trb_ring;

	fibril_mutex_t guard;		/**< Guard access to this structure. */
	list_t cmd_list;

	xhci_cr_state_t state;		/**< Whether commands are allowed to be
					     added. */
	fibril_condvar_t state_cv;	/**< For waiting on CR state change. */

	fibril_condvar_t stopped_cv;	/**< For waiting on CR stopped event. */
} xhci_cmd_ring_t;

typedef struct xhci_command {
	/** Internal fields used for bookkeeping. Need not worry about these. */
	struct {
		link_t link;

		xhci_cmd_type_t cmd;

		xhci_trb_t trb;
		uintptr_t trb_phys;

		bool async;
		bool completed;

		/* Will broadcast after command completes. */
		fibril_mutex_t completed_mtx;
		fibril_condvar_t completed_cv;
	} _header;

	/** Below are arguments of all commands mixed together.
	 *  Be sure to know which command accepts what arguments. */

	uint32_t slot_id;
	uint32_t endpoint_id;
	uint16_t stream_id;

	dma_buffer_t input_ctx, bandwidth_ctx;
	uintptr_t dequeue_ptr;

	bool tsp;
	uint8_t susp;
	uint8_t device_speed;
	uint32_t status;
	bool deconfigure;
} xhci_cmd_t;

/* Command handling control */
extern errno_t xhci_init_commands(xhci_hc_t *);
extern void xhci_fini_commands(xhci_hc_t *);

extern void xhci_nuke_command_ring(xhci_hc_t *);
extern void xhci_stop_command_ring(xhci_hc_t *);
extern void xhci_abort_command_ring(xhci_hc_t *);
extern void xhci_start_command_ring(xhci_hc_t *);

extern errno_t xhci_handle_command_completion(xhci_hc_t *, xhci_trb_t *);

/* Command lifecycle */
extern void xhci_cmd_init(xhci_cmd_t *, xhci_cmd_type_t);
extern void xhci_cmd_fini(xhci_cmd_t *);

/* Issuing commands */
extern errno_t xhci_cmd_sync(xhci_hc_t *, xhci_cmd_t *);
extern errno_t xhci_cmd_sync_fini(xhci_hc_t *, xhci_cmd_t *);
extern errno_t xhci_cmd_async_fini(xhci_hc_t *, xhci_cmd_t *);

static inline errno_t xhci_cmd_sync_inline_wrapper(xhci_hc_t *hc, xhci_cmd_t cmd)
{
	/* Poor man's xhci_cmd_init (everything else is zeroed) */
	link_initialize(&cmd._header.link);
	fibril_mutex_initialize(&cmd._header.completed_mtx);
	fibril_condvar_initialize(&cmd._header.completed_cv);

	/* Issue the command */
	const errno_t err = xhci_cmd_sync(hc, &cmd);
	xhci_cmd_fini(&cmd);

	return err;
}

/** The inline macro expects:
 *    - hc      - HC to schedule command on (xhci_hc_t *).
 *    - command - Member of `xhci_cmd_type_t` without the "XHCI_CMD_" prefix.
 *    - VA_ARGS - (optional) Command arguments in struct initialization notation.
 *
 *  The return code and semantics matches those of `xhci_cmd_sync_fini`.
 *
 *  Example:
 *    errno_t err = xhci_cmd_sync_inline(hc, DISABLE_SLOT, .slot_id = 42);
 */

#define xhci_cmd_sync_inline(hc, command, ...) \
	xhci_cmd_sync_inline_wrapper(hc, \
	(xhci_cmd_t) { ._header.cmd = XHCI_CMD_##command, ##__VA_ARGS__ })

#endif



/**
 * @}
 */
