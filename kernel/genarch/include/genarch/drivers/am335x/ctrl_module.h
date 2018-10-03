/*
 * Copyright (c) 2013 Maurizio Lombardi
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
