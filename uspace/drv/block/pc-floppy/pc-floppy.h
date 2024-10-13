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

/** @addtogroup pc-floppy
 * @{
 */
/** @file PC floppy disk driver
 */

#ifndef PC_FLOPPY_H
#define PC_FLOPPY_H

#include <bd_srv.h>
#include <ddf/driver.h>
#include <fibril_synch.h>
#include <stdbool.h>
#include <stdint.h>
#include "pc-floppy_hw.h"

#define NAME "pc-floppy"

/** PC floppy controller hardware resources */
typedef struct {
	/** I/O registers */
	uintptr_t regs;
	/** IRQ */
	int irq;
	/** DMA channel */
	int dma;
} pc_fdc_hwres_t;

/** PC floppy disk controller */
typedef struct pc_fdc {
	/** DDF device */
	ddf_dev_t *dev;
	/** I/O base address of the registers */
	uintptr_t regs_physical;

	/** Command registers */
	pc_fdc_regs_t *regs;
	/** IRQ (-1 if not used) */
	int irq;
	/** DMA (-1 if not used) */
	int dma;
	/** IRQ handle */
	cap_irq_handle_t ihandle;

	/** DMA buffer */
	void *dma_buf;
	/** DMA buffer physical address */
	uintptr_t dma_buf_pa;
	/** DMA buffer size */
	size_t dma_buf_size;

	/** Synchronize controller access */
	fibril_mutex_t lock;

	struct pc_fdc_drive *drive[2];
} pc_fdc_t;

/** PC floppy drive */
typedef struct pc_fdc_drive {
	pc_fdc_t *fdc;
	ddf_fun_t *fun;
	void *charg;
	size_t sec_size;
	unsigned cylinders;
	unsigned heads;
	unsigned sectors;
	bd_srvs_t bds;
} pc_fdc_drive_t;

extern errno_t pc_fdc_create(ddf_dev_t *, pc_fdc_hwres_t *, pc_fdc_t **);
extern errno_t pc_fdc_destroy(pc_fdc_t *);

#endif

/** @}
 */
