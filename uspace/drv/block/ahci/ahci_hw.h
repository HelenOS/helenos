/*
 * Copyright (c) 2012 Petr Jerman
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

/** @file
 * Header for AHCI driver (AHCI 1.3 specification).
 */

#ifndef __AHCI_HW_H__
#define __AHCI_HW_H__

#include <stdint.h>

/*
 * AHCI standard constants
 */

/** AHCI standard 1.3 - maximum ports. */
#define AHCI_MAX_PORTS  32

/*
 * AHCI PCI Registers
 */

/** AHCI PCI register Identifiers offset. */
#define AHCI_PCI_ID     0x00
/** AHCI PCI register Command offset. */
#define AHCI_PCI_CMD    0x04
/** AHCI PCI register Device Status offset. */
#define AHCI_PCI_STS    0x06
/** AHCI PCI register Revision ID offset. */
#define AHCI_PCI_RID    0x08
/** AHCI PCI register Class Codes offset. */
#define AHCI_PCI_CC     0x09
/** AHCI PCI register Cache Line Size offset. */
#define AHCI_PCI_CLS    0x0C
/** AHCI PCI register Master Latency Timer offset. */
#define AHCI_PCI_MLT    0x0D
/** AHCI PCI register Header Type offset. */
#define AHCI_PCI_HTYPE  0x0E
/** AHCI PCI register Built In Self Test (Optional) offset. */
#define AHCI_PCI_BIST   0x0F
/** AHCI PCI register Other Base Address Registres (Optional). */
#define AHCI_PCI_BAR0   0x10
/** AHCI PCI register Other Base Address Registres (Optional). */
#define AHCI_PCI_BAR1   0x14
/** AHCI PCI register Other Base Address Registres (Optional). */
#define AHCI_PCI_BAR2   0x18
/** AHCI PCI register Other Base Address Registres (Optional). */
#define AHCI_PCI_BAR3   0x1C
/** AHCI PCI register Other Base Address Registres (Optional). */
#define AHCI_PCI_BAR4   0x20
/** AHCI PCI register AHCI Base Address offset. */
#define AHCI_PCI_ABAR   0x24
/** AHCI PCI register Subsystem Identifiers offset. */
#define AHCI_PCI_SS     0x2C
/** AHCI PCI register Expansion ROM Base Address (Optional) offset. */
#define AHCI_PCI_EROM   0x30
/** AHCI PCI register Capabilities Pointer offset. */
#define AHCI_PCI_CAP    0x34
/** AHCI PCI register Interrupt Information offset. */
#define AHCI_PCI_INTR   0x3C
/** AHCI PCI register Min Grant (Optional) offset. */
#define AHCI_PCI_MGNT   0x3E
/** AHCI PCI register Max Latency (Optional) offset. */
#define AHCI_PCI_MLAT   0x3F

/** AHCI PCI register Identifiers. */
typedef struct {
	/** Indicates the company vendor assigned by the PCI SIG. */
	uint16_t vendorid;
	/** Indicates what device number assigned by the vendor */
	uint16_t deviceid;
} ahci_pcireg_id_t;

/** AHCI PCI register Command. */
typedef union {
	struct {
		/** I/O Space Enable. */
		unsigned int iose : 1;
		/** Memory Space Enable. */
		unsigned int mse : 1;
		/** Bus Master Enable. */
		unsigned int bme : 1;
		/** Special Cycle Enable. */
		unsigned int sce : 1;
		/** Memory Write and Invalidate Enable. */
		unsigned int mwie : 1;
		/** VGA Palette Snooping Enable. */
		unsigned int vga : 1;
		/** Parity Error Response Enable. */
		unsigned int pee : 1;
		/** Wait Cycle Enable. */
		unsigned int wcc : 1;
		/** SERR# Enable. */
		unsigned int see : 1;
		/** Fast Back-to-Back Enable. */
		unsigned int fbe : 1;
		/** Interrupt Disable - disables the HBA from generating interrupts.
		 * This bit does not have any effect on MSI operation.
		 */
		unsigned int id : 1;
		/** Reserved. */
		unsigned int reserved : 5;
	};
	uint16_t u16;
} ahci_pcireg_cmd_t;

/** AHCI PCI register Command - Interrupt Disable bit. */
#define AHCI_PCIREG_CMD_ID  0x0400

/** AHCI PCI register Command - Bus Master Enable bit. */
#define AHCI_PCIREG_CMD_BME  0x0004

/** AHCI PCI register Device status. */
typedef union {
	struct {
		/** Reserved. */
		unsigned int reserved1 : 3;
		/** Indicate the interrupt status of the device (1 = asserted). */
		unsigned int is : 1;
		/** Indicates presence of capatibility list. */
		unsigned int cl : 1;
		/** 66 Mhz capable. */
		unsigned int c66 : 1;
		/** Reserved. */
		unsigned int reserved2 : 1;
		/** Fast back to back capable. */
		unsigned int fbc : 1;
		/** Master data parity error detected. */
		unsigned int dpd : 1;
		/** Device select timing. */
		unsigned int devt : 2;
		/** Signaled target abort. */
		unsigned int sta : 1;
		/** Received target abort. */
		unsigned int rta : 1;
		/** Received master abort. */
		unsigned int rma : 1;
		/** Signaled system error. */
		unsigned int sse : 1;
		/** Detected parity error. */
		unsigned int dpe : 1;
	};
	uint16_t u16;
} ahci_pcireg_sts_t;

/** AHCI PCI register Revision ID. */
typedef struct {
	/** Indicates stepping of the HBA hardware. */
	uint8_t u8;
} ahci_pcireg_rid_t;

/** AHCI PCI register Class Codes. */
typedef struct {
	/** Programing interface, when set to 01h and the scc is set to 06h,
	 * indicates that this an AHCI HBA major revision 1.
	 */
	uint8_t pi;
	/** When set to 06h, indicates that is a SATA device. */
	uint8_t scc;
	/** Value 01 indicates that is a mass storage device. */
	uint8_t bcc;
} ahci_pcireg_cc_t_t;

/** AHCI PCI register Cache Line Size. */
typedef struct {
	/** Cache line size for use with the memory write and invalidate command. */
	uint8_t u8;
} ahci_pcireg_cls_t;

/** AHCI PCI register Master Latency Timer. */
typedef struct {
	/** Master latency timer,indicates the number of clocks the HBA is allowed
	 * to acts as master on PCI.
	 */
	uint8_t u8;
} ahci_pcireg_mlt_t;

/** AHCI PCI register Header Type. */
typedef union {
	struct {
		/** Header layout. */
		unsigned int hl : 7;
		/** Multi function device flag. */
		unsigned int mfd : 1;
	};
	uint8_t u8;
} ahci_pciregs_htype_t;

/** AHCI PCI register Built in self test. */
typedef union {
	struct {
		/** Indicates the completion status of BIST
		 * non-zero value indicates a failure.
		 */
		unsigned int cc : 4;
		/** Reserved. */
		unsigned int reserved : 2;
		/** Software sets this bit to 1 to invoke BIST,
		 * the HBA clears this bit to 0 when BIST is complete.
		 */
		unsigned int sb : 1;
		/** BIST capable. */
		unsigned int bc : 1;
	};
	uint8_t u8;
} ahci_pciregs_bist_t;

/** AHCI PCI register AHCI Base Address <BAR 5>. */
typedef union {
	struct {
		/** Indicates a request for register memory space. */
		unsigned int rte : 1;
		/** Indicates the this range can be mapped anywhere in 32-bit address
		 * space.
		 */
		unsigned int tp : 2;
		/** Indicate that this range is not prefetchable. */
		unsigned int pf : 1;
		/** Reserved. */
		unsigned int reserved : 9;
		/** Base address of registry memory space. */
		unsigned int ba : 19;
	};
	uint32_t u32;
} ahci_pciregs_abar_t;

/** AHCI PCI register Subsystem Identifiers. */
typedef struct {
	/** Sub system vendor identifier. */
	uint8_t ssvid;
	/** Sub system identifier. */
	uint8_t ssid;
} ahci_pcireg_ss_t;

/** AHCI PCI registers Expansion ROM Base Address. */
typedef struct {
	/** Indicates the base address of the HBA expansion ROM. */
	uint32_t u32;
} ahci_pcireg_erom_t;

/** AHCI PCI register Capabilities Pointer. */
typedef struct {
	/** Indicates the first capability pointer offset. */
	uint8_t u8;
} ahci_pcireg_cap_t;

/** AHCI PCI register Interrupt Information. */
typedef struct {
	/*
	 * Software written value to indicate which interrupt vector
	 * the interrupt is connected to.
	 */
	uint8_t iline;
	/** This indicates the interrupt pin the HBA uses. */
	uint8_t ipin;
} ahci_pcireg_intr;

/** AHCI PCI register Min Grant (Optional). */
typedef struct {
	/** Indicates the minimum grant time that the device
	 * wishes grant asserted.
	 */
	uint8_t u8;
} ahci_pcireg_mgnt_t;

/** AHCI PCI register Max Latency (Optional). */
typedef struct {
	/** Indicates the maximum latency that the device can withstand. */
	uint8_t u8;
} ahci_pcireg_mlat_t;

/*
 * AHCI Memory Registers
 */

/** Number of pages for ahci memory registers. */
#define AHCI_MEMREGS_PAGES_COUNT  8

/** AHCI Memory register Generic Host Control - HBA Capabilities. */
typedef union {
	struct {
		/** Number of Ports. */
		unsigned int np : 5;
		/** Supports External SATA. */
		unsigned int sxs : 1;
		/** Enclosure Management Supported. */
		unsigned int ems : 1;
		/** Command Completion Coalescing Supported. */
		unsigned int cccs : 1;
		/** Number of Command Slots. */
		unsigned int ncs : 5;
		/** Partial State Capable. */
		unsigned int psc : 1;
		/** Slumber State Capable. */
		unsigned int ssc : 1;
		/** PIO Multiple DRQ Block. */
		unsigned int pmd : 1;
		/** FIS-based Switching Supported. */
		unsigned int fbss : 1;
		/** Supports Port Multiplier. */
		unsigned int spm : 1;
		/** Supports AHCI mode only. */
		unsigned int sam : 1;
		/** Reserved. */
		unsigned int reserved : 1;
		/** Interface Speed Support. */
		unsigned int iss : 4;
		/** Supports Command List Override. */
		unsigned int sclo : 1;
		/** Supports Activity LED. */
		unsigned int sal : 1;
		/** Supports Aggressive Link Power Management. */
		unsigned int salp : 1;
		/** Supports Staggered Spin-up. */
		unsigned int sss : 1;
		/** Supports Mechanical Presence Switch. */
		unsigned int smps : 1;
		/** Supports SNotification Register. */
		unsigned int ssntf : 1;
		/** Supports Native Command Queuing. */
		unsigned int sncq : 1;
		/** Supports 64-bit Addressing. */
		unsigned int s64a : 1;
	};
	uint32_t u32;
} ahci_ghc_cap_t;

/** AHCI Memory register Generic Host Control Global Host Control. */
typedef union {
	struct {
		/** HBA Reset. */
		unsigned int hr : 1;
		/** Interrupt Enable. */
		unsigned int ie : 1;
		/** MSI Revert to Single Message. */
		unsigned int mrsm : 1;
		/** Reserved. */
		unsigned int reserved : 28;
		/** AHCI Enable. */
		unsigned int ae : 1;
	};
	uint32_t u32;
} ahci_ghc_ghc_t;

/** AHCI Enable mask bit. */
#define AHCI_GHC_GHC_AE  0x80000000

/** AHCI Interrupt Enable mask bit. */
#define AHCI_GHC_GHC_IE  0x00000002

/** AHCI Memory register Interrupt pending register. */
typedef uint32_t ahci_ghc_is_t;

/** AHCI GHC register offset. */
#define AHCI_GHC_IS_REGISTER_OFFSET  2

/** AHCI ports registers offset. */
#define AHCI_PORTS_REGISTERS_OFFSET  64

/** AHCI port registers size. */
#define AHCI_PORT_REGISTERS_SIZE  32

/** AHCI port IS register offset. */
#define AHCI_PORT_IS_REGISTER_OFFSET  4

/** AHCI Memory register Ports implemented. */
typedef struct {
	/** If a bit is set to 1, the corresponding port
	 * is available for software use.
	 */
	uint32_t u32;
} ahci_ghc_pi_t;

/** AHCI Memory register AHCI version. */
typedef struct {
	/** Indicates the minor version */
	uint16_t mnr;
	/** Indicates the major version */
	uint16_t mjr;
} ahci_ghc_vs_t;

/** AHCI Memory register Command completion coalesce control. */
typedef union {
	struct {
		/** Enable CCC features. */
		unsigned int en : 1;
		/** Reserved. */
		unsigned int reserved : 2;
		/** Interrupt number for CCC. */
		unsigned int intr : 5;
		/** Number of command completions that are necessary to cause
		 * a CCC interrupt.
		 */
		uint8_t cc;
		/** Timeout value in  ms. */
		uint16_t tv;
	};
	uint32_t u32;
} ahci_ghc_ccc_ctl_t;

/** AHCI Memory register Command completion coalescing ports. */
typedef struct {
	/** If a bit is set to 1, the corresponding port is
	 * part of the command completion coalescing feature.
	 */
	uint32_t u32;
} ahci_ghc_ccc_ports_t;

/** AHCI Memory register Enclosure management location. */
typedef struct {
	/** Size of the transmit message buffer area in dwords. */
	uint16_t sz;
	/*
	 * Offset of the transmit message buffer area in dwords
	 * from the beginning of ABAR
	 */
	uint16_t ofst;
} ahci_ghc_em_loc;

/** AHCI Memory register Enclosure management control. */
typedef union {
	struct {
		/** Message received. */
		unsigned int mr : 1;
		/** Reserved. */
		unsigned int reserved : 7;
		/** Transmit message. */
		unsigned int tm : 1;
		/** Reset. */
		unsigned int rst : 1;
		/** Reserved. */
		unsigned int reserved2 : 6;
		/** LED message types. */
		unsigned int led : 1;
		/** Support SAFT-TE message type. */
		unsigned int safte : 1;
		/** Support SES-2 message type. */
		unsigned int ses2 : 1;
		/** Support SGPIO register. */
		unsigned int sgpio : 1;
		/** Reserved. */
		unsigned int reserved3 : 4;
		/** Single message buffer. */
		unsigned int smb : 1;
		/**  Support transmitting only. */
		unsigned int xmt : 1;
		/** Activity LED hardware driven. */
		unsigned int alhd : 1;
		/** Port multiplier support. */
		unsigned int pm : 1;
		/** Reserved. */
		unsigned int reserved4 : 4;
	};
	uint32_t u32;
} ahci_ghc_em_ctl_t;

/** AHCI Memory register HBA capatibilities extended. */
typedef union {
	struct {
		/** HBA support BIOS/OS handoff mechanism,
		 * implemented BOHC register.
		 */
		unsigned int boh : 1;
		/** Support for NVMHCI register. */
		unsigned int nvmp : 1;
		/** Automatic partial to slumber transition support. */
		unsigned int apst : 1;
		/** Reserved. */
		unsigned int reserved : 29;
	};
	uint32_t u32;
} ahci_ghc_cap2_t;

/** AHCI Memory register BIOS/OS Handoff control and status. */
typedef union {
	struct {
		/** BIOS Owned semaphore. */
		unsigned int bos : 1;
		/** OS Owned semaphore. */
		unsigned int oos : 1;
		/** SMI on OS ownership change enable. */
		unsigned int sooe : 1;
		/** OS ownership change. */
		unsigned int ooc : 1;
		/** BIOS Busy. */
		unsigned int bb : 1;
		/** Reserved. */
		unsigned int reserved : 27;
	};
	uint32_t u32;
} ahci_ghc_bohc_t;

/** AHCI Memory register Generic Host Control. */
typedef struct {
	/** Host Capabilities */
	uint32_t cap;
	/** Global Host Control */
	uint32_t ghc;
	/** Interrupt Status */
	ahci_ghc_is_t is;
	/** Ports Implemented */
	uint32_t pi;
	/** Version */
	uint32_t vs;
	/** Command Completion Coalescing Control */
	uint32_t ccc_ctl;
	/** Command Completion Coalescing Ports */
	uint32_t ccc_ports;
	/** Enclosure Management Location */
	uint32_t em_loc;
	/** Enclosure Management Control */
	uint32_t em_ctl;
	/** Host Capabilities Extended */
	uint32_t cap2;
	/** BIOS/OS Handoff Control and Status */
	uint32_t bohc;
} ahci_ghc_t;

/** AHCI Memory register Port x Command List Base Address. */
typedef union {
	struct {
		/** Reserved. */
		unsigned int reserved : 10;
		/** Command List Base Address (CLB) - Indicates the 32-bit base physical
		 * address for the command list for this port. This base is used when
		 * fetching commands to execute. The structure pointed to by this
		 * address range is 1K-bytes in length. This address must be 1K-byte
		 * aligned as indicated by bits 09:00 being read only.
		 */
		unsigned int clb : 22;
	};
	uint32_t u32;
} ahci_port_clb_t;

/** AHCI Memory register Port x Command List Base Address Upper 32-Bits. */
typedef struct {
	/** Command List Base Address Upper (CLBU): Indicates the upper 32-bits
	 * for the command list base physical address for this port. This base
	 * is used when fetching commands to execute. This register shall
	 * be read only for HBAs that do not support 64-bit addressing.
	 */
	uint32_t u32;
} ahci_port_clbu_t;

/** AHCI Memory register Port x FIS Base Address. */
typedef union {
	struct {
		/** Reserved. */
		unsigned int reserved : 8;
		/** FIS Base Address (FB) - Indicates the 32-bit base physical address
		 * for received FISes. The structure pointed to by this address range
		 * is 256 bytes in length. This address must be 256-byte aligned as
		 * indicated by bits 07:00 being read only. When FIS-based switching
		 * is in use, this structure is 4KB in length and the address shall be
		 * 4KB aligned.
		 */
		unsigned int fb : 24;
	};
	uint32_t u32;
} ahci_port_fb_t;

/** AHCI Memory register Port x FIS Base Address Upper 32-Bits. */
typedef struct {
	/** FIS Base Address Upper (FBU) - Indicates the upper 32-bits
	 * for the received FIS base physical address for this port. This register
	 * shall be read only for HBAs that do not support 64-bit addressing.
	 */
	uint32_t u32;
} ahci_port_fbu_t;

/** AHCI Memory register Port x Interrupt Status. */
typedef uint32_t ahci_port_is_t;

#define AHCI_PORT_IS_DHRS  (1 << 0)
#define AHCI_PORT_IS_PSS   (1 << 1)
#define AHCI_PORT_IS_DSS   (1 << 2)
#define AHCI_PORT_IS_SDBS  (1 << 3)
#define AHCI_PORT_IS_UFS   (1 << 4)
#define AHCI_PORT_IS_DPS   (1 << 5)
#define AHCI_PORT_IS_PCS   (1 << 6)
#define AHCI_PORT_IS_DMPS  (1 << 7)

#define AHCI_PORT_IS_PRCS  (1 << 22)
#define AHCI_PORT_IS_IPMS  (1 << 23)
#define AHCI_PORT_IS_OFS   (1 << 24)
#define AHCI_PORT_IS_INFS  (1 << 26)
#define AHCI_PORT_IS_IFS   (1 << 27)
#define AHCI_PORT_IS_HDBS  (1 << 28)
#define AHCI_PORT_IS_HBFS  (1 << 29)
#define AHCI_PORT_IS_TFES  (1 << 30)
#define AHCI_PORT_IS_CPDS  (1 << 31)

#define AHCI_PORT_END_OF_OPERATION \
	(AHCI_PORT_IS_DHRS | \
	AHCI_PORT_IS_SDBS )

#define AHCI_PORT_IS_ERROR \
	(AHCI_PORT_IS_UFS | \
	AHCI_PORT_IS_PCS | \
	AHCI_PORT_IS_DMPS | \
	AHCI_PORT_IS_PRCS | \
	AHCI_PORT_IS_IPMS | \
	AHCI_PORT_IS_OFS | \
	AHCI_PORT_IS_INFS | \
	AHCI_PORT_IS_IFS | \
	AHCI_PORT_IS_HDBS | \
	AHCI_PORT_IS_HBFS | \
	AHCI_PORT_IS_TFES | \
	AHCI_PORT_IS_CPDS)

#define AHCI_PORT_IS_PERMANENT_ERROR \
	(AHCI_PORT_IS_PCS | \
	AHCI_PORT_IS_DMPS | \
	AHCI_PORT_IS_PRCS | \
	AHCI_PORT_IS_IPMS | \
	AHCI_PORT_IS_CPDS )

/** Evaluate end of operation status from port interrupt status.
 *
 * @param port_is Value of port interrupt status.
 *
 * @return Indicate end of operation status.
 *
 */
static inline int ahci_port_is_end_of_operation(ahci_port_is_t port_is)
{
	return port_is & AHCI_PORT_END_OF_OPERATION;
}

/** Evaluate error status from port interrupt status.
 *
 * @param port_is Value of port interrupt status.
 *
 * @return Indicate error status.
 *
 */
static inline int ahci_port_is_error(ahci_port_is_t port_is)
{
	return port_is & AHCI_PORT_IS_ERROR;
}

/** Evaluate permanent error status from port interrupt status.
 *
 * @param port_is Value of port interrupt status.
 *
 * @return Indicate permanent error status.
 *
 */
static inline int ahci_port_is_permanent_error(ahci_port_is_t port_is)
{
	return port_is & AHCI_PORT_IS_PERMANENT_ERROR;
}

/** Evaluate task file error status from port interrupt status.
 *
 * @param port_is Value of port interrupt status.
 *
 * @return Indicate error status.
 *
 */
static inline int ahci_port_is_tfes(ahci_port_is_t port_is)
{
	return port_is & AHCI_PORT_IS_TFES;
}

/** AHCI Memory register Port x Interrupt Enable. */
typedef union {
	struct {
		/** Device to Host Register FIS Interrupt Enable. */
		unsigned int dhre : 1;
		/** PIO Setup FIS Interrupt Enable. */
		unsigned int pse : 1;
		/** DMA Setup FIS Interrupt Enable. */
		unsigned int dse : 1;
		/** Set Device Bits Interrupt Eenable. */
		unsigned int sdbe : 1;
		/** Unknown FIS Interrupt Enable. */
		unsigned int ufe : 1;
		/** Descriptor Processed Interrupt Enable. */
		unsigned int dpe : 1;
		/** Port Change Interrupt Enable. */
		unsigned int pce : 1;
		/** Device Mechanical Presence Enable. */
		unsigned int dmpe : 1;
		/** Reserved. */
		unsigned int reserved1 : 14;
		/** PhyRdy Change Interrupt Enable. */
		unsigned int prce : 1;
		/** Incorrect Port Multiplier Enable. */
		unsigned int ipme : 1;
		/** Overflow Status Enable. */
		unsigned int ofe : 1;
		/** Reserved. */
		unsigned int reserved2 : 1;
		/** Interface Non-fatal Error Enable. */
		unsigned int infe : 1;
		/** Interface Fatal Error Enable. */
		unsigned int ife : 1;
		/** Host Bus Data Error Enable. */
		unsigned int hbde : 1;
		/** Host Bus Fatal Error Enable. */
		unsigned int hbfe : 1;
		/** Task File Error Enable. */
		unsigned int tfee : 1;
		/** Cold Port Detect Enable. */
		unsigned int cpde : 1;
	};
	uint32_t u32;
} ahci_port_ie_t;

/** AHCI Memory register Port x Command and Status. */
typedef union {
	struct {
		/** Start - when set, the HBA may process the command list. */
		unsigned int st : 1;
		/** Spin-Up Device. */
		unsigned int sud : 1;
		/** Power On Device. */
		unsigned int pod : 1;
		/** Command List Override. */
		unsigned int clo : 1;
		/** FIS Receive Enable. */
		unsigned int fre : 1;
		/** Reserved. */
		unsigned int reserved : 3;
		/** Current Command Slot. */
		unsigned int ccs : 5;
		/** Mechanical Presence Switch State. */
		unsigned int mpss : 1;
		/** FIS Receive Running. */
		unsigned int fr : 1;
		/** Command List Running. */
		unsigned int cr : 1;
		/** Cold Presence State. */
		unsigned int cps : 1;
		/** Port Multiplier Attached. */
		unsigned int pma : 1;
		/** Hot Plug Capable Port. */
		unsigned int hpcp : 1;
		/** Mechanical Presence Switch Attached to Port. */
		unsigned int mpsp : 1;
		/** Cold Presence Detection. */
		unsigned int cpd : 1;
		/** External SATA Port. */
		unsigned int esp : 1;
		/** FIS-based Switching Capable Port. */
		unsigned int fbscp : 1;
		/** Automatic Partial to Slumber Transitions Enabled. */
		unsigned int apste : 1;
		/** Device is ATAPI. */
		unsigned int atapi : 1;
		/** Drive LED on ATAPI Enable. */
		unsigned int dlae : 1;
		/** Aggressive Link Power Management Enable. */
		unsigned int alpe : 1;
		/** Aggressive Slumber / Partial. */
		unsigned int asp : 1;
		/** Interface Communication Control.
		 * Values:
		 * 7h - fh Reserved,
		 * 6h Slumber - This shall cause the HBA to request a transition
		 * of the interface to the Slumber state,
		 * 3h - 5h Reserved,
		 * 2h Partial - This shall cause the HBA to request a transition
		 * of the interface to the Partial state,
		 * 1h Active,
		 * 0h No-Op / Idle.
		 */
		unsigned int icc : 4;
	};
	uint32_t u32;
} ahci_port_cmd_t;

/** AHCI Memory register Port x Task File Data. */
typedef union {
	struct {
		/** Status (STS): Contains the latest copy of the task file
		 * status register.
		 */
		uint8_t sts;
		/** Error (ERR) - Contains the latest copy of the task file
		 * error register.
		 */
		uint8_t err;
		/** Reserved. */
		uint16_t reserved;
	};
	uint32_t u32;
} ahci_port_tfd_t;

/** AHCI Memory register Port x Signature. */
typedef union {
	struct {
		/** Sector Count Register */
		uint8_t sector_count;
		/** LBA Low Register */
		uint8_t lba_lr;
		/** LBA Mid Register */
		uint8_t lba_mr;
		/** LBA High Register */
		uint8_t lba_hr;
	};
	uint32_t u32;
} ahci_port_sig_t;

/** AHCI Memory register Port x Serial ATA Status (SCR0: SStatus). */
typedef union {
	struct {
		/** Device Detection */
		unsigned int det : 4;
		/** Current Interface Speed */
		unsigned int spd : 4;
		/** Interface Power Management */
		unsigned int ipm : 4;
		/** Reserved. */
		unsigned int reserved : 20;
	};
	uint32_t u32;
} ahci_port_ssts_t;

/** Device detection active status. */
#define AHCI_PORT_SSTS_DET_ACTIVE  3

/** AHCI Memory register Port x Serial ATA Control (SCR2: SControl). */
typedef union {
	struct {
		/** Device Detection Initialization */
		unsigned int det : 4;
		/** Speed Allowed */
		unsigned int spd : 4;
		/** Interface Power Management Transitions Allowed */
		unsigned int ipm : 4;
		/** Reserved. */
		unsigned int reserved : 20;
	};
	uint32_t u32;
} ahci_port_sctl_t;

/** AHCI Memory register Port x Port x Serial ATA Error (SCR1: SError). */
typedef struct {
	/** Error (ERR) - The ERR field contains error information for use
	 * by host software in determining the appropriate response to the
	 * error condition.
	 */
	uint16_t err;
	/** Diagnostics (DIAG) - Contains diagnostic error information for use
	 * by diagnostic software in validating correct operation or isolating
	 * failure modes.
	 */
	uint16_t diag;
} ahci_port_serr_t;

/** AHCI Memory register Port x Serial ATA Active (SCR3: SActive). */
typedef struct {
	/** Device Status - Each bit corresponds to the TAG and
	 * command slot of a native queued command, where bit 0 corresponds
	 * to TAG 0 and command slot 0.
	 */
	uint32_t u32;
} ahci_port_sact_t;

/** AHCI Memory register Port x Command Issue. */
typedef struct {
	/** Commands Issued - Each bit corresponds to a command slot,
	 *  where bit 0 corresponds to command slot 0.
	 */
	uint32_t u32;
} ahci_port_ci_t;

/** AHCI Memory register Port x Serial ATA Notification
 * (SCR4: SNotification).
 */
typedef struct {
	/** PM Notify (PMN): This field indicates whether a particular device with
	 * the corresponding PM Port number issued a Set Device Bits FIS
	 * to the host with the Notification bit set.
	 */
	uint16_t pmn;
	/** Reserved. */
	uint16_t reserved;
} ahci_port_sntf_t;

/** AHCI Memory register Port x FIS-based Switching Control.
 * This register is used to control and obtain status
 * for Port Multiplier FIS-based switching.
 */
typedef union {
	struct {
		/** Enable */
		unsigned int en : 1;
		/** Device Error Clear */
		unsigned int dec : 1;
		/** Single Device Error */
		unsigned int sde : 1;
		/** Reserved. */
		unsigned int reserved1 : 5;
		/** Device To Issue */
		unsigned int dev : 1;
		/** Active Device Optimization */
		unsigned int ado : 1;
		/** Device With Error */
		unsigned int dwe : 1;
		/** Reserved. */
		unsigned int reserved2 : 1;
	};
	uint32_t u32;
} ahci_port_fbs_t;

/** AHCI Memory register Port. */
typedef volatile struct {
	/** Port x Command List Base Address. */
	uint32_t pxclb;
	/** Port x Command List Base Address Upper 32-Bits. */
	uint32_t pxclbu;
	/** Port x FIS Base Address. */
	uint32_t pxfb;
	/** Port x FIS Base Address Upper 32-Bits. */
	uint32_t pxfbu;
	/** Port x Interrupt Status. */
	ahci_port_is_t pxis;
	/** Port x Interrupt Enable. */
	uint32_t pxie;
	/** Port x Command and Status. */
	uint32_t pxcmd;
	/** Reserved. */
	uint32_t reserved1;
	/** Port x Task File Data. */
	uint32_t pxtfd;
	/**  Port x Signature. */
	uint32_t pxsig;
	/** Port x Serial ATA Status (SCR0: SStatus). */
	uint32_t pxssts;
	/** Port x Serial ATA Control (SCR2: SControl). */
	uint32_t pxsctl;
	/** Port x Serial ATA Error (SCR1: SError). */
	uint32_t pxserr;
	/** Port x Serial ATA Active (SCR3: SActive). */
	uint32_t pxsact;
	/** Port x Command Issue. */
	uint32_t pxci;
	/** Port x Serial ATA Notification (SCR4: SNotification). */
	uint32_t pxsntf;
	/** Port x FIS-based Switching Control. */
	uint32_t pxfbs;
	/** Reserved. */
	uint32_t reserved2[11];
	/** Port x Vendor Specific. */
	uint32_t pxvs[4];
} ahci_port_t;

/** AHCI Memory Registers. */
typedef volatile struct {
	/** Generic Host Control. */
	ahci_ghc_t ghc;
	/** Reserved. */
	uint32_t reserved[13];
	/** Reserved for NVMHCI. */
	uint32_t reservedfornvmhci[16];
	/** Vendor Specific registers. */
	uint32_t vendorspecificsregs[24];
	/** Ports. */
	ahci_port_t ports[AHCI_MAX_PORTS];
} ahci_memregs_t;

/** AHCI Command header entry.
 *
 * This structure is not an AHCI register.
 *
 */
typedef volatile struct {
	/** Flags. */
	uint16_t flags;
	/** Physical Region Descriptor Table Length. */
	uint16_t prdtl;
	/** Physical Region Descriptor Byte Count. */
	uint32_t bytesprocessed;
	/** Command Table Descriptor Base Address. */
	uint32_t cmdtable;
	/** Command Table Descriptor Base Address Upper 32-bits. */
	uint32_t cmdtableu;
} ahci_cmdhdr_t;

/** Clear Busy upon R_OK (C) flag. */
#define AHCI_CMDHDR_FLAGS_CLEAR_BUSY_UPON_OK  0x0400

/** Write operation flag. */
#define AHCI_CMDHDR_FLAGS_WRITE  0x0040

/** 2 DW length command flag. */
#define AHCI_CMDHDR_FLAGS_2DWCMD  0x0002

/** 5 DW length command flag. */
#define AHCI_CMDHDR_FLAGS_5DWCMD  0x0005

/** AHCI Command Physical Region Descriptor entry.
 *
 * This structure is not an AHCI register.
 *
 */
typedef volatile struct {
	/** Word aligned 32-bit data base address. */
	uint32_t data_address_low;
	/** Upper data base address, valid only for 64-bit HBA addressing. */
	uint32_t data_address_upper;
	/** Reserved. */
	uint32_t reserved1;
	/** Data byte count */
	unsigned int dbc : 22;
	/** Reserved */
	unsigned int reserved2 : 9;
	/** Set Interrupt on each operation completion */
	unsigned int ioc : 1;
} ahci_cmd_prdt_t;

#endif
