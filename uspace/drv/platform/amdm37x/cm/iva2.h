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
#ifndef AMDM37x_IVA2_CM_H
#define AMDM37x_IVA2_CM_H

#include <ddi.h>
#include <macros.h>

/* AM/DM37x TRM p.446 */
#define IVA2_CM_BASE_ADDRESS  0x48004000
#define IVA2_CM_SIZE  8192

typedef struct {
	ioport32_t fclken;
#define IVA2_CM_FCLKEN_EN_IVA2_FLAG   (1 << 0)

	ioport32_t clken_pll;
#define IVA2_CM_CLKEN_PLL_EN_IVA2_DPLL_LP_MODE_FLAG   (1 << 10)
#define IVA2_CM_CLKEN_PLL_EN_IVA2_DPLL_DRIFTGUARD   (1 << 3)
#define IVA2_CM_CLKEN_PLL_EN_IVA2_DPLL_EN_IVA2_DPLL_MASK   (0x7)
#define IVA2_CM_CLKEN_PLL_EN_IVA2_DPLL_EN_IVA2_DPLL_LP_STOP   (0x1)
#define IVA2_CM_CLKEN_PLL_EN_IVA2_DPLL_EN_IVA2_DPLL_LP_BYPASS   (0x5)
#define IVA2_CM_CLKEN_PLL_EN_IVA2_DPLL_EN_IVA2_DPLL_LOCKED   (0x7)

	PADD32(6);
	const ioport32_t idlest;
#define IVA2_CM_IDLEST_ST_IVA2_STANDBY_FLAG   (1 << 0)

	const ioport32_t idlest_pll;
#define IVA2_CM_IDLEST_PLL_ST_IVA2_CLK_LOCKED_FLAG   (1 << 0)

	PADD32(3);
	ioport32_t autoidle_pll;
#define IVA2_CM_AUTOIDLE_PLL_AUTO_IVA2_DPLL_MASK   (0x7)
#define IVA2_CM_AUTOIDLE_PLL_AUTO_IVA2_DPLL_DISABLED   (0x0)
#define IVA2_CM_AUTOIDLE_PLL_AUTO_IVA2_DPLL_ENABLED   (0x1)

	PADD32(2);
	ioport32_t clksel1_pll;
#define IVA2_CM_CLKSEL1_PLL_IVA2_CLK_SRC_MASK   (0x7 << 19)
#define IVA2_CM_CLKSEL1_PLL_IVA2_CLK_SRC_SHIFT   (19)
#define IVA2_CM_CLKSEL1_PLL_IVA2_CLK_SRC_VAL(x)   ((x >> 19) & 0x7)
#define IVA2_CM_CLKSEL1_PLL_IVA2_CLK_SRC_CORE_DIV_1   (0x1 << 19)
#define IVA2_CM_CLKSEL1_PLL_IVA2_CLK_SRC_CORE_DIV_2   (0x2 << 19)
#define IVA2_CM_CLKSEL1_PLL_IVA2_CLK_SRC_CORE_DIV_4   (0x4 << 19)
#define IVA2_CM_CLKSEL1_PLL_IVA2_DPLL_MULT_MASK   (0x7ff << 8)
#define IVA2_CM_CLKSEL1_PLL_IVA2_DPLL_MULT_SHIFT   (8)
#define IVA2_CM_CLKSEL1_PLL_IVA2_DPLL_DIV_MASK  (0x7f << 0)
#define IVA2_CM_CLKSEL1_PLL_IVA2_DPLL_DIV_SHIFT  (0)

	ioport32_t clksel2_pll;
#define IVA2_CM_CLKSEL2_PLL_IVA2_DPLL_CLKOUT_DIV_MASK   (0x1f)

	ioport32_t clkstctrl;
#define IVA2_CM_CLKSCTRL_CLKTRCTRL_IVA2_MASK   (0x3)
#define IVA2_CM_CLKSCTRL_CLKTRCTRL_IVA2_DISABLED   (0x0)
#define IVA2_CM_CLKSCTRL_CLKTRCTRL_IVA2_START_SLEEP   (0x2)
#define IVA2_CM_CLKSCTRL_CLKTRCTRL_IVA2_START_WAKEUP   (0x2)
#define IVA2_CM_CLKSCTRL_CLKTRCTRL_IVA2_AUTOMATIC   (0x3)

	const ioport32_t clkstst;
#define IVA2_CM_CLKSTST_CLKACTIVITY_IVA2_ACTIVE_FLAG   (1 << 0)

} iva2_cm_regs_t;

#endif
/**
 * @}
 */
