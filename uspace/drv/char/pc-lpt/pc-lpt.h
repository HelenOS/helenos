/*
 * Copyright (c) 2018 Jiri Svoboda
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

/** @addtogroup uspace_drv_pc_lpt
 * @{
 */
/** @file
 */

#ifndef PC_LPT_H
#define PC_LPT_H

#include <async.h>
#include <ddf/driver.h>
#include <ddi.h>
#include <fibril_synch.h>
#include <io/chardev_srv.h>
#include <stdint.h>

#include "pc-lpt_hw.h"

/** PC parallel port resources */
typedef struct {
	uintptr_t base;
	int irq;
} pc_lpt_res_t;

/** PC parallel port */
typedef struct {
	/** DDF device */
	ddf_dev_t *dev;
	/** Character device service structure */
	chardev_srvs_t cds;
	/** Hardware resources */
	pc_lpt_res_t res;
	/** PIO range */
	irq_pio_range_t irq_range[1];
	/** IRQ code */
	irq_code_t irq_code;
	/** Hardware access lock */
	fibril_mutex_t hw_lock;
	/** Hardware registers */
	pc_lpt_regs_t *regs;
	/** IRQ handle */
	cap_irq_handle_t irq_handle;
} pc_lpt_t;

extern errno_t pc_lpt_add(pc_lpt_t *, pc_lpt_res_t *);
extern errno_t pc_lpt_remove(pc_lpt_t *);
extern errno_t pc_lpt_gone(pc_lpt_t *);

#endif

/** @}
 */
