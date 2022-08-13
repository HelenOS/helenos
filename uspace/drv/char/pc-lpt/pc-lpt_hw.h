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

#ifndef PC_LPT_HW_H
#define PC_LPT_HW_H

#include <ddi.h>

/** PC parallel port registers */
typedef struct {
	/** Data out register */
	ioport8_t data;
	/** Status register */
	ioport8_t status;
	/** Control register */
	ioport8_t control;
} pc_lpt_regs_t;

/** Printer control register bits */
typedef enum {
	/** Strobe */
	ctl_strobe = 0,
	/** Auto linefeed */
	ctl_autofd = 1,
	/** -Init */
	ctl_ninit = 2,
	/** Select */
	ctl_select = 3,
	/** IRQ Enable */
	ctl_irq_enable = 4
} pc_lpt_ctl_bits_t;

/** Printer status register bits */
typedef enum {
	/** -Error */
	sts_nerror = 3,
	/** Select */
	sts_select = 4,
	/** Init */
	sts_paper_end = 5,
	/** -Ack */
	sts_nack = 6,
	/** -Busy */
	sts_nbusy = 7
} pc_lpt_sts_bits_t;

#endif

/** @}
 */
