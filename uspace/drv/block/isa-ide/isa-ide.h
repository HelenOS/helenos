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

/** @addtogroup isa-ide
 * @{
 */
/** @file ISA IDE driver definitions.
 */

#ifndef ISA_IDE_H
#define ISA_IDE_H

#include <ata/ata.h>
#include <ata/ata_hw.h>
#include <ddf/driver.h>
#include <fibril_synch.h>
#include <stdbool.h>
#include <stdint.h>

#define NAME "isa-ide"

/** ISA IDE hardware resources */
typedef struct {
	uintptr_t cmd1;	/**< Primary channel command block base address. */
	uintptr_t ctl1;	/**< Primary channel control block base address. */
	uintptr_t cmd2;	/**< Secondary channel command block base address. */
	uintptr_t ctl2;	/**< Secondary channel control block base address. */
	int irq1;	/**< Primary channel IRQ */
	int irq2;	/**< Secondary channel IRQ */
} isa_ide_hwres_t;

/** ISA IDE channel */
typedef struct isa_ide_channel {
	/** Parent controller */
	struct isa_ide_ctrl *ctrl;
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

	/** Libata ATA channel */
	ata_channel_t *channel;
	struct isa_ide_fun *fun[2];

	/** Channel ID */
	unsigned chan_id;
} isa_ide_channel_t;

/** ISA IDE controller */
typedef struct isa_ide_ctrl {
	/** DDF device */
	ddf_dev_t *dev;

	/** Primary and secondary channel */
	isa_ide_channel_t channel[2];
} isa_ide_ctrl_t;

/** ISA IDE function */
typedef struct isa_ide_fun {
	ddf_fun_t *fun;
	void *charg;
} isa_ide_fun_t;

extern errno_t isa_ide_channel_init(isa_ide_ctrl_t *, isa_ide_channel_t *,
    unsigned, isa_ide_hwres_t *);
extern errno_t isa_ide_channel_fini(isa_ide_channel_t *);

#endif

/** @}
 */
