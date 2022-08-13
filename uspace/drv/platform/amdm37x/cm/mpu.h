/*
 * SPDX-FileCopyrightText: 2012 Jan Vesely
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup amdm37x
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
