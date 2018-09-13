/*
 * Copyright (c) 2012 Jan Vesely
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

/** @addtogroup amdm37x
 * @{
 */
/** @file
 * @brief Clock Control Clock Management IO register structure.
 */
#ifndef AMDM37X_PRM_CLOCK_CONTROL_H
#define AMDM37X_PRM_CLOCK_CONTROL_H

#include <ddi.h>
#include <macros.h>

/* AM/DM37x TRM p.536 and p.589 */
#define CLOCK_CONTROL_PRM_BASE_ADDRESS  0x48306d00
#define CLOCK_CONTROL_PRM_SIZE  8192

/** Clock control PRM register map
 */
typedef struct {
	PADD32(16);
	ioport32_t clksel;
#define CLOCK_CONTROL_PRM_CLKSEL_SYS_CLKIN_MASK   (0x7)
#define CLOCK_CONTROL_PRM_CLKSEL_SYS_CLKIN_12M   (0x0)
#define CLOCK_CONTROL_PRM_CLKSEL_SYS_CLKIN_13M   (0x1)
#define CLOCK_CONTROL_PRM_CLKSEL_SYS_CLKIN_19_2M   (0x2)
#define CLOCK_CONTROL_PRM_CLKSEL_SYS_CLKIN_26M   (0x3)
#define CLOCK_CONTROL_PRM_CLKSEL_SYS_CLKIN_38_4M   (0x4)
#define CLOCK_CONTROL_PRM_CLKSEL_SYS_CLKIN_16_8M   (0x5)

	PADD32(12);
	ioport32_t clkout_ctrl;
#define CLOCK_CONTROL_PRM_CLKOUT_CTRL_CLKOUOUT_EN_FLAG   (1 << 7)

} clock_control_prm_regs_t;

static inline unsigned sys_clk_freq_kHz(unsigned reg_val)
{
	switch (reg_val) {
	case CLOCK_CONTROL_PRM_CLKSEL_SYS_CLKIN_12M:
		return 12000;
	case CLOCK_CONTROL_PRM_CLKSEL_SYS_CLKIN_13M:
		return 13000;
	case CLOCK_CONTROL_PRM_CLKSEL_SYS_CLKIN_19_2M:
		return 19200;
	case CLOCK_CONTROL_PRM_CLKSEL_SYS_CLKIN_26M:
		return 26000;
	case CLOCK_CONTROL_PRM_CLKSEL_SYS_CLKIN_38_4M:
		return 38400;
	case CLOCK_CONTROL_PRM_CLKSEL_SYS_CLKIN_16_8M:
		return 16800;
	}
	return 0;
}

#endif
/**
 * @}
 */
