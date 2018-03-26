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

/** @addtogroup amdm37xdrvcorecm
 * @{
 */
/** @file
 * @brief CORE Clock Management IO register structure.
 */
#ifndef AMDM37x_CORE_CM_H
#define AMDM37x_CORE_CM_H

#include <ddi.h>
#include <macros.h>

/* AM/DM37x TRM p.447 */
#define CORE_CM_BASE_ADDRESS  0x48004a00
#define CORE_CM_SIZE  8192

typedef struct {
	ioport32_t fclken1;
#define CORE_CM_FCLKEN1_EN_MCBSP1_FLAG  (1 << 9)
#define CORE_CM_FCLKEN1_EN_MCBSP5_FLAG  (1 << 10)
#define CORE_CM_FCLKEN1_EN_GPT10_FLAG  (1 << 11)
#define CORE_CM_FCLKEN1_EN_GPT11_FLAG  (1 << 12)
#define CORE_CM_FCLKEN1_EN_UART1_FLAG  (1 << 13)
#define CORE_CM_FCLKEN1_EN_UART2_FLAG  (1 << 14)
#define CORE_CM_FCLKEN1_EN_I2C1_FLAG  (1 << 15)
#define CORE_CM_FCLKEN1_EN_I2C2_FLAG  (1 << 16)
#define CORE_CM_FCLKEN1_EN_I2C3_FLAG  (1 << 17)
#define CORE_CM_FCLKEN1_EN_MCSPI1_FLAG  (1 << 18)
#define CORE_CM_FCLKEN1_EN_MCSPI2_FLAG  (1 << 19)
#define CORE_CM_FCLKEN1_EN_MCSPI3_FLAG  (1 << 20)
#define CORE_CM_FCLKEN1_EN_MCSPI4_FLAG  (1 << 21)
#define CORE_CM_FCLKEN1_EN_HDQ_FLAG  (1 << 22)
#define CORE_CM_FCLKEN1_EN_MMC1_FLAG  (1 << 24)
#define CORE_CM_FCLKEN1_EN_MMC2_FLAG  (1 << 25)
#define CORE_CM_FCLKEN1_EN_MMC3_FLAG  (1 << 30)

	PADD32(1);
	ioport32_t fclken3;
#define CORE_CM_FCLKEN3_EN_TS_FLAG  (1 << 1)
#define CORE_CM_FCLKEN3_EN_USBTLL_FLAG  (1 << 2)

	PADD32(1);
	ioport32_t iclken1;
#define CORE_CM_ICLKEN1_EN_SDRC_FLAG  (1 << 1)
#define CORE_CM_ICLKEN1_EN_HSOTGUSB_FLAG  (1 << 4)
#define CORE_CM_ICLKEN1_EN_SCMCTRL_FLAG  (1 << 6)
#define CORE_CM_ICLKEN1_EN_MAILBOXES_FLAG  (1 << 7)
#define CORE_CM_ICLKEN1_EN_MCBSP1_FLAG  (1 << 9)
#define CORE_CM_ICLKEN1_EN_MCBSP5_FLAG  (1 << 10)
#define CORE_CM_ICLKEN1_EN_GPT10_FLAG  (1 << 11)
#define CORE_CM_ICLKEN1_EN_GPT11_FLAG  (1 << 12)
#define CORE_CM_ICLKEN1_EN_UART1_FLAG  (1 << 13)
#define CORE_CM_ICLKEN1_EN_UART2_FLAG  (1 << 14)
#define CORE_CM_ICLKEN1_EN_I2C1_FLAG  (1 << 15)
#define CORE_CM_ICLKEN1_EN_I2C2_FLAG  (1 << 16)
#define CORE_CM_ICLKEN1_EN_I2C3_FLAG  (1 << 17)
#define CORE_CM_ICLKEN1_EN_MCSPI1_FLAG  (1 << 18)
#define CORE_CM_ICLKEN1_EN_MCSPI2_FLAG  (1 << 19)
#define CORE_CM_ICLKEN1_EN_MCSPI3_FLAG  (1 << 20)
#define CORE_CM_ICLKEN1_EN_MCSPI4_FLAG  (1 << 21)
#define CORE_CM_ICLKEN1_EN_HDQ_FLAG  (1 << 22)
#define CORE_CM_ICLKEN1_EN_MMC1_FLAG  (1 << 24)
#define CORE_CM_ICLKEN1_EN_MMC2_FLAG  (1 << 25)
#define CORE_CM_ICLKEN1_EN_ICR_FLAG  (1 << 29)
#define CORE_CM_ICLKEN1_EN_MMC3_FLAG  (1 << 30)

	ioport32_t reserved1;
	ioport32_t iclken3;
#define CORE_CM_ICLKEN3_EN_USBTLL_FLAG  (1 << 2)

	PADD32(1);
	const ioport32_t idlest1;
#define CORE_CM_IDLEST1_ST_SDRC_FLAG  (1 << 1)
#define CORE_CM_IDLEST1_ST_SDMA_FLAG  (1 << 2)
#define CORE_CM_IDLEST1_ST_HSOTGUSB_STBY_FLAG  (1 << 4)
#define CORE_CM_IDLEST1_ST_HSOTGUSB_IDLE_FLAG  (1 << 5)
#define CORE_CM_IDLEST1_ST_SCMCTRL_FLAG  (1 << 6)
#define CORE_CM_IDLEST1_ST_MAILBOXES_FLAG  (1 << 7)
#define CORE_CM_IDLEST1_ST_MCBSP1_FLAG  (1 << 9)
#define CORE_CM_IDLEST1_ST_MCBSP5_FLAG  (1 << 10)
#define CORE_CM_IDLEST1_ST_GPT10_FLAG  (1 << 11)
#define CORE_CM_IDLEST1_ST_GPT11_FLAG  (1 << 12)
#define CORE_CM_IDLEST1_ST_UART1_FLAG  (1 << 13)
#define CORE_CM_IDLEST1_ST_UART2_FLAG  (1 << 14)
#define CORE_CM_IDLEST1_ST_I2C1_FLAG  (1 << 15)
#define CORE_CM_IDLEST1_ST_I2C2_FLAG  (1 << 16)
#define CORE_CM_IDLEST1_ST_I2C3_FLAG  (1 << 17)
#define CORE_CM_IDLEST1_ST_MCSPI1_FLAG  (1 << 18)
#define CORE_CM_IDLEST1_ST_MCSPI2_FLAG  (1 << 19)
#define CORE_CM_IDLEST1_ST_MCSPI3_FLAG  (1 << 20)
#define CORE_CM_IDLEST1_ST_MCSPI4_FLAG  (1 << 21)
#define CORE_CM_IDLEST1_ST_HDQ_FLAG  (1 << 22)
#define CORE_CM_IDLEST1_ST_MMC1_FLAG  (1 << 24)
#define CORE_CM_IDLEST1_ST_MMC2_FLAG  (1 << 25)
#define CORE_CM_IDLEST1_ST_ICR_FLAG  (1 << 29)
#define CORE_CM_IDLEST1_ST_MMC3_FLAG  (1 << 30)

	const ioport32_t reserved2;
	const ioport32_t idlest3;
#define CORE_CM_IDLEST3_ST_USBTLL_FLAG  (1 << 2)

	PADD32(1);
	ioport32_t autoidle1;
#define CORE_CM_AUTOIDLE1_AUTO_HSOTGUSB_FLAG  (1 << 4)
#define CORE_CM_AUTOIDLE1_AUTO_SCMCTRL_FLAG  (1 << 6)
#define CORE_CM_AUTOIDLE1_AUTO_MAILBOXES_FLAG  (1 << 7)
#define CORE_CM_AUTOIDLE1_AUTO_MCBSP1_FLAG  (1 << 9)
#define CORE_CM_AUTOIDLE1_AUTO_MCBSP5_FLAG  (1 << 10)
#define CORE_CM_AUTOIDLE1_AUTO_GPT10_FLAG  (1 << 11)
#define CORE_CM_AUTOIDLE1_AUTO_GPT11_FLAG  (1 << 12)
#define CORE_CM_AUTOIDLE1_AUTO_UART1_FLAG  (1 << 13)
#define CORE_CM_AUTOIDLE1_AUTO_UART2_FLAG  (1 << 14)
#define CORE_CM_AUTOIDLE1_AUTO_I2C1_FLAG  (1 << 15)
#define CORE_CM_AUTOIDLE1_AUTO_I2C2_FLAG  (1 << 16)
#define CORE_CM_AUTOIDLE1_AUTO_I2C3_FLAG  (1 << 17)
#define CORE_CM_AUTOIDLE1_AUTO_MCSPI1_FLAG  (1 << 18)
#define CORE_CM_AUTOIDLE1_AUTO_MCSPI2_FLAG  (1 << 19)
#define CORE_CM_AUTOIDLE1_AUTO_MCSPI3_FLAG  (1 << 20)
#define CORE_CM_AUTOIDLE1_AUTO_MCSPI4_FLAG  (1 << 21)
#define CORE_CM_AUTOIDLE1_AUTO_HDQ_FLAG  (1 << 22)
#define CORE_CM_AUTOIDLE1_AUTO_MMC1_FLAG  (1 << 24)
#define CORE_CM_AUTOIDLE1_AUTO_MMC2_FLAG  (1 << 25)
#define CORE_CM_AUTOIDLE1_AUTO_ICR_FLAG  (1 << 29)
#define CORE_CM_AUTOIDLE1_AUTO_MMC3_FLAG  (1 << 30)

	ioport32_t reserved3;
	ioport32_t autoidle3;
#define CORE_CM_AUTOIDLE3_AUTO_USBTLL_FLAG  (1 << 2)

	PADD32(1);
	ioport32_t clksel;
#define CORE_CM_CLKSEL_CLKSEL_L3_MASK  (0x3 << 0)
#define CORE_CM_CLKSEL_CLKSEL_L3_DIVIDED1  (0x1 << 0)
#define CORE_CM_CLKSEL_CLKSEL_L3_DIVIDED2  (0x2 << 0)
#define CORE_CM_CLKSEL_CLKSEL_L4_MASK  (0x3 << 2)
#define CORE_CM_CLKSEL_CLKSEL_L4_DIVIDED1  (0x1 << 2)
#define CORE_CM_CLKSEL_CLKSEL_L4_DIVIDED2  (0x2 << 2)
#define CORE_CM_CLKSEL_CLKSEL_96M_MASK  (0x3 << 12)
#define CORE_CM_CLKSEL_CLKSEL_96M_DIVIDED1  (0x1 << 12)
#define CORE_CM_CLKSEL_CLKSEL_96M_DIVIDED2  (0x2 << 12)
#define CORE_CM_CLKSEL_CLKSEL_GPT10_FLAG (1 << 6)
#define CORE_CM_CLKSEL_CLKSEL_GPT11_FLAG (1 << 7)

	PADD32(1);
	ioport32_t clkstctrl;
#define CORE_CM_CLKCTRL_CLKCTRL_L3_MASK  (0x3 << 0)
#define CORE_CM_CLKCTRL_CLKCTRL_L3_AUTO_EN  (0x0 << 0)
#define CORE_CM_CLKCTRL_CLKCTRL_L3_AUTO_DIS  (0x3 << 0)
#define CORE_CM_CLKCTRL_CLKCTRL_L4_MASK  (0x3 << 2)
#define CORE_CM_CLKCTRL_CLKCTRL_L4_AUTO_EN  (0x0 << 2)
#define CORE_CM_CLKCTRL_CLKCTRL_L4_AUTO_DIS  (0x3 << 2)

	const ioport32_t clkstst;
#define CORE_CM_CLKSTST_CLKACTIVITY_L3_FLAG  (1 << 0)
#define CORE_CM_CLKSTST_CLKACTIVITY_L4_FLAG  (1 << 1)
} core_cm_regs_t;

#endif
/**
 * @}
 */
