/*
 * Copyright (c) 2017 Ondrej Hlavaty
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
 */

#ifndef XHCI_REGS_H
#define XHCI_REGS_H

#include <macros.h>
#include <ddi.h>
#include "common.h"

/*
 * The macros XHCI_REG_* might seem a bit magic. It is the most compact way to
 * provide flexible interface abstracting from the real storage of given
 * register, but to avoid having to specify several constants per register.
 */

#define XHCI_PIO_CHANGE_UDELAY 5

#define host2xhci(size, val) host2uint##size##_t_le((val))
#define xhci2host(size, val) uint##size##_t_le2host((val))

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

/*
 * Field handling is the easiest. Just do it with whole field.
 */
#define XHCI_REG_RD_FIELD(ptr, size)         xhci2host(size, pio_read_##size((ptr)))
#define XHCI_REG_WR_FIELD(ptr, value, size)  pio_write_##size((ptr), host2xhci(size, value))
#define XHCI_REG_SET_FIELD(ptr, value, size) pio_set_##size((ptr), host2xhci(size, value), XHCI_PIO_CHANGE_UDELAY);
#define XHCI_REG_CLR_FIELD(ptr, value, size) pio_clear_##size((ptr), host2xhci(size, value), XHCI_PIO_CHANGE_UDELAY);

/*
 * Flags are just trivial case of ranges.
 */
#define XHCI_REG_RD_FLAG(ptr, size, offset)         XHCI_REG_RD_RANGE((ptr), size, (offset), (offset))
#define XHCI_REG_WR_FLAG(ptr, value, size, offset)  XHCI_REG_WR_RANGE((ptr), (value), size, (offset), (offset))
#define XHCI_REG_SET_FLAG(ptr, value, size, offset) XHCI_REG_SET_RANGE((ptr), (value), size, (offset), (offset))
#define XHCI_REG_CLR_FLAG(ptr, value, size, offset) XHCI_REG_CLR_RANGE((ptr), (value), size, (offset), (offset))

/*
 * Ranges are the most difficult. We need to play around with bitmasks.
 */
#define XHCI_REG_RD_RANGE(ptr, size, hi, lo) \
	BIT_RANGE_EXTRACT(uint##size##_t, (hi), (lo), XHCI_REG_RD_FIELD((ptr), size))

#define XHCI_REG_WR_RANGE(ptr, value, size, hi, lo) \
	pio_change_##size((ptr), host2xhci(size, BIT_RANGE_INSERT(uint##size##_t, (hi), (lo), (value))), \
		host2xhci(size, BIT_RANGE(uint##size##_t, (hi), (lo))), \
		XHCI_PIO_CHANGE_UDELAY);

#define XHCI_REG_SET_RANGE(ptr, value, size, hi, lo) \
	pio_set_##size((ptr), host2xhci(size, BIT_RANGE_INSERT(uint##size##_t, (hi), (lo), (value))), \
		XHCI_PIO_CHANGE_UDELAY);

#define XHCI_REG_CLR_RANGE(ptr, value, size, hi, lo) \
	pio_clear_##size((ptr), host2xhci(size, BIT_RANGE_INSERT(uint##size##_t, (hi), (lo), (value))), \
		XHCI_PIO_CHANGE_UDELAY);

/** HC capability registers: section 5.3 */
typedef const struct xhci_cap_regs {

	/* Size of this structure, offset for the operation registers */
	const ioport8_t caplength;

	const PADD8;

	/* BCD of specification version */
	const ioport16_t hciversion;

	/*
	 *  7:0  - MaxSlots
	 * 18:8  - MaxIntrs
	 * 31:24 - MaxPorts
	 */
	const ioport32_t hcsparams1;

	/*
	 *  0:3  - IST
	 *  7:4  - ERST Max
	 * 21:25 - Max Scratchpad Bufs Hi
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
#define XHCI_CAP_SPR          hcsparams2, 32, RANGE, 26, 26
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
#define XHCI_CAP_MAX_PSA_SIZE hccparams1, 32,  FLAG, 12
#define XHCI_CAP_XECP         hccparams1, 32,  FLAG, 13
#define XHCI_CAP_DBOFF             dboff, 32, FIELD
#define XHCI_CAP_RTSOFF           rtsoff, 32, FIELD
#define XHCI_CAP_U3C          hccparams2, 32,  FLAG,  0
#define XHCI_CAP_CMC          hccparams2, 32,  FLAG,  1
#define XHCI_CAP_FSC          hccparams2, 32,  FLAG,  2
#define XHCI_CAP_CTC          hccparams2, 32,  FLAG,  3
#define XHCI_CAP_LEC          hccparams2, 32,  FLAG,  4
#define XHCI_CAP_CIC          hccparams2, 32,  FLAG,  5

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
#define XHCI_PORT_PIC           portsc, 32, RANGE, 13, 10
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
#define XHCI_PORT_DR            portsc, 32,  FLAG, 28
#define XHCI_PORT_WPR           portsc, 32,  FLAG, 29

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
#define XHCI_PORT_USB2_HIRDM porthlmpc, 32, RANGE,  1,  0
#define XHCI_PORT_USB2_L1TO  porthlmpc, 32, RANGE,  9,  2
#define XHCI_PORT_USB2_BESLD porthlmpc, 32, RANGE, 13, 10

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

	PADD32[2];

	/*
	 * 15:0 - Notification enable
	 */
	ioport32_t dnctrl;

	/*          3  2  1   0
	 *  3:0 - CRR CA CS RCS
	 * 64:6 - Command Ring Pointer
	 */
	ioport32_t crcr_lo;
	ioport32_t crcr_hi;

	PADD32[4];

	ioport32_t dcbaap_lo;
	ioport32_t dcbaap_hi;

	/*
	 * 7:0 - MaxSlotsEn
	 *   8 - U3E
	 *   9 - CIE
	 */
	ioport32_t config;

	PADD32[36 * 4 + 1];

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
#define XHCI_OP_RCS            crcr_lo, 32,  FLAG, 0
#define XHCI_OP_CS             crcr_lo, 32,  FLAG, 1
#define XHCI_OP_CA             crcr_lo, 32,  FLAG, 2
#define XHCI_OP_CRR            crcr_lo, 32,  FLAG, 3
#define XHCI_OP_CRCR_LO        crcr_lo, 32, RANGE, 31, 6
#define XHCI_OP_CRCR_HI        crcr_lo, 32, FIELD

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

	PADD32;

	ioport32_t erstba_lo;
	ioport32_t erstba_hi;

	/*
	 *  2:0 - Dequeue ERST Segment Index
	 *    3 - Event Handler Busy
	 * 63:4 - Event Ring Dequeue Pointer
	 */
	ioport32_t erdp_lo;
	ioport32_t erdp_hi;
} xhci_interrupter_regs_t;

#define XHCI_INTR_IP              iman, 32,  FLAG,  0
#define XHCI_INTR_IE              iman, 32,  FLAG,  1
#define XHCI_INTR_IMI             imod, 32, RANGE, 15, 0
#define XHCI_INTR_IMC             imod, 32, RANGE, 31, 16
#define XHCI_INTR_ERSTSZ        erstsz, 32, FIELD
#define XHCI_INTR_ERSTBA_LO  erstba_lo, 32, FIELD
#define XHCI_INTR_ERSTBA_HI  erstba_hi, 32, FIELD
#define XHCI_INTR_ERDP_LO      erdp_lo, 32, FIELD
#define XHCI_INTR_ERDP_HI      erdp_hi, 32, FIELD

/**
 * XHCI Runtime registers: section 5.5
 */
typedef struct xhci_rt_regs {
	ioport32_t mfindex;

	PADD32 [5];

	xhci_interrupter_regs_t ir[1024];
} xhci_rt_regs_t;

#define XHCI_RT_MFINDEX        mfindex, 32, FIELD

#endif
/**
 * @}
 */
