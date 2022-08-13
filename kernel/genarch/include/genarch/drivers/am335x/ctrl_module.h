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
 * @brief Texas Instruments AM335x control module.
 */

#ifndef _KERN_AM335X_CTRL_MODULE_H_
#define _KERN_AM335X_CTRL_MODULE_H_

#include <errno.h>
#include <typedefs.h>
#include "ctrl_module_regs.h"

#define AM335x_CTRL_MODULE_BASE_ADDRESS  0x44E10000
#define AM335x_CTRL_MODULE_SIZE          131072 /* 128 Kb */

typedef ioport32_t am335x_ctrl_module_t;

static errno_t
am335x_ctrl_module_clock_freq_get(am335x_ctrl_module_t *base, unsigned *freq)
{
	unsigned const control_status = AM335x_CTRL_MODULE_REG_VALUE(base,
	    CONTROL_STATUS);

	/* Get the sysboot1 field at control_status[22,23] */
	unsigned const sysboot1 = (control_status >> 22) & 0x03;

	switch (sysboot1) {
	default:
		return EINVAL;
	case 0:
		*freq = 19200000; /* 19.2 Mhz */
		break;
	case 1:
		*freq = 24000000; /* 24 Mhz */
		break;
	case 2:
		*freq = 25000000; /* 25 Mhz */
		break;
	case 3:
		*freq = 26000000; /* 26 Mhz */
		break;
	}

	return EOK;
}

#endif

/**
 * @}
 */
