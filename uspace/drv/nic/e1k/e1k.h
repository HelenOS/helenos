/*
 * Copyright (c) 2011 Zdenek Bouska
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

/** @file e1k.h
 *
 * Registers, bit positions and masks definition of the E1000 network family
 * cards
 *
 */

#ifndef E1K_H_
#define E1K_H_

#include <stdint.h>

/** Ethernet CRC size after frame received in rx_descriptor */
#define E1000_CRC_SIZE  4

#define VET_VALUE  0x8100

#define E1000_RAL_ARRAY(n)   (E1000_RAL + ((n) * 8))
#define E1000_RAH_ARRAY(n)   (E1000_RAH + ((n) * 8))
#define E1000_VFTA_ARRAY(n)  (E1000_VFTA + (0x04 * (n)))

/** Receive descriptior */
typedef struct {
	/** Buffer Address - physical */
	uint64_t phys_addr;
	/** Length is per segment */
	uint16_t length;
	/** Checksum - not all types, on some reseved  */
	uint16_t checksum;
	/** Status field */
	uint8_t status;
	/** Errors field */
	uint8_t errors;
	/** Special Field - not all types, on some reseved  */
	uint16_t special;
} e1000_rx_descriptor_t;

/** Legacy transmit descriptior */
typedef struct {
	/** Buffer Address - physical */
	uint64_t phys_addr;
	/** Length is per segment */
	uint16_t length;
	/** Checksum Offset */
	uint8_t checksum_offset;
	/** Command field */
	uint8_t command;
	/** Status field, upper bits are reserved */
	uint8_t status;
	/** Checksum Start Field */
	uint8_t checksum_start_field;
	/** Special Field */
	uint16_t special;
} e1000_tx_descriptor_t;

/** E1000 boards */
typedef enum {
	E1000_82540,
	E1000_82541,
	E1000_82541REV2,
	E1000_82545,
	E1000_82546,
	E1000_82547,
	E1000_82572,
	E1000_80003ES2
} e1000_board_t;

typedef struct {
	uint32_t eerd_start;
	uint32_t eerd_done;

	uint32_t eerd_address_offset;
	uint32_t eerd_data_offset;
} e1000_info_t;

/** VLAN tag bits */
typedef enum {
	VLANTAG_CFI = (1 << 12),  /**< Canonical Form Indicator */
} e1000_vlantag_t;

/** Transmit descriptor COMMAND field bits */
typedef enum {
	TXDESCRIPTOR_COMMAND_VLE = (1 << 6),   /**< VLAN frame Enable */
	TXDESCRIPTOR_COMMAND_RS = (1 << 3),    /**< Report Status */
	TXDESCRIPTOR_COMMAND_IFCS = (1 << 1),  /**< Insert FCS */
	TXDESCRIPTOR_COMMAND_EOP = (1 << 0)    /**< End Of Packet */
} e1000_txdescriptor_command_t;

/** Transmit descriptor STATUS field bits */
typedef enum {
	TXDESCRIPTOR_STATUS_DD = (1 << 0)  /**< Descriptor Done */
} e1000_txdescriptor_status_t;

/** E1000 Registers */
typedef enum {
	E1000_CTRL = 0x0,      /**< Device Control Register */
	E1000_STATUS = 0x8,    /**< Device Status Register */
	E1000_EERD = 0x14,     /**< EEPROM Read Register */
	E1000_TCTL = 0x400,    /**< Transmit Control Register */
	E1000_TIPG = 0x410,    /**< Transmit IPG Register */
	E1000_TDBAL = 0x3800,  /**< Transmit Descriptor Base Address Low */
	E1000_TDBAH = 0x3804,  /**< Transmit Descriptor Base Address High */
	E1000_TDLEN = 0x3808,  /**< Transmit Descriptor Length */
	E1000_TDH = 0x3810,    /**< Transmit Descriptor Head */
	E1000_TDT = 0x3818,    /**< Transmit Descriptor Tail */
	E1000_RCTL = 0x100,    /**< Receive Control Register */
	E1000_RDBAL = 0x2800,  /**< Receive Descriptor Base Address Low */
	E1000_RDBAH = 0x2804,  /**< Receive Descriptor Base Address High */
	E1000_RDLEN = 0x2808,  /**< Receive Descriptor Length */
	E1000_RDH = 0x2810,    /**< Receive Descriptor Head */
	E1000_RDT = 0x2818,    /**< Receive Descriptor Tail */
	E1000_RAL = 0x5400,    /**< Receive Address Low */
	E1000_RAH = 0x5404,    /**< Receive Address High */
	E1000_VFTA = 0x5600,   /**< VLAN Filter Table Array */
	E1000_VET = 0x38,      /**< VLAN Ether Type */
	E1000_FCAL = 0x28,     /**< Flow Control Address Low */
	E1000_FCAH = 0x2C,     /**< Flow Control Address High */
	E1000_FCTTV = 0x170,   /**< Flow Control Transmit Timer Value */
	E1000_FCT = 0x30,      /**< Flow Control Type */
	E1000_ICR = 0xC0,      /**< Interrupt Cause Read Register */
	E1000_ITR = 0xC4,      /**< Interrupt Throttling Register */
	E1000_IMS = 0xD0,      /**< Interrupt Mask Set/Read Register */
	E1000_IMC = 0xD8       /**< Interrupt Mask Clear Register */
} e1000_registers_t;

/** Device Control Register fields */
typedef enum {
	CTRL_FD = (1 << 0),    /**< Full-Duplex */
	CTRL_LRST = (1 << 3),  /**< Link Reset */
	CTRL_ASDE = (1 << 5),  /**< Auto-Speed Detection Enable */
	CTRL_SLU = (1 << 6),   /**< Set Link Up */
	CTRL_ILOS = (1 << 7),  /**< Invert Loss-of-Signal */

	/** Speed selection shift */
	CTRL_SPEED_SHIFT = 8,
	/** Speed selection size */
	CTRL_SPEED_SIZE = 2,
	/** Speed selection all bit set value */
	CTRL_SPEED_ALL = ((1 << CTRL_SPEED_SIZE) - 1),
	/** Speed selection shift */
	CTRL_SPEED_MASK = CTRL_SPEED_ALL << CTRL_SPEED_SHIFT,
	/** Speed selection 10 Mb/s value */
	CTRL_SPEED_10 = 0,
	/** Speed selection 10 Mb/s value */
	CTRL_SPEED_100 = 1,
	/** Speed selection 10 Mb/s value */
	CTRL_SPEED_1000 = 2,

	CTRL_FRCSPD = (1 << 11),   /**< Force Speed */
	CTRL_FRCDPLX = (1 << 12),  /**< Force Duplex */
	CTRL_RST = (1 << 26),      /**< Device Reset */
	CTRL_VME = (1 << 30),      /**< VLAN Mode Enable */
	CTRL_PHY_RST = (1 << 31)   /**< PHY Reset */
} e1000_ctrl_t;

/** Device Status Register fields */
typedef enum {
	STATUS_FD = (1 << 0),  /**< Link Full Duplex configuration Indication */
	STATUS_LU = (1 << 1),  /**< Link Up Indication */

	/** Link speed setting shift */
	STATUS_SPEED_SHIFT = 6,
	/** Link speed setting size */
	STATUS_SPEED_SIZE = 2,
	/** Link speed setting all bits set */
	STATUS_SPEED_ALL = ((1 << STATUS_SPEED_SIZE) - 1),
	/** Link speed setting 10 Mb/s value */
	STATUS_SPEED_10 = 0,
	/** Link speed setting 100 Mb/s value */
	STATUS_SPEED_100 = 1,
	/** Link speed setting 1000 Mb/s value variant A */
	STATUS_SPEED_1000A = 2,
	/** Link speed setting 1000 Mb/s value variant B */
	STATUS_SPEED_1000B = 3,
} e1000_status_t;

/** Transmit IPG Register fields
 *
 * IPG = Inter Packet Gap
 *
 */
typedef enum {
	TIPG_IPGT_SHIFT = 0,    /**< IPG Transmit Time shift */
	TIPG_IPGR1_SHIFT = 10,  /**< IPG Receive Time 1 */
	TIPG_IPGR2_SHIFT = 20   /**< IPG Receive Time 2 */
} e1000_tipg_t;

/** Transmit Control Register fields */
typedef enum {
	TCTL_EN = (1 << 1),    /**< Transmit Enable */
	TCTL_PSP =  (1 << 3),  /**< Pad Short Packets */
	TCTL_CT_SHIFT = 4,     /**< Collision Threshold shift */
	TCTL_COLD_SHIFT = 12   /**< Collision Distance shift */
} e1000_tctl_t;

/** ICR register fields */
typedef enum {
	ICR_TXDW = (1 << 0),  /**< Transmit Descriptor Written Back */
	ICR_RXT0 = (1 << 7)   /**< Receiver Timer Interrupt */
} e1000_icr_t;

/** RAH register fields */
typedef enum {
	RAH_AV = (1 << 31)   /**< Address Valid */
} e1000_rah_t;

/** RCTL register fields */
typedef enum {
	RCTL_EN = (1 << 1),    /**< Receiver Enable */
	RCTL_SBP = (1 << 2),   /**< Store Bad Packets */
	RCTL_UPE = (1 << 3),   /**< Unicast Promiscuous Enabled */
	RCTL_MPE = (1 << 4),   /**< Multicast Promiscuous Enabled */
	RCTL_BAM = (1 << 15),  /**< Broadcast Accept Mode */
	RCTL_VFE = (1 << 18)   /**< VLAN Filter Enable */
} e1000_rctl_t;

#endif
