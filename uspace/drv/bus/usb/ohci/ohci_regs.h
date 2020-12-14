/*
 * Copyright (c) 2011 Jan Vesely
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

/** @addtogroup drvusbohci
 * @{
 */
/** @file
 * @brief OHCI host controller register structure
 */

#ifndef DRV_OHCI_OHCI_REGS_H
#define DRV_OHCI_OHCI_REGS_H

#include <ddi.h>
#include <byteorder.h>

#define OHCI_WR(reg, val) pio_write_32(&(reg), host2uint32_t_le(val))
#define OHCI_RD(reg) uint32_t_le2host(pio_read_32(&(reg)))
#define OHCI_SET(reg, val) pio_set_32(&(reg), host2uint32_t_le(val), 1)
#define OHCI_CLR(reg, val) pio_clear_32(&(reg), host2uint32_t_le(val), 1)

#define LEGACY_REGS_OFFSET 0x100

/** OHCI memory mapped registers structure */
typedef struct ohci_regs {
	const ioport32_t revision;

	ioport32_t control;

	ioport32_t command_status;

	/** Interupt enable/disable/status,
	 * reads give the same value,
	 * writing causes enable/disable,
	 * status is write-clean (writing 1 clears the bit
	 */
	ioport32_t interrupt_status;
	ioport32_t interrupt_enable;
	ioport32_t interrupt_disable;

	/** HCCA pointer (see hw_struct hcca.h) */
	ioport32_t hcca;

	/** Currently executed periodic endpoint */
	const ioport32_t periodic_current;

	/** The first control endpoint */
	ioport32_t control_head;

	/** Currently executed control endpoint */
	ioport32_t control_current;

	/** The first bulk endpoint */
	ioport32_t bulk_head;

	/** Currently executed bulk endpoint */
	ioport32_t bulk_current;

	/** Done TD list, this value is periodically written to HCCA */
	const ioport32_t done_head;

	/** Frame time and max packet size for all transfers */
	ioport32_t fm_interval;

	/** Bit times remaining in current frame */
	const ioport32_t fm_remaining;

	/** Frame number */
	const ioport32_t fm_number;

	/** Remaining bit time in frame to start periodic transfers */
	ioport32_t periodic_start;

	/** Threshold for starting LS transaction */
	ioport32_t ls_threshold;

	/** The first root hub control register */
	ioport32_t rh_desc_a;

	/** The other root hub control register */
	ioport32_t rh_desc_b;

	/** Root hub status register */
	ioport32_t rh_status;

	/** Root hub per port status */
	ioport32_t rh_port_status[];
#define RHPS_CCS_FLAG (1 << 0)                /* r: current connect status,
                                               * w: 1-clear port enable, 0-N/S*/
#define RHPS_CLEAR_PORT_ENABLE RHPS_CCS_FLAG
#define RHPS_PES_FLAG (1 << 1)               /* r: port enable status
                                              * w: 1-set port enable, 0-N/S */
#define RHPS_SET_PORT_ENABLE RHPS_PES_FLAG
#define RHPS_PSS_FLAG (1 << 2)                /* r: port suspend status
                                               * w: 1-set port suspend, 0-N/S */
#define RHPS_SET_PORT_SUSPEND RHPS_PSS_FLAG
#define RHPS_POCI_FLAG (1 << 3)                /* r: port over-current
                                                * (if reports are per-port
                                                * w: 1-clear port suspend
						*  (start resume if suspened)
                                                *    0-nothing */
#define RHPS_CLEAR_PORT_SUSPEND RHPS_POCI_FLAG
#define RHPS_PRS_FLAG (1 << 4)                /* r: port reset status
                                               * w: 1-set port reset, 0-N/S */
#define RHPS_SET_PORT_RESET RHPS_PRS_FLAG
#define RHPS_PPS_FLAG (1 << 8)               /* r: port power status
                                              * w: 1-set port power, 0-N/S */
#define RHPS_SET_PORT_POWER RHPS_PPS_FLAG
#define RHPS_LSDA_FLAG (1 << 9)                /* r: low speed device attached
                                                * w: 1-clear port power, 0-N/S*/
#define RHPS_CLEAR_PORT_POWER RHPS_LSDA_FLAG
#define RHPS_CSC_FLAG  (1 << 16) /* connect status change WC */
#define RHPS_PESC_FLAG (1 << 17) /* port enable status change WC */
#define RHPS_PSSC_FLAG (1 << 18) /* port suspend status change WC */
#define RHPS_OCIC_FLAG (1 << 19) /* port over-current change WC */
#define RHPS_PRSC_FLAG (1 << 20) /* port reset status change WC */
#define RHPS_CHANGE_WC_MASK (0x1f0000)
} ohci_regs_t;

/*
 * ohci_regs_t.revision
 */

#define R_REVISION_MASK (0x3f)
#define R_LEGACY_FLAG   (0x80)

/*
 * ohci_regs_t.control
 */

/* Control-bulk service ratio */
#define C_CBSR_1_1  (0x0)
#define C_CBSR_1_2  (0x1)
#define C_CBSR_1_3  (0x2)
#define C_CBSR_1_4  (0x3)
#define C_CBSR_MASK (0x3)
#define C_CBSR_SHIFT 0

#define C_PLE (1 << 2)   /* Periodic list enable */
#define C_IE  (1 << 3)   /* Isochronous enable */
#define C_CLE (1 << 4)   /* Control list enable */
#define C_BLE (1 << 5)   /* Bulk list enable */

/* Host controller functional state */
#define C_HCFS_RESET       (0x0)
#define C_HCFS_RESUME      (0x1)
#define C_HCFS_OPERATIONAL (0x2)
#define C_HCFS_SUSPEND     (0x3)
#define C_HCFS_GET(reg) ((OHCI_RD(reg) >> 6) & 0x3)
#define C_HCFS_SET(reg, value) \
do { \
	uint32_t r = OHCI_RD(reg); \
	r &= ~(0x3 << 6); \
	r |= (value & 0x3) << 6; \
	OHCI_WR(reg, r); \
} while (0)

#define C_IR  (1 << 8)  /* Interrupt routing, make sure it's 0 */
#define C_RWC (1 << 9)  /* Remote wakeup connected, host specific */
#define C_RWE (1 << 10)  /* Remote wakeup enable */

/*
 * ohci_regs_t.command_status
 */

#define CS_HCR (1 << 0)   /* Host controller reset */
#define CS_CLF (1 << 1)   /* Control list filled */
#define CS_BLF (1 << 2)   /* Bulk list filled */
#define CS_OCR (1 << 3)   /* Ownership change request */
#if 0
#define CS_SOC_MASK (0x3) /* Scheduling overrun count */
#define CS_SOC_SHIFT (16)
#endif

/*
 * ohci_regs_t.interrupt_xxx
 */

#define I_SO   (1 << 0)   /* Scheduling overrun */
#define I_WDH  (1 << 1)   /* Done head write-back */
#define I_SF   (1 << 2)   /* Start of frame */
#define I_RD   (1 << 3)   /* Resume detect */
#define I_UE   (1 << 4)   /* Unrecoverable error */
#define I_FNO  (1 << 5)   /* Frame number overflow */
#define I_RHSC (1 << 6)   /* Root hub status change */
#define I_OC   (1 << 30)  /* Ownership change */
#define I_MI   (1 << 31)  /* Master interrupt (any/all) */

/*
 * ohci_regs_t.hcca
 */

#define HCCA_PTR_MASK 0xffffff00 /* HCCA is 256B aligned */

/*
 * ohci_regs_t.fm_interval
 */

#define FMI_FI_MASK (0x3fff) /* Frame interval in bit times (should be 11999)*/
#define FMI_FI_SHIFT (0)
#define FMI_FSMPS_MASK (0x7fff) /* Full speed max packet size */
#define FMI_FSMPS_SHIFT (16)
#define FMI_TOGGLE_FLAG (1 << 31)

/*
 * ohci_regs_t.fm_remaining
 */

#define FMR_FR_MASK FMI_FI_MASK
#define FMR_FR_SHIFT FMI_FI_SHIFT
#define FMR_TOGGLE_FLAG FMI_TOGGLE_FLAG

/*
 * ohci_regs_t.fm_number
 */

#define FMN_NUMBER_MASK (0xffff)

/*
 * ohci_regs_t.periodic_start
 */

#define PS_MASK 0x3fff
#define PS_SHIFT 0

/*
 * ohci_regs_t.ls_threshold
 */

#define LST_LST_MASK (0x7fff)

/*
 * ohci_regs_t.rh_desc_a
 */

/** Number of downstream ports, max 15 */
#define RHDA_NDS_MASK  (0xff)
/** Power switching mode: 0-global, 1-per port */
#define RHDA_PSM_FLAG  (1 << 8)
/** No power switch: 1-power on, 0-use PSM */
#define RHDA_NPS_FLAG  (1 << 9)
/** 1-Compound device, must be 0 */
#define RHDA_DT_FLAG   (1 << 10)
/** Over-current mode: 0-global, 1-per port */
#define RHDA_OCPM_FLAG (1 << 11)
/** OC control: 0-use OCPM, 1-OC off */
#define RHDA_NOCP_FLAG (1 << 12)
/** Power on to power good time */
#define RHDA_POTPGT_SHIFT   24

/*
 * ohci_regs_t.rh_desc_b
 */

/** Device removable mask */
#define RHDB_DR_SHIFT   0
#define RHDB_DR_MASK    0xffffU

/** Power control mask */
#define RHDB_PCC_MASK	0xffffU
#define RHDB_PCC_SHIFT	16

/*
 * ohci_regs_t.rh_status
 */

/*
 * read: 0,
 * write: 0-no effect,
 *        1-turn off port power for ports
 *        specified in PPCM(RHDB), or all ports,
 *        if power is set globally
 */
#define RHS_LPS_FLAG  (1 <<  0)
#define RHS_CLEAR_GLOBAL_POWER RHS_LPS_FLAG /* synonym for the above */
/** Over-current indicator, if per-port: 0 */
#define RHS_OCI_FLAG  (1 <<  1)
/*
 * read: 0-connect status change does not wake HC
 *       1-connect status change wakes HC
 * write: 1-set DRWE, 0-no effect
 */
#define RHS_DRWE_FLAG (1 << 15)
#define RHS_SET_DRWE RHS_DRWE_FLAG
/*
 * read: 0,
 * write: 0-no effect
 *        1-turn on port power for ports
 *        specified in PPCM(RHDB), or all ports,
 *        if power is set globally
 */
#define RHS_LPSC_FLAG (1 << 16)
#define RHS_SET_GLOBAL_POWER RHS_LPSC_FLAG /* synonym for the above */
/** Over-current change indicator */
#define RHS_OCIC_FLAG (1 << 17)
#define RHS_CLEAR_DRWE (1 << 31)

#endif

/*
 * ohci_regs_t.rh_port_status[x]
 */

/** r: current connect status, w: 1-clear port enable, 0-N/S */
#define RHPS_CCS_FLAG (1 << 0)
#define RHPS_CLEAR_PORT_ENABLE RHPS_CCS_FLAG
/** r: port enable status, w: 1-set port enable, 0-N/S */
#define RHPS_PES_FLAG (1 << 1)
#define RHPS_SET_PORT_ENABLE RHPS_PES_FLAG
/** r: port suspend status, w: 1-set port suspend, 0-N/S */
#define RHPS_PSS_FLAG (1 << 2)
#define RHPS_SET_PORT_SUSPEND RHPS_PSS_FLAG
/** r: port over-current (if reports are per-port
 * w: 1-clear port suspend (start resume if suspened), 0-nothing
 */
#define RHPS_POCI_FLAG (1 << 3)
#define RHPS_CLEAR_PORT_SUSPEND RHPS_POCI_FLAG
/** r: port reset status, w: 1-set port reset, 0-N/S */
#define RHPS_PRS_FLAG (1 << 4)
#define RHPS_SET_PORT_RESET RHPS_PRS_FLAG
/** r: port power status, w: 1-set port power, 0-N/S */
#define RHPS_PPS_FLAG (1 << 8)
#define RHPS_SET_PORT_POWER RHPS_PPS_FLAG
/** r: low speed device attached, w: 1-clear port power, 0-N/S */
#define RHPS_LSDA_FLAG (1 << 9)
#define RHPS_CLEAR_PORT_POWER RHPS_LSDA_FLAG
/** connect status change WC */
#define RHPS_CSC_FLAG  (1 << 16)
/** port enable status change WC */
#define RHPS_PESC_FLAG (1 << 17)
/** port suspend status change WC */
#define RHPS_PSSC_FLAG (1 << 18)
/** port over-current change WC */
#define RHPS_OCIC_FLAG (1 << 19)
/** port reset status change WC */
#define RHPS_PRSC_FLAG (1 << 20)
#define RHPS_CHANGE_WC_MASK (0x1f0000)

/**
 * @}
 */
