/*
 * SPDX-FileCopyrightText: 2018 Ondrej Hlavaty, Jan Hrach
 *
 * SPDX-License-Identifier: BSD-3-Clause
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
