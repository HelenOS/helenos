/*
 * Copyright (c) 2018 Ondrej Hlavaty
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
 * Memory-mapped register structures of the xHC.
 *
 * The main pr
 */

#ifndef XHCI_REGS_H
#define XHCI_REGS_H

#include <macros.h>
#include <ddi.h>
#include "common.h"

#define XHCI_PIO_CHANGE_UDELAY 5

/*
 * These four are the main macros to be used.
 * Semantics is usual - READ reads value, WRITE changes value, SET sets
 * selected bits, CLEAR clears selected bits to 0.
 *
 * The key thing here is the order of macro expansion, expanding the reg_spec
 * argument as more arguments (comma delimited) for the inner macro.
 */
#define XHCI_REG_RD(reg_set, reg_spec)         XHCI_REG_RD_INNER(reg_set, reg_spec)
#define XHCI_REG_WR(reg_set, reg_spec, value)  XHCI_REG_WR_INNER(reg_set, value, reg_spec)
#define XHCI_REG_SET(reg_set, reg_spec, value) XHCI_REG_SET_INNER(reg_set, value, reg_spec)
#define XHCI_REG_CLR(reg_set, reg_spec, value) XHCI_REG_CLR_INNER(reg_set, value, reg_spec)
#define XHCI_REG_MASK(reg_spec)                XHCI_REG_MASK_INNER(reg_spec)
#define XHCI_REG_SHIFT(reg_spec)               XHCI_REG_SHIFT_INNER(reg_spec)

/*
 * These take a pointer to the field, and selects the type-specific macro.
 */
#define XHCI_REG_RD_INNER(reg_set, field, size, type, ...) \
	XHCI_REG_RD_##type(&((reg_set)->field), size, ##__VA_ARGS__)

#define XHCI_REG_WR_INNER(reg_set, value, field, size, type, ...) \
	XHCI_REG_WR_##type(&(reg_set)->field, value, size, ##__VA_ARGS__)

#define XHCI_REG_SET_INNER(reg_set, value, field, size, type, ...) \
	XHCI_REG_SET_##type(&(reg_set)->field, value, size, ##__VA_ARGS__)

#define XHCI_REG_CLR_INNER(reg_set, value, field, size, type, ...) \
	XHCI_REG_CLR_##type(&(reg_set)->field, value, size, ##__VA_ARGS__)

#define XHCI_REG_MASK_INNER(field, size, type, ...) \
	XHCI_REG_MASK_##type(size, ##__VA_ARGS__)

#define XHCI_REG_SHIFT_INNER(field, size, type, ...) \
	XHCI_REG_SHIFT_##type(size, ##__VA_ARGS__)

/*
 * Field handling is the easiest. Just do it with whole field.
 */
#define XHCI_REG_RD_FIELD(ptr, size) \
	xhci2host(size, pio_read_##size((ptr)))
#define XHCI_REG_WR_FIELD(ptr, value, size) \
	pio_write_##size((ptr), host2xhci(size, value))
#define XHCI_REG_SET_FIELD(ptr, value, size) \
	pio_set_##size((ptr), host2xhci(size, value), XHCI_PIO_CHANGE_UDELAY);
#define XHCI_REG_CLR_FIELD(ptr, value, size) \
	pio_clear_##size((ptr), host2xhci(size, value), XHCI_PIO_CHANGE_UDELAY);
#define XHCI_REG_MASK_FIELD(size)            (~((uint##size##_t) 0))
#define XHCI_REG_SHIFT_FIELD(size)           (0)

/*
 * Flags are just trivial case of ranges.
 */
#define XHCI_REG_RD_FLAG(ptr, size, offset) \
	XHCI_REG_RD_RANGE((ptr), size, (offset), (offset))
#define XHCI_REG_WR_FLAG(ptr, value, size, offset) \
	XHCI_REG_WR_RANGE((ptr), (value), size, (offset), (offset))
#define XHCI_REG_SET_FLAG(ptr, value, size, offset) \
	XHCI_REG_SET_RANGE((ptr), (value), size, (offset), (offset))
#define XHCI_REG_CLR_FLAG(ptr, value, size, offset) \
	XHCI_REG_CLR_RANGE((ptr), (value), size, (offset), (offset))
#define XHCI_REG_MASK_FLAG(size, offset)            BIT_V(uint##size##_t, offset)
#define XHCI_REG_SHIFT_FLAG(size, offset)           (offset)

/*
 * Ranges are the most difficult. We need to play around with bitmasks.
 */
#define XHCI_REG_RD_RANGE(ptr, size, hi, lo) \
	BIT_RANGE_EXTRACT(uint##size##_t, (hi), (lo), XHCI_REG_RD_FIELD((ptr), size))

#define XHCI_REG_WR_RANGE(ptr, value, size, hi, lo) \
	pio_change_##size((ptr), host2xhci(size, BIT_RANGE_INSERT(uint##size##_t, \
			(hi), (lo), (value))), \
		host2xhci(size, BIT_RANGE(uint##size##_t, (hi), (lo))), \
		XHCI_PIO_CHANGE_UDELAY);

#define XHCI_REG_SET_RANGE(ptr, value, size, hi, lo) \
	pio_set_##size((ptr), host2xhci(size, BIT_RANGE_INSERT(uint##size##_t, \
			(hi), (lo), (value))), \
		XHCI_PIO_CHANGE_UDELAY);

#define XHCI_REG_CLR_RANGE(ptr, value, size, hi, lo) \
	pio_clear_##size((ptr), host2xhci(size, BIT_RANGE_INSERT(uint##size##_t, \
			(hi), (lo), (value))), \
		XHCI_PIO_CHANGE_UDELAY);

#define XHCI_REG_MASK_RANGE(size, hi, lo)  BIT_RANGE(uint##size##_t, hi, lo)
#define XHCI_REG_SHIFT_RANGE(size, hi, lo) (lo)

/** HC capability registers: section 5.3 */
typedef const struct xhci_cap_regs {

	/* Size of this structure, offset for the operation registers */
	const ioport8_t caplength;

	const PADD8(1);

	/* BCD of specification version */
	const ioport16_t hciversion;

	/*
	 *  7:0  - MaxSlots
	 * 18:8  - MaxIntrs
	 * 31:24 - MaxPorts
	 */
	const ioport32_t hcsparams1;

	/*
	 *  3:0  - IST
	 *  7:4  - ERST Max
	 * 25:21 - Max Scratchpad Bufs Hi
	 *    26 - SPR
	 * 31:27 - Max Scratchpad Bufs Lo
	 */
	const ioport32_t hcsparams2;

	/*
	 *  7:0  - U1 Device Exit Latency
	 * 31:16 - U2 Device Exit Latency
	 */
	const ioport32_t hcsparams3;

	/*
	 *          11  10   9   8   7   6 5    4   3   2   1    0
	 * 11:0  - CFC SEC SPC PAE NSS LTC C PIND PPC CSZ BNC AC64
	 * 15:12 - MaxPSASize
	 * 31:16 - xECP
	 */
	const ioport32_t hccparams1;

	/*
	 * 31:2 - Doorbell Array Offset
	 */
	const ioport32_t dboff;

	/*
	 * 31:5 - Runtime Register Space Offset
	 */
	const ioport32_t rtsoff;

	/*
	 *                 5   4   3   2   1   0
	 * 5:0  - Flags: CIC LEC CTC FSC CMC U3C
	 */
	const ioport32_t hccparams2;

	// the rest to operational registers is reserved
} xhci_cap_regs_t;

/*
 * The register specifiers are to be used as the reg_spec argument.
 *
 * The values are field, bitsize, type, (type specific args)
 * When the type is RANGE: hi, lo
 */
#define XHCI_CAP_LENGTH        caplength,  8, FIELD
#define XHCI_CAP_VERSION      hciversion, 16, FIELD
#define XHCI_CAP_MAX_SLOTS    hcsparams1, 32, RANGE,  7,  0
#define XHCI_CAP_MAX_INTRS    hcsparams1, 32, RANGE, 18,  8
#define XHCI_CAP_MAX_PORTS    hcsparams1, 32, RANGE, 31, 24
#define XHCI_CAP_IST          hcsparams2, 32, RANGE,  3,  0
#define XHCI_CAP_ERST_MAX     hcsparams2, 32, RANGE,  7,  4
#define XHCI_CAP_MAX_SPBUF_HI hcsparams2, 32, RANGE, 25, 21
#define XHCI_CAP_SPR          hcsparams2, 32,  FLAG, 26
#define XHCI_CAP_MAX_SPBUF_LO hcsparams2, 32, RANGE, 31, 27
#define XHCI_CAP_U1EL         hcsparams3, 32, RANGE,  7,  0
#define XHCI_CAP_U2EL         hcsparams3, 32, RANGE, 31, 16
#define XHCI_CAP_AC64         hccparams1, 32,  FLAG,  0
#define XHCI_CAP_BNC          hccparams1, 32,  FLAG,  1
#define XHCI_CAP_CSZ          hccparams1, 32,  FLAG,  2
#define XHCI_CAP_PPC          hccparams1, 32,  FLAG,  3
#define XHCI_CAP_PIND         hccparams1, 32,  FLAG,  4
#define XHCI_CAP_C            hccparams1, 32,  FLAG,  5
#define XHCI_CAP_LTC          hccparams1, 32,  FLAG,  6
#define XHCI_CAP_NSS          hccparams1, 32,  FLAG,  7
#define XHCI_CAP_PAE          hccparams1, 32,  FLAG,  8
#define XHCI_CAP_SPC          hccparams1, 32,  FLAG,  9
#define XHCI_CAP_SEC          hccparams1, 32,  FLAG, 10
#define XHCI_CAP_CFC          hccparams1, 32,  FLAG, 11
#define XHCI_CAP_MAX_PSA_SIZE hccparams1, 32, RANGE, 15, 12
#define XHCI_CAP_XECP         hccparams1, 32, RANGE, 31, 16
#define XHCI_CAP_DBOFF             dboff, 32, FIELD
#define XHCI_CAP_RTSOFF           rtsoff, 32, FIELD
#define XHCI_CAP_U3C          hccparams2, 32,  FLAG,  0
#define XHCI_CAP_CMC          hccparams2, 32,  FLAG,  1
#define XHCI_CAP_FSC          hccparams2, 32,  FLAG,  2
#define XHCI_CAP_CTC          hccparams2, 32,  FLAG,  3
#define XHCI_CAP_LEC          hccparams2, 32,  FLAG,  4
#define XHCI_CAP_CIC          hccparams2, 32,  FLAG,  5

static inline unsigned xhci_get_max_spbuf(xhci_cap_regs_t *cap_regs)
{
	return XHCI_REG_RD(cap_regs, XHCI_CAP_MAX_SPBUF_HI) << 5 |
	    XHCI_REG_RD(cap_regs, XHCI_CAP_MAX_SPBUF_LO);
}

/**
 * XHCI Port Register Set: section 5.4, table 32
 */
typedef struct xhci_port_regs {
	/*
	 *          4   3 2   1   0
	 *  4:0  - PR OCA Z PED CCS
	 *  8:5  - PLS
	 *    9  - PP
	 * 13:10 - Port Speed
	 * 15:14 - PIC
	 *          27  26  25  24  23  22  21  20  19  18  17  16
	 * 27:16 - WOE WDE WCE CAS CEC PLC PRC OCC WRC PEC CSC LWS
	 *    30 - DR
	 *    31 - WPR
	 */
	ioport32_t portsc;

	/*
	 * Contents of this fields depends on the protocol supported by the port.
	 * USB3:
	 *      7:0  - U1 Timeout
	 *     15:8  - U2 Timeout
	 *        16 - Force Link PM Accept
	 * USB2:
	 *      2:0  - L1S
	 *        3  - RWE
	 *      7:4  - BESL
	 *     15:8  - L1 Device Slot
	 *        16 - HLE
	 *     31:28 - Test Mode
	 */
	ioport32_t portpmsc;

	/*
	 * This field is valid only for USB3 ports.
	 * 15:0  - Link Error Count
	 * 19:16 - RLC
	 * 23:20 - TLC
	 */
	ioport32_t portli;

	/*
	 * This field is valid only for USB2 ports.
	 *  1:0  - HIRDM
	 *  9:2  - L1 Timeout
	 * 13:10 - BESLD
	 */
	ioport32_t porthlpmc;
} xhci_port_regs_t;

#define XHCI_PORT_CCS           portsc, 32,  FLAG,  0
#define XHCI_PORT_PED           portsc, 32,  FLAG,  1
#define XHCI_PORT_OCA           portsc, 32,  FLAG,  3
#define XHCI_PORT_PR            portsc, 32,  FLAG,  4
#define XHCI_PORT_PLS           portsc, 32, RANGE,  8,  5
#define XHCI_PORT_PP            portsc, 32,  FLAG,  9
#define XHCI_PORT_PS            portsc, 32, RANGE, 13, 10
#define XHCI_PORT_PIC           portsc, 32, RANGE, 15, 14
#define XHCI_PORT_LWS           portsc, 32,  FLAG, 16
#define XHCI_PORT_CSC           portsc, 32,  FLAG, 17
#define XHCI_PORT_PEC           portsc, 32,  FLAG, 18
#define XHCI_PORT_WRC           portsc, 32,  FLAG, 19
#define XHCI_PORT_OCC           portsc, 32,  FLAG, 20
#define XHCI_PORT_PRC           portsc, 32,  FLAG, 21
#define XHCI_PORT_PLC           portsc, 32,  FLAG, 22
#define XHCI_PORT_CEC           portsc, 32,  FLAG, 23
#define XHCI_PORT_CAS           portsc, 32,  FLAG, 24
#define XHCI_PORT_WCE           portsc, 32,  FLAG, 25
#define XHCI_PORT_WDE           portsc, 32,  FLAG, 26
#define XHCI_PORT_WOE           portsc, 32,  FLAG, 27
#define XHCI_PORT_DR            portsc, 32,  FLAG, 30
#define XHCI_PORT_WPR           portsc, 32,  FLAG, 31

#define XHCI_PORT_USB3_U1TO   portpmsc, 32, RANGE,  7,  0
#define XHCI_PORT_USB3_U2TO   portpmsc, 32, RANGE, 15,  8
#define XHCI_PORT_USB3_FLPMA  portpmsc, 32,  FLAG, 16
#define XHCI_PORT_USB3_LEC      portli, 32, RANGE, 15,  0
#define XHCI_PORT_USB3_RLC      portli, 32, RANGE, 19, 16
#define XHCI_PORT_USB3_TLC      portli, 32, RANGE, 23, 20

#define XHCI_PORT_USB2_L1S    portpmsc, 32, RANGE,  2,  0
#define XHCI_PORT_USB2_RWE    portpmsc, 32,  FLAG,  3
#define XHCI_PORT_USB2_BESL   portpmsc, 32, RANGE,  7,  4
#define XHCI_PORT_USB2_L1DS   portpmsc, 32, RANGE, 15,  8
#define XHCI_PORT_USB2_HLE    portpmsc, 32,  FLAG, 16
#define XHCI_PORT_USB2_TM     portpmsc, 32, RANGE, 31, 28
#define XHCI_PORT_USB2_HIRDM porthlpmc, 32, RANGE,  1,  0
#define XHCI_PORT_USB2_L1TO  porthlpmc, 32, RANGE,  9,  2
#define XHCI_PORT_USB2_BESLD porthlpmc, 32, RANGE, 13, 10

/**
 * XHCI Operational Registers: section 5.4
 */
typedef struct xhci_op_regs {

	/*
	 *            3    2     1   0
	 *  3:0  - HSEE INTE HCRST R/S
	 *
	 *           11  10   9   8      7
	 * 11:7  - EU3S EWE CRS CSS LHCRST
	 *    13 - CME
	 */
	ioport32_t usbcmd;

	/*
	 *          4    3   2 1   0
	 *  4:0 - PCD EINT HSE _ HCH
	 *
	 *        12  11  10   9   8
	 * 12:8 - HCE CNR SRE RSS SSS
	 */
	ioport32_t usbsts;

	/*
	 * Bitmask of page sizes supported: 128M .. 4K
	 */
	ioport32_t pagesize;

	PADD32(2);

	/*
	 * 15:0 - Notification enable
	 */
	ioport32_t dnctrl;

	/*
	 *          3  2  1   0
	 *  3:0 - CRR CA CS RCS
	 * 64:6 - Command Ring Pointer
	 */
	ioport64_t crcr;

	PADD32(4);

	ioport64_t dcbaap;

	/*
	 * 7:0 - MaxSlotsEn
	 *   8 - U3E
	 *   9 - CIE
	 */
	ioport32_t config;

	/* Offset of portrs from op_regs addr is 0x400. */
	PADD32(241);

	/*
	 * Individual ports register sets
	 */
	xhci_port_regs_t portrs[256];
} xhci_op_regs_t;

#define XHCI_OP_RS              usbcmd, 32,  FLAG,  0
#define XHCI_OP_HCRST           usbcmd, 32,  FLAG,  1
#define XHCI_OP_INTE            usbcmd, 32,  FLAG,  2
#define XHCI_OP_HSEE            usbcmd, 32,  FLAG,  3
#define XHCI_OP_LHCRST          usbcmd, 32,  FLAG,  7
#define XHCI_OP_CSS             usbcmd, 32,  FLAG,  8
#define XHCI_OP_CRS             usbcmd, 32,  FLAG,  9
#define XHCI_OP_EWE             usbcmd, 32,  FLAG, 10
#define XHCI_OP_EU3S            usbcmd, 32,  FLAG, 11
#define XHCI_OP_CME             usbcmd, 32,  FLAG, 13
#define XHCI_OP_HCH             usbsts, 32,  FLAG,  0
#define XHCI_OP_HSE             usbsts, 32,  FLAG,  2
#define XHCI_OP_EINT            usbsts, 32,  FLAG,  3
#define XHCI_OP_PCD             usbsts, 32,  FLAG,  4
#define XHCI_OP_SSS             usbsts, 32,  FLAG,  8
#define XHCI_OP_RSS             usbsts, 32,  FLAG,  9
#define XHCI_OP_SRE             usbsts, 32,  FLAG, 10
#define XHCI_OP_CNR             usbsts, 32,  FLAG, 11
#define XHCI_OP_HCE             usbsts, 32,  FLAG, 12
#define XHCI_OP_PAGESIZE      pagesize, 32, FIELD
#define XHCI_OP_NOTIFICATION    dnctrl, 32, RANGE, 15, 0
#define XHCI_OP_RCS               crcr, 64,  FLAG, 0
#define XHCI_OP_CS                crcr, 64,  FLAG, 1
#define XHCI_OP_CA                crcr, 64,  FLAG, 2
#define XHCI_OP_CRR               crcr, 64,  FLAG, 3
/*
 * This shall be RANGE, 6, 0, but the value containing CR pointer and RCS flag
 * must be written at once.
 */
#define XHCI_OP_CRCR              crcr, 64, FIELD
#define XHCI_OP_DCBAAP          dcbaap, 64, FIELD
#define XHCI_OP_MAX_SLOTS_EN    config, 32, RANGE, 7, 0
#define XHCI_OP_U3E             config, 32,  FLAG, 8
#define XHCI_OP_CIE             config, 32,  FLAG, 9

/* Aggregating field to read & write whole status at once */
#define XHCI_OP_STATUS          usbsts, 32, RANGE, 12, 0

/* RW1C fields in usbsts */
#define XHCI_STATUS_ACK_MASK     0x41C

/**
 * Interrupter Register Set: section 5.5.2
 */
typedef struct xhci_interrupter_regs {
	/*
	 * 0 - Interrupt Pending
	 * 1 - Interrupt Enable
	 */
	ioport32_t iman;

	/*
	 * 15:0  - Interrupt Moderation Interval
	 * 31:16 - Interrupt Moderation Counter
	 */
	ioport32_t imod;

	ioport32_t erstsz;

	PADD32(1);

	ioport64_t erstba;

	/*
	 *  2:0 - Dequeue ERST Segment Index
	 *    3 - Event Handler Busy
	 * 63:4 - Event Ring Dequeue Pointer
	 */
	ioport64_t erdp;
} xhci_interrupter_regs_t;

#define XHCI_INTR_IP              iman, 32,  FLAG,  0
#define XHCI_INTR_IE              iman, 32,  FLAG,  1
#define XHCI_INTR_IMI             imod, 32, RANGE, 15, 0
#define XHCI_INTR_IMC             imod, 32, RANGE, 31, 16
#define XHCI_INTR_ERSTSZ        erstsz, 32, FIELD
#define XHCI_INTR_ERSTBA        erstba, 64, FIELD
#define XHCI_INTR_ERDP_ESI        erdp, 64, RANGE,  2, 0
#define XHCI_INTR_ERDP_EHB        erdp, 64,  FLAG,  3
#define XHCI_INTR_ERDP            erdp, 64, FIELD

/**
 * XHCI Runtime registers: section 5.5
 */
typedef struct xhci_rt_regs {
	ioport32_t mfindex;

	PADD32(7);

	xhci_interrupter_regs_t ir [];
} xhci_rt_regs_t;

#define XHCI_RT_MFINDEX        mfindex, 32, RANGE, 13, 0
#define XHCI_MFINDEX_MAX	(1 << 14)

/**
 * XHCI Doorbell Registers: section 5.6
 *
 * These registers are to be written as a whole field.
 */
typedef ioport32_t xhci_doorbell_t;

enum xhci_plt {
	XHCI_PSI_PLT_SYMM,
	XHCI_PSI_PLT_RSVD,
	XHCI_PSI_PLT_RX,
	XHCI_PSI_PLT_TX
};

/**
 * Protocol speed ID: section 7.2.1
 */
typedef struct xhci_psi {
	xhci_dword_t psi;
} xhci_psi_t;

#define XHCI_PSI_PSIV    psi, 32, RANGE,  3,  0
#define XHCI_PSI_PSIE    psi, 32, RANGE,  5,  4
#define XHCI_PSI_PLT     psi, 32, RANGE,  7,  6
#define XHCI_PSI_PFD     psi, 32,  FLAG,  8
#define XHCI_PSI_PSIM    psi, 32, RANGE, 31, 16

enum xhci_extcap_type {
	XHCI_EC_RESERVED = 0,
	XHCI_EC_USB_LEGACY,
	XHCI_EC_SUPPORTED_PROTOCOL,
	XHCI_EC_EXTENDED_POWER_MANAGEMENT,
	XHCI_EC_IOV,
	XHCI_EC_MSI,
	XHCI_EC_LOCALMEM,
	XHCI_EC_DEBUG = 10,
	XHCI_EC_MSIX = 17,
	XHCI_EC_MAX = 255
};

/**
 * xHCI Extended Capability: section 7
 */
typedef struct xhci_extcap {
	xhci_dword_t header;
	xhci_dword_t cap_specific[];
} xhci_extcap_t;

#define XHCI_EC_CAP_ID                  header, 32, RANGE,  7,  0
#define XHCI_EC_SIZE                    header, 32, RANGE, 15,  8

/* Supported protocol */
#define XHCI_EC_SP_MINOR                header, 32, RANGE, 23, 16
#define XHCI_EC_SP_MAJOR                header, 32, RANGE, 31, 24
#define XHCI_EC_SP_NAME        cap_specific[0], 32, FIELD
#define XHCI_EC_SP_CP_OFF      cap_specific[1], 32, RANGE,  7,  0
#define XHCI_EC_SP_CP_COUNT    cap_specific[1], 32, RANGE, 15,  8
#define XHCI_EC_SP_PSIC        cap_specific[1], 32, RANGE, 31, 28
#define XHCI_EC_SP_SLOT_TYPE   cap_specific[2], 32, RANGE,  4,  0

typedef union {
	char str [4];
	uint32_t packed;
} xhci_sp_name_t;

static const xhci_sp_name_t xhci_name_usb = {
	.str = "USB "
};

static inline xhci_extcap_t *xhci_extcap_next(const xhci_extcap_t *cur)
{
	unsigned dword_offset = XHCI_REG_RD(cur, XHCI_EC_SIZE);
	if (!dword_offset)
		return NULL;
	return (xhci_extcap_t *) (((xhci_dword_t *) cur) + dword_offset);
}

static inline xhci_psi_t *xhci_extcap_psi(const xhci_extcap_t *ec, unsigned psid)
{
	assert(XHCI_REG_RD(ec, XHCI_EC_CAP_ID) == XHCI_EC_SUPPORTED_PROTOCOL);
	assert(XHCI_REG_RD(ec, XHCI_EC_SP_PSIC) > psid);

	unsigned dword_offset = 4 + psid;
	return (xhci_psi_t *) (((xhci_dword_t *) ec) + dword_offset);
}

/**
 * USB Legacy Support: section 7.1
 *
 * Legacy support have an exception from dword-access, because it needs to be
 * byte-accessed.
 */
typedef struct xhci_extcap_legsup {
	ioport8_t cap_id;
	ioport8_t size;			/**< Next Capability Pointer */
	ioport8_t sem_bios;
	ioport8_t sem_os;

	/** USB Legacy Support Control/Status - RW for BIOS, RO for OS */
	xhci_dword_t usblegctlsts;
} xhci_legsup_t;

#define XHCI_LEGSUP_SEM_BIOS	sem_bios, 8, FLAG, 0
#define XHCI_LEGSUP_SEM_OS	sem_os, 8, FLAG, 0

/* 4.22.1 BIOS may take up to 1 second to release the device */
#define XHCI_LEGSUP_BIOS_TIMEOUT_US   1000000
#define XHCI_LEGSUP_POLLING_DELAY_1MS    1000

#endif
/**
 * @}
 */
