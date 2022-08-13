/*
 * SPDX-FileCopyrightText: 2013 Maurizio Lombardi
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
/** @addtogroup kernel_genarch
 * @{
 */
/**
 * @file
 * @brief Texas Instruments AM335x DPLL.
 */

#ifndef _KERN_AM335X_CM_DPLL_H_
#define _KERN_AM335X_CM_DPLL_H_

#include "cm_dpll_regs.h"
#include "timer.h"

#define AM335x_CM_DPLL_BASE_ADDRESS   0x44E00500
#define AM335x_CM_DPLL_SIZE           256

static ioport32_t *am335x_cm_dpll_timer_reg_get(am335x_cm_dpll_regs_t *cm,
    am335x_timer_id_t id)
{
	switch (id) {
	default:
		return NULL;
	case DMTIMER2:
		return &cm->clksel_timer2;
	case DMTIMER3:
		return &cm->clksel_timer3;
	case DMTIMER4:
		return &cm->clksel_timer4;
	case DMTIMER5:
		return &cm->clksel_timer5;
	case DMTIMER6:
		return &cm->clksel_timer6;
	case DMTIMER7:
		return &cm->clksel_timer7;
	}
}

static void am335x_clock_source_select(am335x_cm_dpll_regs_t *cm,
    am335x_timer_id_t id, am335x_clk_src_t src)
{
	ioport32_t *reg = am335x_cm_dpll_timer_reg_get(cm, id);
	if (!reg)
		return;

	*reg = (*reg & ~0x03) | src;
}

#endif

/**
 * @}
 */
