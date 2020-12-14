/*
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

/** @file rtl8139_defs.h
 *
 * Registers, bit positions and masks definition
 * of the RTL8139 network family cards
 */

#ifndef RTL8139_DEFS_H_
#define RTL8139_DEFS_H_

#include <stdint.h>
#include <ddi.h>

/** Size of RTL8139 registers address space */
#define RTL8139_IO_SIZE  256

/** Maximal transmitted frame length
 *
 * Maximal transmitted frame length in bytes
 * allowed according to the RTL8139 documentation
 * (see SIZE part of TSD documentation).
 *
 */
#define RTL8139_FRAME_MAX_LENGTH  1792

/** HW version
 *
 * As can be detected from HWVERID part of TCR
 * (Transmit Configuration Register).
 *
 */
typedef enum {
	RTL8139 = 0,          /**< RTL8139 */
	RTL8139A,             /**< RTL8139A */
	RTL8139A_G,           /**< RTL8139A-G */
	RTL8139B,             /**< RTL8139B */
	RTL8130,              /**< RTL8130 */
	RTL8139C,             /**< RTL8139C */
	RTL8100,              /**< RTL8100 */
	RTL8139Cp,            /**< RTL8139C+ */
	RTL8139D,             /**< RTL8139D */
	RTL8100B = RTL8139D,  /**< RTL8100B and RTL8139D, the same HWVERID in TCR */
	RTL8101,              /**< RTL8101 */
	RTL8139_VER_COUNT     /**< Count of known RTL versions, the last value */
} rtl8139_version_id_t;

/** Registers of RTL8139 family card offsets from the memory address base */
enum rtl8139_registers {
	IDR0  = 0x00,    /**< First MAC address bit, 6 1b registres sequence */
	MAC0  = IDR0,    /**< Alias for IDR0 */

	// 0x06 - 0x07 reserved

	MAR0    = 0x08,  /**< Multicast mask registers 8 1b registers sequence */

	TSD0    = 0x10,  /**< Transmit status of descriptor 0 */
	TSD1    = 0x14,  /**< Transmit status of descriptor 1 */
	TSD2    = 0x18,  /**< Transmit status of descriptor 2 */
	TSD3    = 0x1C,  /**< Transmit status of descriptor 3 */

	TSAD0   = 0x20,  /**< Physical address of the 1st transmitter buffer, 4b */
	TSAD1   = 0x24,  /**< Physical address of the 2nd transmitter buffer, 4b */
	TSAD2   = 0x28,  /**< Physical address of the 3rd transmitter buffer, 4b */
	TSAD3   = 0x3C,  /**< Physical address of the 4th transmitter buffer, 4b */

	RBSTART = 0x30,  /**< Receive (Rx) buffer start address, 4b */
	ERBCR   = 0x34,  /**< Early receive (Rx) byte count register, 2b */
	ERSR    = 0x36,  /**< Early receive (Rx) status register, 1b */

	CR      = 0x37,  /**< Command register, 1b */
	CAPR    = 0x38,  /**< Current address of frame read, 2b */
	CBA     = 0x3a,  /**< Current buffer address, 2b */

	IMR     = 0x3c,  /**< Interrupt mask register, 2b */
	ISR     = 0x3e,  /**< Interrupt status register, 2b */

	TCR     = 0x40,  /**< Transmit (Tx) configuration register, 4b */
	RCR     = 0x44,  /**< Receive (Rx) configuration register, 4b */

	TCTR    = 0x48,  /**< Timer count register */
	MPC     = 0x4c,  /**< Missed packet count */

	CR9346  = 0x50,  /**< 93C46 command register (locking of registers) */

	CONFIG0 = 0x51,  /**< Configuration register 0, 1b */
	CONFIG1 = 0x52,  /**< Configuration register 1, 1b */

	// 0x53 reserved

	TIMERINT = 0x54,  /**< Timer interrupt register, 4b */
	MSR      = 0x58,  /**< Media status register, 1b */

	CONFIG3  = 0x59,  /**< Configuration register 3, 1b */
	CONFIG4  = 0x5a,  /**< Configuration register 4, 1b */

	// 0x5b reserved

	MULINT   = 0x5c,  /**< Multiple interrupt select, 2b */
	RERID    = 0x5e,  /**< PCI revision ID = 0x10, 1b */

	// 0x5f reserved

	TSALLD  = 0x60,   /**< Transmit status of all descriptors, 2b */

	BMCR    = 0x62,   /**< Basic mode control register */
	BMSR    = 0x64,   /**< Basic mode status register */

	ANAR    = 0x66,   /**< Auto-negotiation advertisement register */
	ANLPAR  = 0x68,   /**< Auto-negotiation link partner register */
	ANER    = 0x6a,   /**< Auto-negotiation expansion register */
	DIS     = 0x6c,   /**< Disconnect counter */
	FCSC    = 0x6e,   /**< False carrier sense counter */
	NWAYTR  = 0x70,   /**< n-way test register */
	REC     = 0x72,   /**< RX_ER counter */
	CSCR    = 0x74,   /**< CS configuration register */

	// 0x76 - 0x77 reserved

	PHY1_PARM = 0x78, /**< PHY parameter 1 */
	TW_PARM   = 0x7c, /**< Twister parameter */
	PHY2_PARM = 0x80, /**< PHY parameter 2 */

	// 0x81 reserved

	TDOKLA  = 0x82,   /**< Low Address of a Tx Descriptor with Tx DMA Ok */
	CRC0    = 0x84,   /**< Power Management CRC register0 for wakeup frame 0 */
	WAKEUP0 = 0x8c,   /**< Power Management wakeup frame 0 */
	LSBCRC0 = 0xcc,   /**< Least significant masked byte of WF0 */
	FLASH   = 0xd4,   /**< Flash memory read/write register */

	CONFIG5 = 0xd8,   /**< Configuration register 5 */

	TPPOL   = 0xd9,   /**< Transmit priority polling register */

	// 0xda - 0xdf reserved

	CPCR    = 0xe0,   /**< C+ mode command register */

	// 0xe2 - 0xe3 reserved

	RDSAR   = 0xe4,   /**< Receive Descriptor Start Address Register */
	ETTHR   = 0xec,   /**< Early transmit threshold register */

	// 0xed - 0xef reserved

	FER   = 0xf0,    /**< Function event register */
	FEMR  = 0xf4,    /**< Function event mask register */
	FPSR  = 0xf8,    /**< Function present state register */
	FFER  = 0xfc,    /**< Function force event register */
	MIIR  = 0xfc     /**< MII register */
};

/** Mask of valid bits in MPC value */
#define MPC_VMASK UINT32_C(0xFFFFFF);

/** Command register bits */
enum rtl8139_cr {
	CR_BUFE = (1 << 0),  /**< Buffer empty bit - read only */
	CR_TE   = (1 << 2),  /**< Transmitter enable bit */
	CR_RE   = (1 << 3),  /**< Receiver enable bit */
	CR_RST  = (1 << 4)   /**< Reset -  set to 1 to force software reset */
};

/** Config1 register bits */
enum rtl8139_config1 {
	CONFIG1_LEDS_SHIFT = 6,       /**< Shift of CONFIG1_LEDS bits */
	CONFIG1_LEDS_SIZE  = 2,       /**< Size of CONFIG1_LEDS bits */

	CONFIG1_DVRLOAD  = (1 << 5),  /**< Driver load */
	CONFIG1_LWACT    = (1 << 4),  /**< LWAKE active mode */
	CONFIG1_MEMMAP   = (1 << 3),  /**< Memory mapping  */
	CONFIG1_IOMAP    = (1 << 2),  /**< I/O space mapping */
	CONFIG1_VPD      = (1 << 1),  /**< Set to enable Vital Product Data */
	CONFIG1_PMEn     = (1 << 0)   /**< Power management enabled */
};

/** Mask of 9346CR register for lock configuration registers */
#define RTL8139_REGS_LOCKED 0
/** Mask of 9346CR register for unlock configuration registers */
#define RTL8139_REGS_UNLOCKED 0xC0

/** Put rtl8139 to normal mode.
 *
 * Writing to Config0-4 and part of BMCR registers is not allowed
 */
static inline void rtl8139_regs_lock(void *io_base)
{
	pio_write_8(io_base + CR9346, RTL8139_REGS_LOCKED);
}

/** Allow to change Config0-4 and BMCR register  */
static inline void rtl8139_regs_unlock(void *io_base)
{
	pio_write_8(io_base + CR9346, RTL8139_REGS_UNLOCKED);
}

/** Force soft reset of the chip. After it:
 *	receiver and transmitter are disabled
 *	transmitter FIFO is cleared
 *	transmitter buffer is set to TSDA0
 *	receiver buffer is empty
 *
 *	The reset bit in command register must be set to 1, the value of the
 *	the register is 1 during reset operation
 *
 *	@param base_port The base address of the port mappings
 */
#define rtl8139_hw_reset(base_port)\
	{\
		pio_write_8(base_port + CR, CR_RST);\
		while((pio_read_8(base_port + CR) & CR_RST) != 0);\
	}

/** Interrupt_masks
 *
 *  The masks are the same for both IMR and ISR
 */
enum rtl8139_interrupts {
	INT_SERR          = (1 << 15),  /**< System error interrupt */
	INT_TIME_OUT      = (1 << 14),  /**< Time out interrupt */
	INT_LENGTH_CHANGE = (1 << 13),  /**< Cable length change interrupt  */

	/* bits 7 - 12 reserved */

	INT_FIFOOVW = (1 << 6),   /**< Receiver FIFO overflow interrupt */
	INT_PUN     = (1 << 5),   /**< Packet Underrun/Link Change Interrupt  */
	INT_RXOVW   = (1 << 4),   /**< Receiver buffer overflow */
	INT_TER     = (1 << 3),   /**< Transmit error interrupt */
	INT_TOK     = (1 << 2),   /**< Transmit OK interrupt */
	INT_RER     = (1 << 1),   /**< Receive error interrupt */
	INT_ROK     = (1 << 0)    /**< Receive OK interrupt */
};

/** Transmit status descriptor registers bits */
enum rtl8139_tsd {
	TSD_CRS          = (1 << 31),    /**< Carrier Sense Lost */
	TSD_TABT         = (1 << 30),    /**<  Transmit Abort */
	TSD_OWC          = (1 << 29),    /**< Out of Window Collision */
	TSD_CDH          = (1 << 28),    /**< CD Heart Beat */
	TSD_NCC_SHIFT    = 24,           /**< Collision Count - bit shift */
	TSD_NCC_SIZE     = 4,            /**< Collision Count - bit size */
	TSD_NCC_MASK     = (1 << 4) - 1, /**< Collision Count - bit size */
	TSD_ERTXTH_SHIFT = 16,           /**< Early Tx Threshold - bit shift */
	TSD_ERTXTH_SIZE  = 6,            /**< Early Tx  Treshold - bit size */
	TSD_TOK          = (1 << 15),    /**< Transmit OK */
	TSD_TUN          = (1 << 14),    /**< Transmit FIFO Underrun */
	TSD_OWN          = (1 << 13),    /**< OWN */
	TSD_SIZE_SHIFT   = 0,            /**< Size - bit shift */
	TSD_SIZE_SIZE    = 13,           /**< Size - bit size */
	TSD_SIZE_MASK    = 0x1fff        /**< Size - bit mask */
};

/** Receiver control register values */
enum rtl8139_rcr {
	/** Early Rx treshold part shift */
	RCR_ERTH_SHIFT = 24,
	/** Early Rx treshold part size */
	RCR_ERTH_SIZE = 4,

	/** Multiple early interrupt select */
	RCR_MulERINT = 1 << 17,

	/** Minimal error frame length (1 = 8B, 0 = 64B).
	 * If AER/AR is set, RER8 is "Don't care"
	 */
	RCR_RER8 = 1 << 16,

	/** Rx FIFO treshold part shift */
	RCR_RXFTH_SHIFT = 13,
	/** Rx FIFO treshold part size */
	RCR_RXFTH_SIZE  = 3,

	/** Rx buffer length part shift */
	RCR_RBLEN_SHIFT = 11,
	/** Rx buffer length part size */
	RCR_RBLEN_SIZE  = 2,

	/** 8K + 16 byte rx buffer */
	RCR_RBLEN_8k  = 0x00 << RCR_RBLEN_SHIFT,
	/** 16K + 16 byte rx buffer */
	RCR_RBLEN_16k = 0x01 << RCR_RBLEN_SHIFT,
	/** 32K + 16 byte rx buffer */
	RCR_RBLEN_32k = 0x02 << RCR_RBLEN_SHIFT,
	/** 64K + 16 byte rx buffer */
	RCR_RBLEN_64k = 0x03 << RCR_RBLEN_SHIFT,

	/** Max DMA Burst Size part shift */
	RCR_MXDMA_SHIFT = 8,
	/** Max DMA Burst Size part size */
	RCR_MXDMA_SIZE  = 3,

	/** Rx buffer wrapped */
	RCR_WRAP              = 1 << 7,
	/** Accept error frame */
	RCR_ACCEPT_ERROR      = 1 << 5,
	/** Accept Runt (8-64 bytes) frames */
	RCR_ACCEPT_RUNT       = 1 << 4,
	/** Accept broadcast */
	RCR_ACCEPT_BROADCAST  = 1 << 3,
	/** Accept multicast */
	RCR_ACCEPT_MULTICAST  = 1 << 2,
	/** Accept device MAC address match */
	RCR_ACCEPT_PHYS_MATCH = 1 << 1,
	/** Accept all frames with phys. destination */
	RCR_ACCEPT_ALL_PHYS   = 1 << 0,
	/** Mask of accept part */
	RCR_ACCEPT_MASK = (1 << 6) - 1
};

/** CSCR register bits */
enum rtl8139_cscr {
	CS_Testfun       = (1 << 15),
	/** Low TPI link disable signal */
	CS_LD            = (1 << 9),
	/** Heart beat enable; 10Mbit mode only */
	CS_HEART_BEAT    = (1 << 8),
	/** Enable jabber function */
	CS_JABBER_ENABLE = (1 << 7),
	CS_F_LINK100     = (1 << 6),
	CS_F_CONNECT     = (1 << 5),
	/** connection status: 1 = valid, 0 = disconnected */
	CS_CON_STATUS    = (1 << 3),
	/** LED1 pin connection status indication */
	CS_CON_STATUS_EN = (1 << 2),
	/** Bypass Scramble */
	CS_PASS_SCR      = (1 << 0)
};

/** MSR register bits */
enum rtl8139_msr {
	MSR_TXFCE       = (1 << 7),  /**< Transmitter flow control enable */
	MSR_RXFCE       = (1 << 6),  /**< Receiver flow control enable */

	MSR_AUX_PRESENT = (1 << 4),  /**< Aux. Power present Status */
	MSR_SPEED10     = (1 << 3),  /**< 10MBit mode sign (1 = 10Mb, 0 = 100Mb) */
	MSR_LINKB       = (1 << 2),  /**< Link Bad (fail) */
	MSR_TXPF        = (1 << 1),  /**< Transmitter pause flag */
	MSR_RXPF        = (1 << 0)   /**< Receiver pause flag */
};

/** BMCR register bits (basic mode control register) */
enum rtl8139_bmcr {
	BMCR_Reset     = (1 << 15),  /**< Software reset */
	BMCR_Spd_100   = (1 << 13),  /**< 100 MBit mode set, 10 MBit otherwise */
	BMCR_AN_ENABLE = (1 << 12),  /**< Autonegotion enable */

	/* 10,11 reserved */

	BMCR_AN_RESTART = (1 << 9),  /**< Restart autonegotion */
	BMCR_DUPLEX     = (1 << 8)   /**< Duplex mode: 1=full duplex */

	/* 0-7 reserved */
};

/** Auto-negotiation advertisement register */
enum rtl8139_anar {
	/** Next page bit, 0 - primary capability, 1 - protocol specific */
	ANAR_NEXT_PAGE    = (1 << 15),
	/** Capability reception acknowledge */
	ANAR_ACK          = (1 << 14),
	/** Remote fault detection capability */
	ANAR_REMOTE_FAULT = (1 << 13),
	/** Symetric pause frame capability */
	ANAR_PAUSE        = (1 << 10),
	/** T4, not supported by the device */
	ANAR_100T4        = (1 << 9),
	/** 100BASE_TX full duplex */
	ANAR_100TX_FD     = (1 << 8),
	/** 100BASE_TX half duplex */
	ANAR_100TX_HD     = (1 << 7),
	/** 10BASE_T full duplex */
	ANAR_10_FD        = (1 << 6),
	/** 10BASE_T half duplex */
	ANAR_10_HD        = (1 << 5),
	/** Selector, CSMA/CD (0x1) supported only */
	ANAR_SELECTOR     = 0x1
};

/**  Autonegotiation expansion register bits */
enum rtl8139_aner {
	ANER_MFL        = (1 << 4),  /**< Multiple link fault occured */
	ANER_LP_NP_ABLE = (1 << 3),  /**< Link parent supports next page */
	ANER_NP_ABLE    = (1 << 2),  /**< Local node is able to send next pages */
	ANER_PAGE_RX    = (1 << 1),  /** New page received, cleared on LPAR read */
	ANER_LP_NW_ABLE = (1 << 0)   /**< Link partner autonegotiation support */
};

enum rtl8139_config5 {
	CONFIG5_BROADCAST_WAKEUP = (1 << 6),  /**< Broadcast wakeup frame enable */
	CONFIG5_MULTICAST_WAKEUP = (1 << 5),  /**< Multicast wakeup frame enable */
	CONFIG5_UNICAST_WAKEUP   = (1 << 4),  /**< Unicast wakeup frame enable */

	/** Descending/ascending grow of Rx/Tx FIFO (to test FIFO SRAM only) */
	CONFIG5_FIFO_ADDR_PTR = (1 << 3),
	/** Powersave if cable is disconnected */
	CONFIG5_LINK_DOWN_POWERSAVE = (1 << 2),

	CONFIG5_LAN_WAKE     = (1 << 1),   /**< LANWake signal enabled */
	CONFIG5_PME_STATUS   = (1 << 0)    /**< PMEn change: 0 = SW, 1 = SW+PCI */
};

enum rtl8139_config3 {
	CONFIG3_GNT_SELECT = (1 << 7),  /**< Gnt select */
	CONFIG3_PARM_EN    = (1 << 6),  /**< Parameter enabled (100MBit mode) */
	CONFIG3_MAGIC      = (1 << 5),  /**< WoL Magic frame enable */
	CONFIG3_LINK_UP    = (1 << 4),  /**< Wakeup if link is reestablished */
	CONFIG3_CLKRUN_EN  = (1 << 2),  /**< CLKRUN enabled */ /* TODO: check what does it mean */
	CONFIG3_FBTBEN     = (1 << 0)   /**< Fast back to back enabled */
};

enum rtl8139_config4 {
	/** Automatic RxFIFO owerflow clear */
	CONFIG4_RxFIFOAutoClr = (1 << 7),
	/** Analog poweroff */
	CONFIG4_AnaOff        = (1 << 6),
	/** Long wakeup frame (2xCRC8 + 3xCRC16) */
	CONFIG4_LongWF        = (1 << 5),
	/** LWAKE and PMEB assertion */
	CONFIG4_LWPME         = (1 << 4),
	/** LWake pattern */
	CONFIG4_LWPTN         = (1 << 2),
	/** Preboot wakeup */
	CONFIG4_PBWakeup      = (1 << 0)
};

/** Maximal runt frame size + 1 */
#define RTL8139_RUNT_MAX_SIZE  64

/** Bits in frame header */
enum rtl8139_frame_header {
	RSR_MAR  = (1 << 15),  /**< Multicast received */
	RSR_PAM  = (1 << 14),  /**< Physical address match */
	RSR_BAR  = (1 << 13),  /**< Broadcast match */

	RSR_ISE  = (1 << 5),   /**< Invalid symbol error, 100BASE-TX only */
	RSR_RUNT = (1 << 4),   /**< Runt frame (< RTL8139_RUNT_MAX_SIZE bytes) */

	RSR_LONG = (1 << 3),   /**< Long frame (size > 4k bytes) */
	RSR_CRC  = (1 << 2),   /**< CRC error */
	RSR_FAE  = (1 << 1),   /**< Frame alignment error */
	RSR_ROK  = (1 << 0)    /**< Good frame received */
};

enum rtl8139_tcr_bits {
	/** HW version id, part A shift */
	HWVERID_A_SHIFT = 26,
	/** HW version id, part A bit size */
	HWVERID_A_SIZE  = 5,
	/** HW version id, part A mask */
	HWVERID_A_MASK  = (1 << 5) - 1,

	/** The interframe gap time setting shift */
	IFG_SHIFT = 24,
	/** The interframe gap time setting bit size */
	IFG_SIZE  = 2,

	/** HW version id, part B shift */
	HWVERID_B_SHIFT = 22,
	/** HW version id, part B bit size */
	HWVERID_B_SIZE  = 2,
	/** HW version id, part B mask */
	HWVERID_B_MASK  = (1 << 2) - 1,

	/** Loopback mode shift */
	LOOPBACK_SHIFT  = 17,
	/** Loopback mode size. 00 = normal, 11 = loopback */
	LOOPBACK_SIZE   = 2,

	/** Append CRC at the end of a frame */
	APPEND_CRC = 1 << 16,

	/** Max. DMA Burst per TxDMA shift, burst = 16^value */
	MXTxDMA_SHIFT = 8,
	/** Max. DMA Burst per TxDMA bit size */
	MXTxDMA_SIZE  = 3,

	/** Retries before aborting shift */
	TX_RETRY_COUNT_SHIFT = 4,
	/** Retries before aborting size */
	TX_RETRY_COUNT_SIZE  = 4,

	/** Retransmit aborted frame at the last transmitted descriptor */
	CLEAR_ABORT = 1 << 0
};

#define RTL8139_HWVERID_A(tcr) (((tcr) >> HWVERID_A_SHIFT) & HWVERID_A_MASK)
#define RTL8139_HWVERID_B(tcr) (((tcr) >> HWVERID_B_SHIFT) & HWVERID_B_MASK)
#define RTL8139_HWVERID(tcr) ((RTL8139_HWVERID_A(tcr) << HWVERID_B_SIZE) | \
    RTL8139_HWVERID_B(tcr))

/** Mapping of HW version -> version ID */
struct rtl8139_hwver_map {
	uint32_t hwverid;             /**< HW version value in the register */
	rtl8139_version_id_t ver_id;  /**< appropriate version id */
};

/** Mapping of HW version -> version ID */
extern const struct rtl8139_hwver_map rtl8139_versions[RTL8139_VER_COUNT + 1];
extern const char *model_names[RTL8139_VER_COUNT];

/** Size in the frame header while copying from RxFIFO to Rx buffer */
#define RTL8139_EARLY_SIZE  UINT16_C(0xfff0)

/** The only supported pause frame time value */
#define RTL8139_PAUSE_VAL  UINT16_C(0xFFFF)

/** Size of the frame header in front of the received frame */
#define RTL_FRAME_HEADER_SIZE  4

/** 8k buffer */
#define RTL8139_RXFLAGS_SIZE_8  0
/** 16k buffer */
#define RTL8139_RXFLAGS_SIZE_16 1
/** 32k buffer */
#define RTL8139_RXFLAGS_SIZE_32 2
/** 64k buffer */
#define RTL8139_RXFLAGS_SIZE_64 3

/** Get the buffer initial size without 16B padding
 *  Size is (8 + 2^flags) kB (^ in mean power)
 *
 *  @param flags The flags for Rx buffer size, 0-3
 */
#define RTL8139_RXSIZE(flags) (1 << (13 + (flags)))

/** Padding of the receiver buffer */
#define RTL8139_RXBUF_PAD 16
/** Size needed for buffer allocation */
#define RTL8139_RXBUF_LENGTH(flags) (RTL8139_RXSIZE(flags) + RTL8139_RXBUF_PAD)

#endif
