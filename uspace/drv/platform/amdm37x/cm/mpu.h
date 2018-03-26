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

/** @addtogroup amdm37xdrvcm
 * @{
 */
/** @file
 * @brief MPU Clock Management IO register structure.
 */
#ifndef AMDM37x_MPU_CM_H
#define AMDM37x_MPU_CM_H

#include <ddi.h>
#include <macros.h>

/* AM/DM37x TRM p.455 */
#define MPU_CM_BASE_ADDRESS  0x48004900
#define MPU_CM_SIZE  8192

typedef struct {
	PADD32(1);
	ioport32_t clken_pll;
#define MPU_CM_CLKEN_PLL_EN_MPU_DPLL_LP_MODE_FLAG   (1 << 10)
#define MPU_CM_CLKEN_PLL_EN_MPU_DPLL_DRIFTGUARD   (1 << 3)
#define MPU_CM_CLKEN_PLL_EN_MPU_DPLL_EN_MPU_DPLL_MASK   (0x7)
#define MPU_CM_CLKEN_PLL_EN_MPU_DPLL_EN_MPU_DPLL_LP_BYPASS   (0x5)
#define MPU_CM_CLKEN_PLL_EN_MPU_DPLL_EN_MPU_DPLL_LOCKED   (0x7)

	PADD32(6);
	const ioport32_t idlest;
#define MPU_CM_IDLEST_ST_MPU_STANDBY_FLAG   (1 << 0)

	const ioport32_t idlest_pll;
#define MPU_CM_IDLEST_PLL_ST_MPU_CLK_LOCKED_FLAG   (1 << 0)

	PADD32(3);
	ioport32_t autoidle_pll;
#define MPU_CM_AUTOIDLE_PLL_AUTO_MPU_DPLL_MASK   (0x7)
#define MPU_CM_AUTOIDLE_PLL_AUTO_MPU_DPLL_DISABLED   (0x0)
#define MPU_CM_AUTOIDLE_PLL_AUTO_MPU_DPLL_ENABLED   (0x1)

	PADD32(2);
	ioport32_t clksel1_pll;
#define MPU_CM_CLKSEL1_PLL_MPU_CLK_SRC_MASK   (0x7 << 19)
#define MPU_CM_CLKSEL1_PLL_MPU_CLK_SRC_SHIFT   (19)
#define MPU_CM_CLKSEL1_PLL_MPU_CLK_SRC_VAL(x)   ((x >> 19) & 0x7)
#define MPU_CM_CLKSEL1_PLL_MPU_CLK_SRC_CORE_DIV_1   (0x1 << 19)
#define MPU_CM_CLKSEL1_PLL_MPU_CLK_SRC_CORE_DIV_2   (0x2 << 19)
#define MPU_CM_CLKSEL1_PLL_MPU_CLK_SRC_CORE_DIV_4   (0x4 << 19)
#define MPU_CM_CLKSEL1_PLL_MPU_DPLL_MULT_MASK   (0x7ff << 8)
#define MPU_CM_CLKSEL1_PLL_MPU_DPLL_MULT_SHIFT   (8)
#define MPU_CM_CLKSEL1_PLL_MPU_DPLL_DIV_MASK  (0x7f << 0)
#define MPU_CM_CLKSEL1_PLL_MPU_DPLL_DIV_SHIFT  (0)

	ioport32_t clksel2_pll;
#define MPU_CM_CLKSEL2_PLL_MPU_DPLL_CLKOUT_DIV_MASK   (0x1f)

	ioport32_t clkstctrl;
#define MPU_CM_CLKSCTRL_CLKTRCTRL_MPU_MASK   (0x3)
#define MPU_CM_CLKSCTRL_CLKTRCTRL_MPU_DISABLED   (0x0)
#define MPU_CM_CLKSCTRL_CLKTRCTRL_MPU_START_WAKEUP   (0x2)
#define MPU_CM_CLKSCTRL_CLKTRCTRL_MPU_AUTOMATIC   (0x3)

	const ioport32_t clkstst;
#define MPU_CM_CLKSTST_CLKACTIVITY_MPU_ACTIVE_FLAG   (1 << 0)

} mpu_cm_regs_t;

#endif
/**
 * @}
 */

