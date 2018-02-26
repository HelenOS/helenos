/*
 * Copyright (c) 2018 Ondrej Hlavaty, Jan Hrach
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
 *
 * Utility functions for debugging and logging purposes.
 */

#ifndef XHCI_DEBUG_H
#define XHCI_DEBUG_H

/**
 * As the debug header is likely to be included in every file, avoid including
 * all headers of xhci to support "include what you use".
 */
struct xhci_hc;
struct xhci_cap_regs;
struct xhci_port_regs;
struct xhci_trb;
struct xhci_extcap;
struct xhci_slot_ctx;
struct xhci_endpoint_ctx;
struct xhci_input_ctx;

extern void xhci_dump_cap_regs(const struct xhci_cap_regs *);
extern void xhci_dump_port(const struct xhci_port_regs *);
extern void xhci_dump_state(const struct xhci_hc *);
extern void xhci_dump_ports(const struct xhci_hc *);

extern const char *xhci_trb_str_type(unsigned);
extern void xhci_dump_trb(const struct xhci_trb *trb);

extern const char *xhci_ec_str_id(unsigned);
extern void xhci_dump_extcap(const struct xhci_extcap *);

extern void xhci_dump_slot_ctx(const struct xhci_slot_ctx *);
extern void xhci_dump_endpoint_ctx(const struct xhci_endpoint_ctx *);
extern void xhci_dump_input_ctx(const struct xhci_hc *,
    const struct xhci_input_ctx *);

#endif
/**
 * @}
 */
