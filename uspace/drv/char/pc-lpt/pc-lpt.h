/*
 * SPDX-FileCopyrightText: 2018 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
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
