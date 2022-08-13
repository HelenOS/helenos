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
 * @brief Texas Instruments AM335x DPLL registers.
 */

#ifndef _KERN_AM335X_CM_DPLL_REGS_H_
#define _KERN_AM335X_CM_DPLL_REGS_H_

typedef enum {
	CLK_SRC_TCLKIN = 0x00,
	CLK_SRC_M_OSC,
	CLK_SRC_32_KHZ
} am335x_clk_src_t;

typedef struct am335x_cm_dpll_regs {

	ioport32_t const pad0;

	ioport32_t clksel_timer7;
	ioport32_t clksel_timer2;
	ioport32_t clksel_timer3;
	ioport32_t clksel_timer4;

	ioport32_t clksel_mac;
#define AM335x_CM_CLKSEL_MII_FLAG (1 << 2)

	ioport32_t clksel_timer5;
	ioport32_t clksel_timer6;

	ioport32_t clksel_cpts_rft;

	ioport32_t const pad1;

	ioport32_t clksel_timer1ms;
#define AM335x_CM_CLKSEL_TIMER1MS_CLKMOSC      0x00
#define AM335x_CM_CLKSEL_TIMER1MS_CLK32KHZ     0x01
#define AM335x_CM_CLKSEL_TIMER1MS_TCLKIN       0x02
#define AM335x_CM_CLKSEL_TIMER1MS_CLKRC32KHZ   0x03
#define AM335x_CM_CLKSEL_TIMER1MS_32KHZCRYSTAL 0x04

	ioport32_t clksel_gfx_fclk;
#define AM335x_CM_CLKSEL_GFX_FCLK_CLKDIV_FLAG  (1 << 0)
#define AM335x_CM_CLKSEL_GFX_FCLK_CLKSEL_FLAG  (1 << 1)

	ioport32_t clksel_pru_icss_ocp;
	ioport32_t clksel_lcdc_pixel;
	ioport32_t clksel_wdt1;
	ioport32_t clksel_gpio0_db;

} am335x_cm_dpll_regs_t;

#endif

/**
 * @}
 */
