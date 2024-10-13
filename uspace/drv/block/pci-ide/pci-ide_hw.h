/*
 * Copyright (c) 2024 Jiri Svoboda
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

/** @addtogroup pci-ide
 * @{
 */
/** @file PCI IDE hardware protocol (registers, data structures).
 *
 * Based on Intel 82371AB PCI-TO-ISA / IDE XCELERATOR (PIIX4) document.
 */

#ifndef PCI_IDE_HW_H
#define PCI_IDE_HW_H

#include <stddef.h>
#include <stdint.h>

/** PCI Bus Master IDE I/O Registers */
typedef struct {
	/** Bus Master IDE Command (primary) */
	uint8_t bmicp;
	uint8_t rsvd1;
	/** Bus Master IDE Command (secondary) */
	uint8_t bmisp;
	uint8_t rsvd3;
	/** Bus Master IDE Descriptor Table Pointer (primary) */
	uint32_t bmidtpp;
	/** Bus Master IDE Status (secondary) */
	uint8_t bmics;
	uint8_t rsvd9;
	/** Bus Master IDE Status (secondary) */
	uint8_t bmiss;
	uint8_t rsvd11;
	/** Bus Master IDE Descriptor Table Pointer (secondary) */
	uint32_t bmidtps;
} pci_ide_regs_t;

enum pci_ide_bmicx_bits {
	/** Bus Master Read/Write Control */
	bmicx_rwcon = 0x08,
	/** Start/Stop Bus Master */
	bmicx_ssbm = 0x01
};

enum pci_ide_bmisx_bits {
	/** Drive 1 DMA Capable */
	bmisx_dma1cap = 0x40,
	/** Drive 0 DMA Capable */
	bmisx_dma0cap = 0x20,
	/** IDE Interrupte Status */
	bmisx_ideints = 0x04,
	/** IDE DMA Error */
	bmisx_idedmaerr = 0x02,
	/** Bus Master IDR Active */
	bmisx_bmidea = 0x01
};

#define PCI_IDE_CFG_IDETIM 0x40
#define PCI_IDE_CFG_SIDETIM 0x44
#define PCI_IDE_CFG_UDMACTL 0x48
#define PCI_IDE_CFG_UDMATIM 0x4a

/*
 * For PIIX we need to use ATA ports at fixed legacy ISA addresses.
 * There are no corresponding PCI I/O ranges and these adresses are
 * fixed and cannot be reconfigured.
 */
enum {
	pci_ide_ata_cmd_p = 0x01f0,
	pci_ide_ata_ctl_p = 0x03f4,
	pci_ide_ata_cmd_s = 0x0170,
	pci_ide_ata_ctl_s = 0x0374
};

enum {
	pci_ide_prd_eot = 0x8000
};

/** PIIX physical region descriptor */
typedef struct {
	/** Physical base address */
	uint32_t pba;
	/** Byte count */
	uint16_t bcnt;
	/** EOT / reserved */
	uint16_t eot_res;
} pci_ide_prd_t;

#endif

/** @}
 */
