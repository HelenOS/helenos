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

typedef struct xhci_hc xhci_hc_t;
typedef struct xhci_trb xhci_trb_t;
typedef struct xhci_input_ctx xhci_input_ctx_t;

int xhci_send_no_op_command(xhci_hc_t *);
int xhci_send_enable_slot_command(xhci_hc_t *);
int xhci_send_disable_slot_command(xhci_hc_t *, uint32_t);
int xhci_send_address_device_command(xhci_hc_t *, uint32_t, xhci_input_ctx_t *);
int xhci_send_configure_endpoint_command(xhci_hc_t *, uint32_t, xhci_input_ctx_t *);
int xhci_send_evaluate_context_command(xhci_hc_t *, uint32_t, xhci_input_ctx_t *);
int xhci_send_reset_endpoint_command(xhci_hc_t *, uint32_t, uint32_t, uint8_t);
int xhci_send_stop_endpoint_command(xhci_hc_t *, uint32_t, uint32_t, uint8_t);

int xhci_handle_command_completion(xhci_hc_t *, xhci_trb_t *);

#endif



/**
 * @}
 */
