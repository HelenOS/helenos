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
/** @file PCI IDE driver definitions.
 */

#ifndef PCI_IDE_H
#define PCI_IDE_H

#include <ata/ata.h>
#include <ata/ata_hw.h>
#include <ddf/driver.h>
#include <fibril_synch.h>
#include <stdbool.h>
#include <stdint.h>
#include "pci-ide_hw.h"

#define NAME "pci-ide"

/** PCI IDE hardware resources */
typedef struct {
	uintptr_t bmregs; /** PCI Bus Master register block base address. */
	uintptr_t cmd1;	/**< Primary channel command block base address. */
	uintptr_t ctl1;	/**< Primary channel control block base address. */
	uintptr_t cmd2;	/**< Secondary channel command block base address. */
	uintptr_t ctl2;	/**< Secondary channel control block base address. */
	int irq1;	/**< Primary channel IRQ */
	int irq2;	/**< Secondary channel IRQ */
} pci_ide_hwres_t;

/** PCI IDE channel */
typedef struct pci_ide_channel {
	/** Parent controller */
	struct pci_ide_ctrl *ctrl;
	/** I/O base address of the command registers */
	uintptr_t cmd_physical;
	/** I/O base address of the control registers */
	uintptr_t ctl_physical;

	/** Command registers */
	ata_cmd_t *cmd;
	/** Control registers */
	ata_ctl_t *ctl;
	/** IRQ (-1 if not used) */
	int irq;
	/** IRQ handle */
	cap_irq_handle_t ihandle;

	/** Synchronize controller access */
	fibril_mutex_t lock;
	/** Value of status register read by interrupt handler */
	uint8_t irq_status;

	/** Physical region descriptor table */
	pci_ide_prd_t *prdt;
	/** Physical region descriptor table physical address */
	uintptr_t prdt_pa;
	/** DMA buffer */
	void *dma_buf;
	/** DMA buffer physical address */
	uintptr_t dma_buf_pa;
	/** DMA buffer size */
	size_t dma_buf_size;
	/** Current DMA transfer direction */
	ata_dma_dir_t cur_dir;
	/** Current data buffer */
	void *cur_buf;
	/** Current data buffer size */
	size_t cur_buf_size;

	/** Libata ATA channel */
	ata_channel_t *channel;
	struct pci_ide_fun *fun[2];

	/** Channel ID */
	unsigned chan_id;
} pci_ide_channel_t;

/** ISA IDE controller */
typedef struct pci_ide_ctrl {
	/** DDF device */
	ddf_dev_t *dev;

	/** I/O base address of bus master IDE registers */
	uintptr_t bmregs_physical;
	/** Bus master IDE registers */
	pci_ide_regs_t *bmregs;
	/** Primary and secondary channel */
	pci_ide_channel_t channel[2];
} pci_ide_ctrl_t;

/** PCI IDE function */
typedef struct pci_ide_fun {
	ddf_fun_t *fun;
	void *charg;
} pci_ide_fun_t;

extern errno_t pci_ide_ctrl_init(pci_ide_ctrl_t *, pci_ide_hwres_t *);
extern errno_t pci_ide_ctrl_fini(pci_ide_ctrl_t *);
extern errno_t pci_ide_channel_init(pci_ide_ctrl_t *, pci_ide_channel_t *,
    unsigned, pci_ide_hwres_t *);
extern errno_t pci_ide_channel_fini(pci_ide_channel_t *);

#endif

/** @}
 */
