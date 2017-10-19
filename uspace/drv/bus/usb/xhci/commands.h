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
 * Utility functions used to place TRBs onto the command ring.
 */

#ifndef XHCI_COMMANDS_H
#define XHCI_COMMANDS_H

#include <adt/list.h>
#include <stdbool.h>
#include <fibril_synch.h>
#include "hw_struct/trb.h"

typedef struct xhci_hc xhci_hc_t;
typedef struct xhci_input_ctx xhci_input_ctx_t;
typedef struct xhci_port_bandwidth_ctx xhci_port_bandwidth_ctx_t;

typedef struct xhci_command {
	link_t link;

	xhci_trb_t trb;
	uintptr_t trb_phys;

	uint32_t slot_id;
	uint32_t status;

	bool completed;

	/* Will broadcast after command completes. */
	fibril_mutex_t completed_mtx;
	fibril_condvar_t completed_cv;
} xhci_cmd_t;

int xhci_init_commands(xhci_hc_t *);
void xhci_fini_commands(xhci_hc_t *);

xhci_cmd_t *xhci_cmd_alloc(void);
void xhci_cmd_init(xhci_cmd_t *);
int xhci_cmd_wait(xhci_cmd_t *, suseconds_t);
void xhci_cmd_fini(xhci_cmd_t *);
void xhci_cmd_free(xhci_cmd_t *);

void xhci_stop_command_ring(xhci_hc_t *);
void xhci_abort_command_ring(xhci_hc_t *);
void xhci_start_command_ring(xhci_hc_t *);

int xhci_send_no_op_command(xhci_hc_t *, xhci_cmd_t *);
int xhci_send_enable_slot_command(xhci_hc_t *, xhci_cmd_t *);
int xhci_send_disable_slot_command(xhci_hc_t *, xhci_cmd_t *);
int xhci_send_address_device_command(xhci_hc_t *, xhci_cmd_t *, xhci_input_ctx_t *);
int xhci_send_configure_endpoint_command(xhci_hc_t *, xhci_cmd_t *, xhci_input_ctx_t *);
int xhci_send_evaluate_context_command(xhci_hc_t *, xhci_cmd_t *, xhci_input_ctx_t *);
int xhci_send_reset_endpoint_command(xhci_hc_t *, xhci_cmd_t *, uint32_t, uint8_t);
int xhci_send_stop_endpoint_command(xhci_hc_t *, xhci_cmd_t *, uint32_t, uint8_t);
int xhci_send_set_dequeue_ptr_command(xhci_hc_t *, xhci_cmd_t *, uintptr_t, uint16_t, uint32_t);
int xhci_send_reset_device_command(xhci_hc_t *, xhci_cmd_t *);
// TODO: Force event (optional normative, for VMM, section 4.6.12).
// TODO: Negotiate bandwidth (optional normative, section 4.6.13).
// TODO: Set latency tolerance value (optional normative, section 4.6.14).
int xhci_get_port_bandwidth_command(xhci_hc_t *, xhci_cmd_t *, xhci_port_bandwidth_ctx_t *, uint8_t);
// TODO: Get port bandwidth (mandatory, but needs root hub implementation, section 4.6.15).
// TODO: Force header (mandatory, but needs root hub implementation, section 4.6.16).

int xhci_handle_command_completion(xhci_hc_t *, xhci_trb_t *);

#endif



/**
 * @}
 */
