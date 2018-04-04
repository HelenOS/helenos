/*
 * Copyright (c) 2014 Agnieszka Tabaka
 * Copyright (c) 2011 Jiri Michalec
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

/** @file rtl8169_defs.h
 *
 * Registers, bit positions and masks definition
 * of the RTL8169 network family cards
 */

#ifndef RTL8169_DEFS_H_
#define RTL8169_DEFS_H_

#include <stdint.h>
#include <ddi.h>

#define	PCI_VID_REALTEK		0x10ec
#define	PCI_VID_DLINK		0x1186

/** Size of RTL8169 registers address space */
#define RTL8169_IO_SIZE  256

#define	RTL8169_FRAME_MAX_LENGTH	1518

/** Registers of RTL8169 family card offsets from the memory address base */
enum rtl8169_registers {
	IDR0 = 0x00, /**< First MAC address bit, 6 1b registres sequence */
	MAC0 = IDR0, /**< Alias for IDR0 */
	/* 0x06 - 0x07 reserved */
	MAR0 = 0x08, /**< Multicast mask registers 8 1b registers sequence */
	DTCCR = 0x10, /**< Dump Tally Counter Command Register */
	TNPDS = 0x20, /**< Transmit Normal Priority Descriptors */
	THPDS = 0x28, /**< Transmit High Priority Descriptors */
	FLASH = 0x30, /**< Flash memory read/write register */
	ERBCR = 0x34, /**< Early Receive Byte Count Register */
	ERSR = 0x36, /**< Early Rx Status Register */
	CR = 0x37, /**< Command register, 1b */
	TPPOLL = 0x38, /**< Transmit Priority Polling Register */
	CBA = 0x3a, /**< Current buffer address, 2b */
	IMR = 0x3c, /**< Interrupt mask register, 2b */
	ISR = 0x3e, /**< Interrupt status register, 2b */
	TCR = 0x40, /**< Transmit (Tx) configuration register, 4b */
	RCR = 0x44, /**< Receive (Rx) configuration register, 4b */
	TCTR = 0x48, /**< Timer count register */
	MPC = 0x4c, /**< Missed packet count */
	CR9346 = 0x50, /**< 93C46 command register (locking of registers) */
	CONFIG0 = 0x51,	/**< Configuration register 0, 1b */
	CONFIG1 = 0x52,	/**< Configuration register 1, 1b */
	CONFIG2 = 0x53, /**< Configuration register 2, 1b */
	CONFIG3 = 0x54,	/**< Configuration register 3, 1b */
	CONFIG4 = 0x55, /**< Configuration register 4, 1b */
	CONFIG5 = 0x56,	/**< Configuration register 5, 1b */
	/* 0x57 reserved */
	TIMINT = 0x58, /**< Timer interrupt register, 4b */
	MULINT = 0x58, /**< Multiple interrupt select, 4b */
	/* 0x5e-0x5f reserved */
	PHYAR = 0x60, /**< PHY Access Register, 4b */
	TBICSR0 = 0x64,	/**< TBI Control and Status Register, 4b */
	TBI_ANAR = 0x58, /**< TBI Auto-Negotiation Advertisement Register, 2b */
	TBI_LPAR = 0x6a, /**< TBI Auto-Negotiation Link Partner Ability Register, 2b */
	PHYSTATUS = 0x6c, /**< PHY Status Register */
	/* 0x6d-0x83 reserved */
	WAKEUP0	 = 0x84, /**< Power Management Wakeup frame0, 8b */
	WAKEUP1	 = 0x8c, /**< Power Management Wakeup frame1, 8b */
	WAKEUP2LD = 0x94, /**< Power Management Wakeup frame2 low dword, 8b */
	WAKEUP2HD = 0x9c, /**< Power Management Wakeup frame2 high dword, 8b */
	WAKEUP3LD = 0xa4, /**< Power Management Wakeup frame3 low dword, 8b */
	WAKEUP3HD = 0xac, /**< Power Management Wakeup frame3 high dword, 8b */
	WAKEUP4LD = 0xb4, /**< Power Management Wakeup frame4 low dword, 8b */
	WAKEUP4HD = 0xbc, /**< Power Management Wakeup frame4 high dword, 8b */
	CRC0 = 0xc4, /**< 16-bit CRC of wakeup frame 0, 2b */
	CRC1 = 0xc6, /**< 16-bit CRC of wakeup frame 1, 2b */
	CRC2 = 0xc8, /**< 16-bit CRC of wakeup frame 2, 2b */
	CRC3 = 0xca, /**< 16-bit CRC of wakeup frame 3, 2b */
	CRC4 = 0xcc, /**< 16-bit CRC of wakeup frame 4, 2b */
	/* 0xce - 0xd9 reserved */
	RMS = 0xda, /**< Rx packet Maximum Size, 2b */
	/* 0xdc - 0xdf reserved */
	CCR = 0xe0, /**< C+ Command Register */
	/* 0xe2 - 0xe3 reserved */
	RDSAR = 0xe4, /**< Receive Descriptor Start Address Register, 8b */
	ETTHR = 0xec, /**< Early Transmit Threshold Register, 1b */
	/* 0xed - 0xef reserved */
	FER = 0xf0, /**< Function Event Register, 4b */
	FEMR = 0xf4, /**< Function Event Mask Register, 4b */
	FPSR = 0xf8, /**< Function Preset State Register, 4b */
	FFER = 0xfc, /**< Function Force Event Register, 4b */
};

enum rtl8169_mii_registers {
	MII_BMCR = 0x00,
	MII_BMSR = 0x01,
	MII_ANAR = 0x04,
};

/** Command register bits */
enum rtl8169_cr {
	CR_TE = (1 << 2), /**< Transmitter enable bit */
	CR_RE = (1 << 3), /**< Receiver enable bit */
	CR_RST = (1 << 4), /**< Reset -  set to 1 to force software reset */
};

/** Config1 register bits */
enum rtl8169_config1 {
	CONFIG1_LEDS_SHIFT = 6, /**< Shift of CONFIG1_LEDS bits */
	CONFIG1_LEDS_SIZE = 2, /**< Size of CONFIG1_LEDS bits */
	CONFIG1_DVRLOAD = (1 << 5), /**< Driver load */
	CONFIG1_LWACT = (1 << 4), /**< LWAKE active mode */
	CONFIG1_MEMMAP = (1 << 3), /**< Memory mapping  */
	CONFIG1_IOMAP = (1 << 2), /**< I/O space mapping */
	CONFIG1_VPD = (1 << 1), /**< Set to enable Vital Product Data */
	CONFIG1_PMEn = (1 << 0) /**< Power management enabled */
};

enum rtl8169_config2 {
	CONFIG2_AUXSTATUS = (1 << 4), /**< Auxiliary Power Present Status */
	CONFIG2_PCIBUSWIDTH = (1 << 3), /**< PCI Bus Width */
	CONFIG2_PCICLKF_SHIFT = 0,
	CONFIG2_PCICLKF_SIZE = 2
};

enum rtl8169_config3 {
	CONFIG3_GNT_SELECT = (1 << 7), /**< Gnt select */
	CONFIG3_PARM_EN = (1 << 6), /**< Parameter enabled (100MBit mode) */
	CONFIG3_MAGIC = (1 << 5), /**< WoL Magic frame enable */
	CONFIG3_LINK_UP = (1 << 4), /**< Wakeup if link is reestablished */
	CONFIG3_CLKRUN_EN = (1 << 2), /**< CLKRUN enabled */ /* TODO: check what does it mean */
	CONFIG3_FBTBEN = (1 << 0), /**< Fast back to back enabled */
};

enum rtl8169_config4 {
	CONFIG4_RxFIFOAutoClr = (1 << 7), /**< Automatic RxFIFO owerflow clear */
	CONFIG4_AnaOff = (1 << 6), /**< Analog poweroff */
	CONFIG4_LongWF = (1 << 5), /**< Long wakeup frame */
	CONFIG4_LWPME = (1 << 4), /**< LWAKE and PMEB assertion  */
	CONFIG4_LWPTN = (1 << 2), /**< LWake pattern */
	CONFIG4_PBWakeup = (1 << 0), /**< Preboot wakeup */
};

enum rtl8169_config5 {
	CONFIG5_BROADCAST_WAKEUP = (1 << 6), /**< Broadcast wakeup frame enable */
	CONFIG5_MULTICAST_WAKEUP = (1 << 5), /**< Multicast wakeup frame enable */
	CONFIG5_UNICAST_WAKEUP = (1 << 4), /**< Unicast wakeup frame enable */
	/* Bits 3-2 reserved */
	CONFIG5_LAN_WAKE = (1 << 1), /**< LANWake signal enabled */
	CONFIG5_PME_STATUS = (1 << 0), /**< PMEn change: 0 = SW, 1 = SW+PCI */
};

/** Interrupt_masks
 *
 *  The masks are the same for both IMR and ISR
 */
enum rtl8169_interrupts {
	INT_SERR = (1 << 15), /**< System error interrupt */
	INT_TIME_OUT = (1 << 14), /**< Time out interrupt */
	/* Bits 9 - 13 reserved */
	INT_SW = (1 << 8), /**< Software Interrupt */
	INT_TDU = (1 << 7), /**< Tx Descriptor Unvailable */
	INT_FIFOOVW = (1 << 6), /**< Receiver FIFO overflow interrupt */
	INT_PUN = (1 << 5), /**< Packet Underrun/Link Change Interrupt  */
	INT_RXOVW = (1 << 4), /**< Receiver buffer overflow */
	INT_TER = (1 << 3), /**< Transmit error interrupt */
	INT_TOK = (1 << 2), /**< Transmit OK interrupt */
	INT_RER = (1 << 1), /**< Receive error interrupt */
	INT_ROK = (1 << 0), /**< Receive OK interrupt */
	INT_KNOWN = (INT_SERR | INT_TIME_OUT | INT_SW | INT_TDU |
	    INT_FIFOOVW | INT_PUN | INT_RXOVW | INT_TER |
	    INT_TOK | INT_RER | INT_ROK),
};

/** Transmit status descriptor registers bits */
enum rtl8169_tsd {
	TSD_CRS = (1 << 31), /**< Carrier Sense Lost */
	TSD_TABT = (1 << 30), /**< Transmit Abort */
	TSD_OWC = (1 << 29), /**< Out of Window Collision */
	TSD_CDH = (1 << 28), /**< CD Heart Beat */
	TSD_NCC_SHIFT = 24, /**< Collision Count - bit shift */
	TSD_NCC_SIZE = 4, /**< Collision Count - bit size */
	TSD_NCC_MASK = (1 << 4) - 1, /**< Collision Count - bit size */
	TSD_ERTXTH_SHIFT = 16, /**< Early Tx Threshold - bit shift */
	TSD_ERTXTH_SIZE = 6, /**< Early Tx  Treshold - bit size */
	TSD_TOK = (1 << 15), /**< Transmit OK */
	TSD_TUN = (1 << 14), /**< Transmit FIFO Underrun */
	TSD_OWN = (1 << 13), /**< OWN */
	TSD_SIZE_SHIFT = 0, /**< Size - bit shift */
	TSD_SIZE_SIZE = 13, /**< Size - bit size */
	TSD_SIZE_MASK = 0x1fff, /**< Size - bit mask */
};

/** Receiver control register values */
enum rtl8169_rcr {
	RCR_MulERINT = 1 << 24,    /**< Multiple early interrupt select */
	/* Bits 23-17 reserved */
	RCR_RER8 = 1 << 16,
	RCR_RXFTH_SHIFT = 13, /**< Rx FIFO treshold part shitf */
	RCR_RXFTH_SIZE = 3, /**< Rx FIFO treshold part size */
	/* Bits 12-11 reserved */
	RCR_MXDMA_SHIFT = 8, /**< Max DMA Burst Size part shift */
	RCR_MXDMA_SIZE  = 3, /**< Max DMA Burst Size part size */
	/* Bit 7 reserved */
	RCR_ACCEPT_ERROR = 1 << 5, /**< Accept error frame */
	RCR_ACCEPT_RUNT = 1 << 4, /**< Accept Runt (8-64 bytes) frames */
	RCR_ACCEPT_BROADCAST = 1 << 3, /**< Accept broadcast */
	RCR_ACCEPT_MULTICAST = 1 << 2, /**< Accept multicast */
	RCR_ACCEPT_PHYS_MATCH = 1 << 1, /**< Accept device MAC address match */
	RCR_ACCEPT_ALL_PHYS = 1 << 0, /**< Accept all frames with phys. desticnation */
	RCR_ACCEPT_MASK = (1 << 6) - 1   /**< Mask of accept part */
};

enum rtl8169_cr9346 {
	CR9346EEM1_SHIFT = 6, /**< RTL8169 operating mode shift */
	CR9346EEM1_SIZE = 2, /**< RTL8169 operating mode mask */
	/* Bits 5-4 reserved */
	EECS = (1 << 3), /**< EECS pin of 93C46 */
	EESK = (1 << 2), /**< EESK pin of 93C46 */
	EEDI = (1 << 1), /**< EEDI pin of 93C46 */
	EEDO = (1 << 0), /**< EEDO pin of 93C46 */
};

/** Auto-negotiation advertisement register */
enum rtl8169_anar {
	ANAR_NEXT_PAGE = (1 << 15), /**< Next page bit, 0 - primary capability */
	ANAR_ACK = (1 << 14), /**< Capability reception acknowledge */
	ANAR_REMOTE_FAULT = (1 << 13), /**< Remote fault detection capability */
	ANAR_PAUSE = (1 << 10), /**< Symetric pause frame capability */
	ANAR_100T4 = (1 << 9), /**< T4, not supported by the device */
	ANAR_100TX_FD = (1 << 8), /**< 100BASE_TX full duplex */
	ANAR_100TX_HD = (1 << 7), /**< 100BASE_TX half duplex */
	ANAR_10_FD = (1 << 6), /**< 10BASE_T full duplex */
	ANAR_10_HD = (1 << 5), /**< 10BASE_T half duplex */
	ANAR_SELECTOR = 0x1, /**< Selector */
};

enum rtl8169_phystatus {
	PHYSTATUS_TBIEN = (1 << 7), /**< TBI enabled */
	PHYSTATUS_TXFLOW = (1 << 6), /**< TX flow control enabled */
	PHYSTATUS_RXFLOW = (1 << 5), /**< RX flow control enabled */
	PHYSTATUS_1000M = (1 << 4), /**< Link speed is 1000Mbit/s */
	PHYSTATUS_100M = (1 << 3), /**< Link speed is 100Mbit/s */
	PHYSTATUS_10M = (1 << 2), /**< Link speed is 10Mbit/s */
	PHYSTATUS_LINK = (1 << 1), /**< Link up */
	PHYSTATUS_FDX = (1 << 0), /**< Link is full duplex */
};

enum rtl8169_tppoll {
	TPPOLL_HPQ = (1 << 7), /**< Start transmit on high priority queue */
	TPPOLL_NPQ = (1 << 6), /**< Start transmit on normal queue */
	/* Bits 5-1 reserved */
	TPPOLL_FSWINT = (1 << 0), /** < Generate software interrupt */
};

enum rtl8169_phyar {
	PHYAR_RW_SHIFT = 31, /**< Read (0) or write (1) command */
	PHYAR_RW_READ = (0 << PHYAR_RW_SHIFT),
	PHYAR_RW_WRITE = (1 << PHYAR_RW_SHIFT),
	PHYAR_ADDR_SHIFT = 15,
	PHYAR_ADDR_MASK = 0x1f,
	PHYAR_DATA_MASK = 0xffff
};

enum rtl8169_bmcr {
	BMCR_RESET = (1 << 15), /**< Software reset */
	BMCR_SPD_100 = (1 << 13), /**< 100 MBit mode set */
	BMCR_AN_ENABLE = (1 << 12), /**< Autonegotion enable */
	BMCR_AN_RESTART = (1 << 9), /**< Restart autonegotion */
	BMCR_DUPLEX = (1 << 8), /**< Duplex mode: 1=full duplex */
	BMCR_SPD_1000 = (1 << 6), /**< 1000 Mbit mode set */
};

enum rtl8169_descr_control {
	CONTROL_OWN = (1 << 31), /**< Descriptor ownership */
	CONTROL_EOR = (1 << 30), /**< End Of Ring marker */
	CONTROL_FS = (1 << 29), /**< First Segment marker */
	CONTROL_LS = (1 << 28), /**< Last Segment marker */
	CONTROL_LGSEN = (1 << 27), /**< Large send enable */
	CONTROL_MSS_SHIFT = 16,
	CONTROL_MSS_MASK = 10,
	CONTROL_FRAMELEN_MASK = 0xffff
};

enum rtl8169_descr_txstatus {
	TXSTATUS_UNDERRUN = (1 << 25),
	TXSTATUS_TXERRSUM = (1 << 23),
	TXSTATUS_OWINCOL = (1 << 22),
	TXSTATUS_LINKFAIL = (1 << 21),
	TXSTATUS_EXCESSCOL = (1 << 20)
};

enum rtl8169_descr_rxstatus {
	RXSTATUS_MAR = (1 << 27),
	RXSTATUS_PAM = (1 << 26),
	RXSTATUS_BAR = (1 << 25),
	RXSTATUS_BOVF = (1 << 24),
	RXSTATUS_FOVF = (1 << 23),
	RXSTATUS_RWT = (1 << 22),
	RXSTATUS_RES = (1 << 21),
	RXSTATUS_RUNT = (1 << 20),
	RXSTATUS_CRC = (1 << 19),
	RXSTATUS_PID1 = (1 << 18),
	RXSTATUS_PID0 = (1 << 17),
	RXSTATUS_IPF = (1 << 16),
	RXSTATUS_UDPF = (1 << 15),
	RXSTATUS_TCPF = (1 << 14)
};

typedef struct rtl8169_descr {
	uint32_t	control;
	uint32_t	vlan;
	uint32_t	buf_low;
	uint32_t	buf_high;
} rtl8169_descr_t;

#endif
