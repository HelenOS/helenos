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
 * @brief Texas Instruments AM335x clock module.
 */

#ifndef _KERN_AM335X_CM_PER_H_
#define _KERN_AM335X_CM_PER_H_

#include <typedefs.h>
#include "cm_per_regs.h"
#include "timer.h"

#define AM335x_CM_PER_BASE_ADDRESS   0x44E00000
#define AM335x_CM_PER_SIZE           1024

static ioport32_t *am335x_clock_clkctrl_reg_get(am335x_cm_per_regs_t *cm,
    am335x_timer_id_t id)
{
	switch (id) {
	default:
		return NULL;
	case DMTIMER2:
		return &cm->timer2_clkctrl;
	case DMTIMER3:
		return &cm->timer3_clkctrl;
	case DMTIMER4:
		return &cm->timer4_clkctrl;
	case DMTIMER5:
		return &cm->timer5_clkctrl;
	case DMTIMER6:
		return &cm->timer6_clkctrl;
	case DMTIMER7:
		return &cm->timer7_clkctrl;
	}

}

static void am335x_clock_module_enable(am335x_cm_per_regs_t *cm,
    am335x_timer_id_t timer_id)
{
	ioport32_t *tmr_reg = am335x_clock_clkctrl_reg_get(cm, timer_id);
	if (tmr_reg == NULL)
		return;

	/* Enable the clock module */
	*tmr_reg = (*tmr_reg & ~0x03) | 0x02;

	/* Wait for completion */
	while ((*tmr_reg & 0x03) != 0x02)
		;
}

#endif

/**
 * @}
 */
