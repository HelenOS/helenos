/*
 * Copyright (c) 2012 Maurizio Lombardi
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
 * @brief Texas Instruments AM335x DMTIMER memory mapped registers.
 */

#ifndef _KERN_AM335X_TIMER_REGS_H_
#define _KERN_AM335X_TIMER_REGS_H_

#include <typedefs.h>

typedef struct am335x_timer_regs {

	/*
	 * This read only register contains the
	 * revision number of the module
	 */
	ioport32_t const tidr;
#define AM335x_TIMER_TIDR_MINOR_MASK     0x3F
#define AM335x_TIMER_TIDR_MINOR_SHIFT    0
#define AM335x_TIMER_TIDR_CUSTOM_MASK    0x03
#define AM335x_TIMER_TIDR_CUSTOM_SHIFT   6
#define AM335x_TIMER_TIDR_MAJOR_MASK     0x07
#define AM335x_TIMER_TIDR_MAJOR_SHIFT    8
#define AM335x_TIMER_TIDR_RTL_MASK       0x1F
#define AM335x_TIMER_TIDR_RTL_SHIFT      11
#define AM335x_TIMER_TIDR_FUNC_MASK      0xFFF
#define AM335x_TIMER_TIDR_FUNC_SHIFT     16
#define AM335x_TIMER_TIDR_SCHEME_MASK    0x03
#define AM335x_TIMER_TIDR_SCHEME_SHIFT   30

	ioport32_t const pad1[3];

	/*
	 * This register allows controlling various
	 * parameters of the OCP interface.
	 */
	ioport32_t tiocp_cfg;
#define AM335x_TIMER_TIOCPCFG_SOFTRESET_FLAG    (1 << 0)
#define AM335x_TIMER_TIOCPCFG_EMUFREE_FLAG      (1 << 1)

#define AM335x_TIMER_TIOCPCFG_IDLEMODE_MASK            0x02
#define AM335x_TIMER_TIOCPCFG_IDLEMODE_SHIFT           2
#  define AM335x_TIMER_TIOCCPCFG_IDLEMODE_FORCE        0x00
#  define AM335x_TIMER_TIOCCPCFG_IDLEMODE_DISABLED     0x01
#  define AM335x_TIMER_TIOCCPCFG_IDLEMODE_SMART        0x02
#  define AM335x_TIMER_TIOCCPCFG_IDLEMODE_SMART_WAKEUP 0x03

	ioport32_t const pad2[4];

	ioport32_t irqstatus_raw;
#define AM335x_TIMER_IRQSTATUS_RAW_MAT_FLAG     (1 << 0)
#define AM335x_TIMER_IRQSTATUS_RAW_OVF_FLAG     (1 << 1)
#define AM335x_TIMER_IRQSTATUS_RAW_TCAR_FLAG    (1 << 2)

	ioport32_t irqstatus;
#define AM335x_TIMER_IRQSTATUS_MAT_FLAG     (1 << 0)
#define AM335x_TIMER_IRQSTATUS_OVF_FLAG     (1 << 1)
#define AM335x_TIMER_IRQSTATUS_TCAR_FLAG    (1 << 2)

	ioport32_t irqenable_set;
#define AM335x_TIMER_IRQENABLE_SET_MAT_FLAG (1 << 0)
#define AM335x_TIMER_IRQENABLE_SET_OVF_FLAG (1 << 1)
#define AM335x_TIMER_IRQENABLE_SET_TCAR_FLAG (1 << 2)

	ioport32_t irqenable_clr;
#define AM335x_TIMER_IRQENABLE_CLR_MAT_FLAG (1 << 0)
#define AM335x_TIMER_IRQENABLE_CLR_OVF_FLAG (1 << 1)
#define AM335x_TIMER_IRQENABLE_CLR_TCAR_FLAG (1 << 2)

	/* Timer IRQ wakeup enable register */
	ioport32_t irqwakeen;
#define AM335x_TIMER_IRQWAKEEN_MAT_FLAG     (1 << 0)
#define AM335x_TIMER_IRQWAKEEN_OVF_FLAG     (1 << 1)
#define AM335x_TIMER_IRQWAKEEN_TCAR_FLAG    (1 << 2)

	/* Timer control register */
	ioport32_t tclr;
#define AM335x_TIMER_TCLR_ST_FLAG           (1 << 0)
#define AM335x_TIMER_TCLR_AR_FLAG           (1 << 1)
#define AM335x_TIMER_TCLR_PTV_MASK          0x07
#define AM335x_TIMER_TCLR_PTV_SHIFT         2
#define AM335x_TIMER_TCLR_PRE_FLAG          (1 << 5)
#define AM335x_TIMER_TCLR_CE_FLAG           (1 << 6)
#define AM335x_TIMER_TCLR_SCPWM_FLAG        (1 << 7)
#define AM335x_TIMER_TCLR_TCM_MASK          0x03
#define AM335x_TIMER_TCLR_TCM_SHIFT         8
#define AM335x_TIMER_TCLR_TGR_MASK          0x03
#define AM335x_TIMER_TCLR_TGR_SHIFT         10
#define AM335x_TIMER_TCLR_PT_FLAG           (1 << 12)
#define AM335x_TIMER_TCLR_CAPT_MODE_FLAG    (1 << 13)
#define AM335x_TIMER_TCLR_GPO_CFG_FLAG      (1 << 14)

	/* Timer counter register */
	ioport32_t tcrr;

	/* Timer load register */
	ioport32_t tldr;

	/* Timer trigger register */
	ioport32_t const ttgr;

	/* Timer write posted status register */
	ioport32_t twps;
#define AM335x_TIMER_TWPS_PEND_TCLR         (1 << 0)
#define AM335x_TIMER_TWPS_PEND_TCRR         (1 << 1)
#define AM335x_TIMER_TWPS_PEND_TLDR         (1 << 2)
#define AM335x_TIMER_TWPS_PEND_TTGR         (1 << 3)
#define AM335x_TIMER_TWPS_PEND_TMAR         (1 << 4)

	/* Timer match register */
	ioport32_t tmar;

	/* Timer capture register */
	ioport32_t tcar1;

	/* Timer synchronous interface control register */
	ioport32_t tsicr;
#define AM335x_TIMER_TSICR_SFT_FLAG         (1 << 1)
#define AM335x_TIMER_TSICR_POSTED_FLAG      (1 << 2)

	/* Timer capture register */
	ioport32_t tcar2;

} am335x_timer_regs_t;

#endif

/**
 * @}
 */
