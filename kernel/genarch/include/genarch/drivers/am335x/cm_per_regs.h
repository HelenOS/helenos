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
 * @brief Texas Instruments AM335x clock module registers.
 */

#ifndef _KERN_AM335X_CM_PER_REGS_H_
#define _KERN_AM335X_CM_PER_REGS_H_

#include <typedefs.h>

typedef struct am335x_cm_per_regs {

	ioport32_t l4ls_clkstctrl;
	ioport32_t l3ls_clkstctrl;
	ioport32_t l4fw_clkstctrl;
	ioport32_t l3_clkstctrl;

	ioport32_t const pad0;

	ioport32_t cpgmac0_clkctrl;
	ioport32_t lcdc_clkctrl;
	ioport32_t usb0_clkctrl;

	ioport32_t const pad1;

	ioport32_t tptc0_clkctrl;
	ioport32_t emif_clkctrl;
	ioport32_t ocmcram_clkctrl;
	ioport32_t gpmc_clkctrl;
	ioport32_t mcasp0_clkctrl;
	ioport32_t uart5_clkctrl;
	ioport32_t mmc0_clkctrl;
	ioport32_t elm_clkctrl;
	ioport32_t i2c2_clkctrl;
	ioport32_t i2c1_clkctrl;
	ioport32_t spi0_clkctrl;
	ioport32_t spi1_clkctrl;

	ioport32_t const pad2[3];

	ioport32_t l4ls_clkctrl;
	ioport32_t l4fw_clkctrl;
	ioport32_t mcasp1_clkctrl;
	ioport32_t uart1_clkctrl;
	ioport32_t uart2_clkctrl;
	ioport32_t uart3_clkctrl;
	ioport32_t uart4_clkctrl;
	ioport32_t timer7_clkctrl;
	ioport32_t timer2_clkctrl;
	ioport32_t timer3_clkctrl;
	ioport32_t timer4_clkctrl;

	ioport32_t const pad3[8];

	ioport32_t gpio1_clkctrl;
	ioport32_t gpio2_clkctrl;
	ioport32_t gpio3_clkctrl;

	ioport32_t const pad4;

	ioport32_t tpcc_clkctrl;
	ioport32_t dcan0_clkctrl;
	ioport32_t dcan1_clkctrl;
	ioport32_t epwmss1_clkctrl;
	ioport32_t emiffw_clkctrl;
	ioport32_t epwmss0_clkctrl;
	ioport32_t epwmss2_clkctrl;
	ioport32_t l3instr_clkctrl;
	ioport32_t l3_clkctrl;
	ioport32_t ieee5000_clkctrl;
	ioport32_t pruicss_clkctrl;
	ioport32_t timer5_clkctrl;
	ioport32_t timer6_clkctrl;
	ioport32_t mmc1_clkctrl;
	ioport32_t mmc2_clkctrl;
	ioport32_t tptc1_clkctrl;
	ioport32_t tptc2_clkctrl;

	ioport32_t const pad5[2];

	ioport32_t spinlock_clkctrl;
	ioport32_t mailbox0_clkctrl;

	ioport32_t const pad6[2];

	ioport32_t l4hs_clkstctrl;
	ioport32_t l4hs_clkctrl;

	ioport32_t const pad7[2];

	ioport32_t ocpwp_l3_clkstctrl;
	ioport32_t ocpwp_clkctrl;

	ioport32_t const pad8[3];

	ioport32_t pruicss_clkstctrl;
	ioport32_t cpsw_clkstctrl;
	ioport32_t lcdc_clkstctrl;
	ioport32_t clkdiv32_clkctrl;
	ioport32_t clk24mhz_clkstctrl;
} am335x_cm_per_regs_t;

#endif

/**
 * @}
 */
