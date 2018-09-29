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
 * Various functions to examine current state of the xHC.
 */

#include <inttypes.h>
#include <byteorder.h>
#include <usb/debug.h>

#include "hw_struct/trb.h"
#include "debug.h"
#include "hc.h"

#define PX "\t%-21s = "

#define DUMP_REG_FIELD(ptr, title, size, ...) \
	usb_log_debug(PX "%" PRIu##size, title, XHCI_REG_RD_FIELD(ptr, size, ##__VA_ARGS__))

#define DUMP_REG_RANGE(ptr, title, size, ...) \
	usb_log_debug(PX "%" PRIu##size, title, XHCI_REG_RD_RANGE(ptr, size, ##__VA_ARGS__))

#define DUMP_REG_FLAG(ptr, title, size, ...) \
	usb_log_debug(PX "%s", title, XHCI_REG_RD_FLAG(ptr, size, ##__VA_ARGS__) ? "true" : "false")

#define DUMP_REG_INNER(set, title, field, size, type, ...) \
	DUMP_REG_##type(&(set)->field, title, size, ##__VA_ARGS__)

#define DUMP_REG(set, c) DUMP_REG_INNER(set, #c, c)

/**
 * Dumps all capability registers.
 */
void xhci_dump_cap_regs(const xhci_cap_regs_t *cap)
{
	usb_log_debug("Capabilities:");

	DUMP_REG(cap, XHCI_CAP_LENGTH);
	DUMP_REG(cap, XHCI_CAP_VERSION);
	DUMP_REG(cap, XHCI_CAP_MAX_SLOTS);
	DUMP_REG(cap, XHCI_CAP_MAX_INTRS);
	DUMP_REG(cap, XHCI_CAP_MAX_PORTS);
	DUMP_REG(cap, XHCI_CAP_IST);
	DUMP_REG(cap, XHCI_CAP_ERST_MAX);
	usb_log_debug(PX "%u", "Max Scratchpad bufs", xhci_get_max_spbuf(cap));
	DUMP_REG(cap, XHCI_CAP_SPR);
	DUMP_REG(cap, XHCI_CAP_U1EL);
	DUMP_REG(cap, XHCI_CAP_U2EL);
	DUMP_REG(cap, XHCI_CAP_AC64);
	DUMP_REG(cap, XHCI_CAP_BNC);
	DUMP_REG(cap, XHCI_CAP_CSZ);
	DUMP_REG(cap, XHCI_CAP_PPC);
	DUMP_REG(cap, XHCI_CAP_PIND);
	DUMP_REG(cap, XHCI_CAP_C);
	DUMP_REG(cap, XHCI_CAP_LTC);
	DUMP_REG(cap, XHCI_CAP_NSS);
	DUMP_REG(cap, XHCI_CAP_PAE);
	DUMP_REG(cap, XHCI_CAP_SPC);
	DUMP_REG(cap, XHCI_CAP_SEC);
	DUMP_REG(cap, XHCI_CAP_CFC);
	DUMP_REG(cap, XHCI_CAP_MAX_PSA_SIZE);
	DUMP_REG(cap, XHCI_CAP_XECP);
	DUMP_REG(cap, XHCI_CAP_DBOFF);
	DUMP_REG(cap, XHCI_CAP_RTSOFF);
	DUMP_REG(cap, XHCI_CAP_U3C);
	DUMP_REG(cap, XHCI_CAP_CMC);
	DUMP_REG(cap, XHCI_CAP_FSC);
	DUMP_REG(cap, XHCI_CAP_CTC);
	DUMP_REG(cap, XHCI_CAP_LEC);
	DUMP_REG(cap, XHCI_CAP_CIC);
}

/**
 * Dumps registers of one port.
 */
void xhci_dump_port(const xhci_port_regs_t *port)
{
	DUMP_REG(port, XHCI_PORT_CCS);
	DUMP_REG(port, XHCI_PORT_PED);
	DUMP_REG(port, XHCI_PORT_OCA);
	DUMP_REG(port, XHCI_PORT_PR);
	DUMP_REG(port, XHCI_PORT_PLS);
	DUMP_REG(port, XHCI_PORT_PP);
	DUMP_REG(port, XHCI_PORT_PS);
	DUMP_REG(port, XHCI_PORT_PIC);
	DUMP_REG(port, XHCI_PORT_LWS);
	DUMP_REG(port, XHCI_PORT_CSC);
	DUMP_REG(port, XHCI_PORT_PEC);
	DUMP_REG(port, XHCI_PORT_WRC);
	DUMP_REG(port, XHCI_PORT_OCC);
	DUMP_REG(port, XHCI_PORT_PRC);
	DUMP_REG(port, XHCI_PORT_PLC);
	DUMP_REG(port, XHCI_PORT_CEC);
	DUMP_REG(port, XHCI_PORT_CAS);
	DUMP_REG(port, XHCI_PORT_WCE);
	DUMP_REG(port, XHCI_PORT_WDE);
	DUMP_REG(port, XHCI_PORT_WOE);
	DUMP_REG(port, XHCI_PORT_DR);
	DUMP_REG(port, XHCI_PORT_WPR);
	DUMP_REG(port, XHCI_PORT_USB3_U1TO);
	DUMP_REG(port, XHCI_PORT_USB3_U2TO);
	DUMP_REG(port, XHCI_PORT_USB3_FLPMA);
	DUMP_REG(port, XHCI_PORT_USB3_LEC);
	DUMP_REG(port, XHCI_PORT_USB3_RLC);
	DUMP_REG(port, XHCI_PORT_USB3_TLC);
	DUMP_REG(port, XHCI_PORT_USB2_L1S);
	DUMP_REG(port, XHCI_PORT_USB2_RWE);
	DUMP_REG(port, XHCI_PORT_USB2_BESL);
	DUMP_REG(port, XHCI_PORT_USB2_L1DS);
	DUMP_REG(port, XHCI_PORT_USB2_HLE);
	DUMP_REG(port, XHCI_PORT_USB2_TM);
	DUMP_REG(port, XHCI_PORT_USB2_HIRDM);
	DUMP_REG(port, XHCI_PORT_USB2_L1TO);
	DUMP_REG(port, XHCI_PORT_USB2_BESLD);
}

/**
 * Dumps all registers that define state of the HC.
 */
void xhci_dump_state(const xhci_hc_t *hc)
{
	usb_log_debug("Operational registers:");

	DUMP_REG(hc->op_regs, XHCI_OP_RS);
	DUMP_REG(hc->op_regs, XHCI_OP_HCRST);
	DUMP_REG(hc->op_regs, XHCI_OP_INTE);
	DUMP_REG(hc->op_regs, XHCI_OP_HSEE);
	DUMP_REG(hc->op_regs, XHCI_OP_LHCRST);
	DUMP_REG(hc->op_regs, XHCI_OP_CSS);
	DUMP_REG(hc->op_regs, XHCI_OP_CRS);
	DUMP_REG(hc->op_regs, XHCI_OP_EWE);
	DUMP_REG(hc->op_regs, XHCI_OP_EU3S);
	DUMP_REG(hc->op_regs, XHCI_OP_CME);
	DUMP_REG(hc->op_regs, XHCI_OP_HCH);
	DUMP_REG(hc->op_regs, XHCI_OP_HSE);
	DUMP_REG(hc->op_regs, XHCI_OP_EINT);
	DUMP_REG(hc->op_regs, XHCI_OP_PCD);
	DUMP_REG(hc->op_regs, XHCI_OP_SSS);
	DUMP_REG(hc->op_regs, XHCI_OP_RSS);
	DUMP_REG(hc->op_regs, XHCI_OP_SRE);
	DUMP_REG(hc->op_regs, XHCI_OP_CNR);
	DUMP_REG(hc->op_regs, XHCI_OP_HCE);
	DUMP_REG(hc->op_regs, XHCI_OP_PAGESIZE);
	DUMP_REG(hc->op_regs, XHCI_OP_NOTIFICATION);
	DUMP_REG(hc->op_regs, XHCI_OP_RCS);
	DUMP_REG(hc->op_regs, XHCI_OP_CS);
	DUMP_REG(hc->op_regs, XHCI_OP_CA);
	DUMP_REG(hc->op_regs, XHCI_OP_CRR);
	DUMP_REG(hc->op_regs, XHCI_OP_CRCR);
	DUMP_REG(hc->op_regs, XHCI_OP_DCBAAP);
	DUMP_REG(hc->rt_regs, XHCI_RT_MFINDEX);

	usb_log_debug("Interrupter 0 state:");
	DUMP_REG(&hc->rt_regs->ir[0], XHCI_INTR_IP);
	DUMP_REG(&hc->rt_regs->ir[0], XHCI_INTR_IE);
	DUMP_REG(&hc->rt_regs->ir[0], XHCI_INTR_IMI);
	DUMP_REG(&hc->rt_regs->ir[0], XHCI_INTR_IMC);
	DUMP_REG(&hc->rt_regs->ir[0], XHCI_INTR_ERSTSZ);
	DUMP_REG(&hc->rt_regs->ir[0], XHCI_INTR_ERSTBA);
	DUMP_REG(&hc->rt_regs->ir[0], XHCI_INTR_ERDP);
}

/**
 * Dump registers of all ports.
 */
void xhci_dump_ports(const xhci_hc_t *hc)
{
	const size_t num_ports = XHCI_REG_RD(hc->cap_regs, XHCI_CAP_MAX_PORTS);
	for (size_t i = 0; i < num_ports; i++) {
		usb_log_debug("Port %zu state:", i);

		xhci_dump_port(&hc->op_regs->portrs[i]);
	}
}

static const char *trb_types [] = {
	[0] = "<empty>",
#define TRB(t) [XHCI_TRB_TYPE_##t] = #t
	TRB(NORMAL),
	TRB(SETUP_STAGE),
	TRB(DATA_STAGE),
	TRB(STATUS_STAGE),
	TRB(ISOCH),
	TRB(LINK),
	TRB(EVENT_DATA),
	TRB(NO_OP),
	TRB(ENABLE_SLOT_CMD),
	TRB(DISABLE_SLOT_CMD),
	TRB(ADDRESS_DEVICE_CMD),
	TRB(CONFIGURE_ENDPOINT_CMD),
	TRB(EVALUATE_CONTEXT_CMD),
	TRB(RESET_ENDPOINT_CMD),
	TRB(STOP_ENDPOINT_CMD),
	TRB(SET_TR_DEQUEUE_POINTER_CMD),
	TRB(RESET_DEVICE_CMD),
	TRB(FORCE_EVENT_CMD),
	TRB(NEGOTIATE_BANDWIDTH_CMD),
	TRB(SET_LATENCY_TOLERANCE_VALUE_CMD),
	TRB(GET_PORT_BANDWIDTH_CMD),
	TRB(FORCE_HEADER_CMD),
	TRB(NO_OP_CMD),
	TRB(TRANSFER_EVENT),
	TRB(COMMAND_COMPLETION_EVENT),
	TRB(PORT_STATUS_CHANGE_EVENT),
	TRB(BANDWIDTH_REQUEST_EVENT),
	TRB(DOORBELL_EVENT),
	TRB(HOST_CONTROLLER_EVENT),
	TRB(DEVICE_NOTIFICATION_EVENT),
	TRB(MFINDEX_WRAP_EVENT),
#undef TRB
	[XHCI_TRB_TYPE_MAX] = NULL,
};

/**
 * Stringify XHCI_TRB_TYPE_*.
 */
const char *xhci_trb_str_type(unsigned type)
{
	static char type_buf [20];

	if (type < XHCI_TRB_TYPE_MAX && trb_types[type] != NULL)
		return trb_types[type];

	snprintf(type_buf, sizeof(type_buf), "<unknown (%u)>", type);
	return type_buf;
}

/**
 * Dump a TRB.
 */
void xhci_dump_trb(const xhci_trb_t *trb)
{
	usb_log_debug("TRB(%p): type %s, cycle %u, status 0x%#08" PRIx32 ", "
	    "parameter 0x%#016" PRIx64, trb, xhci_trb_str_type(TRB_TYPE(*trb)),
	    TRB_CYCLE(*trb), trb->status, trb->parameter);
}

static const char *ec_ids [] = {
	[0] = "<empty>",
#define EC(t) [XHCI_EC_##t] = #t
	EC(USB_LEGACY),
	EC(SUPPORTED_PROTOCOL),
	EC(EXTENDED_POWER_MANAGEMENT),
	EC(IOV),
	EC(MSI),
	EC(LOCALMEM),
	EC(DEBUG),
	EC(MSIX),
#undef EC
	[XHCI_EC_MAX] = NULL
};

/**
 * Dump Extended Capability ID.
 */
const char *xhci_ec_str_id(unsigned id)
{
	static char buf [20];

	if (id < XHCI_EC_MAX && ec_ids[id] != NULL)
		return ec_ids[id];

	snprintf(buf, sizeof(buf), "<unknown (%u)>", id);
	return buf;
}

/**
 * Dump Protocol Speed ID.
 */
static void xhci_dump_psi(const xhci_psi_t *psi)
{
	static const char speed_exp [] = " KMG";
	static const char *psi_types [] = { "", " rsvd", " RX", " TX" };

	usb_log_debug("Speed %u%s: %5u %cb/s, %s",
	    XHCI_REG_RD(psi, XHCI_PSI_PSIV),
	    psi_types[XHCI_REG_RD(psi, XHCI_PSI_PLT)],
	    XHCI_REG_RD(psi, XHCI_PSI_PSIM),
	    speed_exp[XHCI_REG_RD(psi, XHCI_PSI_PSIE)],
	    XHCI_REG_RD(psi, XHCI_PSI_PFD) ? "full-duplex" : "");
}

/**
 * Dump given Extended Capability.
 */
void xhci_dump_extcap(const xhci_extcap_t *ec)
{
	xhci_sp_name_t name;
	unsigned ports_from, ports_to;

	unsigned id = XHCI_REG_RD(ec, XHCI_EC_CAP_ID);
	usb_log_debug("Extended capability %s", xhci_ec_str_id(id));

	switch (id) {
	case XHCI_EC_SUPPORTED_PROTOCOL:
		name.packed = host2uint32_t_le(XHCI_REG_RD(ec, XHCI_EC_SP_NAME));
		ports_from = XHCI_REG_RD(ec, XHCI_EC_SP_CP_OFF);
		ports_to = ports_from + XHCI_REG_RD(ec, XHCI_EC_SP_CP_COUNT) - 1;
		unsigned psic = XHCI_REG_RD(ec, XHCI_EC_SP_PSIC);

		usb_log_debug("\tProtocol %.4s%u.%u, ports %u-%u, "
		    "%u protocol speeds", name.str,
		    XHCI_REG_RD(ec, XHCI_EC_SP_MAJOR),
		    XHCI_REG_RD(ec, XHCI_EC_SP_MINOR),
		    ports_from, ports_to, psic);

		for (unsigned i = 0; i < psic; i++)
			xhci_dump_psi(xhci_extcap_psi(ec, i));
		break;
	}
}

void xhci_dump_slot_ctx(const struct xhci_slot_ctx *ctx)
{
#define SLOT_DUMP(name)	usb_log_debug("\t" #name ":\t0x%x", XHCI_SLOT_##name(*ctx))
	SLOT_DUMP(ROUTE_STRING);
	SLOT_DUMP(SPEED);
	SLOT_DUMP(MTT);
	SLOT_DUMP(HUB);
	SLOT_DUMP(CTX_ENTRIES);
	SLOT_DUMP(MAX_EXIT_LATENCY);
	SLOT_DUMP(ROOT_HUB_PORT);
	SLOT_DUMP(NUM_PORTS);
	SLOT_DUMP(TT_HUB_SLOT_ID);
	SLOT_DUMP(TT_PORT_NUM);
	SLOT_DUMP(TT_THINK_TIME);
	SLOT_DUMP(INTERRUPTER);
	SLOT_DUMP(DEVICE_ADDRESS);
	SLOT_DUMP(STATE);
#undef SLOT_DUMP
}

void xhci_dump_endpoint_ctx(const struct xhci_endpoint_ctx *ctx)
{
#define EP_DUMP_DW(name)	usb_log_debug("\t" #name ":\t0x%x", XHCI_EP_##name(*ctx))
#define EP_DUMP_QW(name)	usb_log_debug("\t" #name ":\t0x%" PRIx64, XHCI_EP_##name(*ctx))
	EP_DUMP_DW(STATE);
	EP_DUMP_DW(MULT);
	EP_DUMP_DW(MAX_P_STREAMS);
	EP_DUMP_DW(LSA);
	EP_DUMP_DW(INTERVAL);
	EP_DUMP_DW(ERROR_COUNT);
	EP_DUMP_DW(TYPE);
	EP_DUMP_DW(HID);
	EP_DUMP_DW(MAX_BURST_SIZE);
	EP_DUMP_DW(MAX_PACKET_SIZE);
	EP_DUMP_QW(DCS);
	EP_DUMP_QW(TR_DPTR);
	EP_DUMP_DW(MAX_ESIT_PAYLOAD_LO);
	EP_DUMP_DW(MAX_ESIT_PAYLOAD_HI);
#undef EP_DUMP_DW
#undef EP_DUMP_QW
}

void xhci_dump_input_ctx(const xhci_hc_t *hc, const struct xhci_input_ctx *ictx)
{
	xhci_device_ctx_t *device_ctx = XHCI_GET_DEVICE_CTX(ictx, hc);
	xhci_slot_ctx_t *slot_ctx = XHCI_GET_SLOT_CTX(device_ctx, hc);
	xhci_input_ctrl_ctx_t *ctrl_ctx = XHCI_GET_CTRL_CTX(ictx, hc);

	usb_log_debug("Input control context:");
	usb_log_debug("\tDrop:\t0x%08x", xhci2host(32, ctrl_ctx->data[0]));
	usb_log_debug("\tAdd:\t0x%08x", xhci2host(32, ctrl_ctx->data[1]));

	usb_log_debug("\tConfig:\t0x%02x", XHCI_INPUT_CTRL_CTX_CONFIG_VALUE(*ctrl_ctx));
	usb_log_debug("\tIface:\t0x%02x", XHCI_INPUT_CTRL_CTX_IFACE_NUMBER(*ctrl_ctx));
	usb_log_debug("\tAlternate:\t0x%02x", XHCI_INPUT_CTRL_CTX_ALTER_SETTING(*ctrl_ctx));

	usb_log_debug("Slot context:");
	xhci_dump_slot_ctx(slot_ctx);

	for (uint8_t dci = 1; dci <= XHCI_EP_COUNT; dci++)
		if (XHCI_INPUT_CTRL_CTX_DROP(*ctrl_ctx, dci) ||
		    XHCI_INPUT_CTRL_CTX_ADD(*ctrl_ctx, dci)) {
			usb_log_debug("Endpoint context DCI %u:", dci);
			xhci_ep_ctx_t *ep_ctx = XHCI_GET_EP_CTX(device_ctx, hc, dci);
			xhci_dump_endpoint_ctx(ep_ctx);
		}
}

/**
 * @}
 */
