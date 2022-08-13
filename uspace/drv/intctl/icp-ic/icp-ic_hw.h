/*
 * SPDX-FileCopyrightText: 2014 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup icp-ic
 * @{
 */

#ifndef ICP_IC_HW_H
#define ICP_IC_HW_H

#include <ddi.h>

typedef struct {
	ioport32_t irq_status;
	ioport32_t irq_rawstat;
	ioport32_t irq_enableset;
	ioport32_t irq_enableclr;
	ioport32_t int_softset;
	ioport32_t int_softclr;
	ioport32_t fiq_status;
	ioport32_t fiq_rawstat;
	ioport32_t fiq_enableset;
	ioport32_t fiq_enableclr;
} icpic_regs_t;

#endif

/** @}
 */
