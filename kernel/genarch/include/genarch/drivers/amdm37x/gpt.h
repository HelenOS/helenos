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
/** @addtogroup genarch
 * @{
 */
/**
 * @file
 * @brief Texas Instruments AM/DM37x MPU general purpose timer driver.
 */

#ifndef KERN_AMDM37x_GPT_H_
#define KERN_AMDM37x_GPT_H_

#include <assert.h>
#include <typedefs.h>
#include <mm/km.h>
#include <time/clock.h>

/* AMDM37x TRM p. 2740 */
#define AMDM37x_GPT1_BASE_ADDRESS  0x48318000
#define AMDM37x_GPT1_SIZE  4096
#define AMDM37x_GPT1_IRQ  37
#define AMDM37x_GPT2_BASE_ADDRESS  0x49032000
#define AMDM37x_GPT2_SIZE  4096
#define AMDM37x_GPT2_IRQ  38
#define AMDM37x_GPT3_BASE_ADDRESS  0x49034000
#define AMDM37x_GPT3_SIZE  4096
#define AMDM37x_GPT3_IRQ  39
#define AMDM37x_GPT4_BASE_ADDRESS  0x49036000
#define AMDM37x_GPT4_SIZE  4096
#define AMDM37x_GPT4_IRQ  40
#define AMDM37x_GPT5_BASE_ADDRESS  0x49038000
#define AMDM37x_GPT5_SIZE  4096
#define AMDM37x_GPT5_IRQ  41
#define AMDM37x_GPT6_BASE_ADDRESS  0x4903a000
#define AMDM37x_GPT6_SIZE  4096
#define AMDM37x_GPT6_IRQ  42
#define AMDM37x_GPT7_BASE_ADDRESS  0x4903c000
#define AMDM37x_GPT7_SIZE  4096
#define AMDM37x_GPT7_IRQ  43
#define AMDM37x_GPT8_BASE_ADDRESS  0x4903e000
#define AMDM37x_GPT8_SIZE  4096
#define AMDM37x_GPT8_IRQ  44
#define AMDM37x_GPT9_BASE_ADDRESS  0x49040000
#define AMDM37x_GPT9_SIZE  4096
#define AMDM37x_GPT9_IRQ  45
#define AMDM37x_GPT10_BASE_ADDRESS  0x48086000
#define AMDM37x_GPT10_SIZE  4096
#define AMDM37x_GPT10_IRQ  46
#define AMDM37x_GPT11_BASE_ADDRESS  0x48088000
#define AMDM37x_GPT11_SIZE  4096
#define AMDM37x_GPT11_IRQ  47


/** GPT register map AMDM37x TRM p. 2740 */
typedef struct {
	/** IP revision */
	const ioport32_t tidr;
#define AMDM37x_GPT_TIDR_MINOR_MASK  (0xf)
#define AMDM37x_GPT_TIDR_MINOR_SHIFT  (0)
#define AMDM37x_GPT_TIDR_MAJOR_MASK  (0xf)
#define AMDM37x_GPT_TIDR_MAJOR_SHIFT  (4)
	uint32_t padd0_[3];

	/** L4 Interface parameters */
	ioport32_t tiocp_cfg;
#define AMDM37x_GPT_TIOCP_CFG_AUTOIDLE_FLAG  (1 << 0)
#define AMDM37x_GPT_TIOCP_CFG_SOFTRESET_FLAG  (1 << 1)
#define AMDM37x_GPT_TIOCP_CFG_ENWAKEUP_FLAG  (1 << 2)
#define AMDM37x_GPT_TIOCP_CFG_IDLEMODE_MASK  (0x3)
#define AMDM37x_GPT_TIOCP_CFG_IDLEMODE_SHIFT  (3)
#define AMDM37x_GPT_TIOCP_CFG_EMUFREE_FlAG  (1 << 5)
#define AMDM37x_GPT_TIOCP_CFG_CLOCKACTIVITY_MASK  (0x3)
#define AMDM37x_GPT_TIOCP_CFG_CLOCKACTIVITY_SHIFT (8)

	/** Module status information, excluding irq */
	const ioport32_t tistat;
#define AMDM37x_GPT_TISTAT_RESET_DONE_FLAG  (1 << 0)

	/** Interrupt status register */
	ioport32_t tisr;
#define AMDM37x_GPT_TISR_MAT_IRQ_FLAG  (1 << 0)
#define AMDM37x_GPT_TISR_OVF_IRQ_FLAG  (1 << 1)
#define AMDM37x_GPT_TISR_TCAR_IRQ_FLAG  (1 << 2)

	/* Interrupt enable register */
	ioport32_t tier;
#define AMDM37x_GPT_TIER_MAT_IRQ_FLAG  (1 << 0)
#define AMDM37x_GPT_TIER_OVF_IRQ_FLAG  (1 << 1)
#define AMDM37x_GPT_TIER_TCAR_IRQ_FLAG  (1 << 2)

	/** Wakeup enable register */
	ioport32_t twer;
#define AMDM37x_GPT_TWER_MAT_IRQ_FLAG  (1 << 0)
#define AMDM37x_GPT_TWER_OVF_IRQ_FLAG  (1 << 1)
#define AMDM37x_GPT_TWER_TCAR_IRQ_FLAG  (1 << 2)

	/** Optional features control register */
	ioport32_t tclr;
#define AMDM37x_GPT_TCLR_ST_FLAG  (1 << 0)
#define AMDM37x_GPT_TCLR_AR_FLAG  (1 << 1)
#define AMDM37x_GPT_TCLR_PTV_MASK  (0x7)
#define AMDM37x_GPT_TCLR_PTV_SHIFT  (2)
#define AMDM37x_GPT_TCLR_PRE_FLAG  (1 << 5)
#define AMDM37x_GPT_TCLR_CE_FLAG  (1 << 6)
#define AMDM37x_GPT_TCLR_SCPWM  (1 << 7)
#define AMDM37x_GPT_TCLR_TCM_MASK  (0x3 << 8)
#define AMDM37x_GPT_TCLR_TCM_NO_CAPTURE   (0x0 << 8)
#define AMDM37x_GPT_TCLR_TCM_RAISE_CAPTURE   (0x1 << 8)
#define AMDM37x_GPT_TCLR_TCM_FALL_CAPTURE   (0x2 << 8)
#define AMDM37x_GPT_TCLR_TCM_BOTH_CAPTURE   (0x3 << 8)
#define AMDM37x_GPT_TCLR_TRG_MASK  (0x3 << 10)
#define AMDM37x_GPT_TCLR_TRG_NO  (0x0 << 10)
#define AMDM37x_GPT_TCLR_TRG_OVERFLOW  (0x1 << 10)
#define AMDM37x_GPT_TCLR_TRG_OVERMATCH  (0x2 << 10)
#define AMDM37x_GPT_TCLR_PT_FLAG  (1 << 12)
#define AMDM37x_GPT_TCLR_CAPT_MODE_FLAG  (1 << 13)
#define AMDM37x_GPT_TCLR_GPO_CFG_FLAG  (1 << 14)

	/** Value of timer counter */
	ioport32_t tccr;

	/** Timer load register */
	ioport32_t tldr;

	/** Timer trigger register */
	ioport32_t ttgr;

	/** Write-posted pending register */
	const ioport32_t twps;
#define AMDM37x_GPT_TWPS_TCLR_FLAG  (1 << 0)
#define AMDM37x_GPT_TWPS_TCRR_FLAG  (1 << 1)
#define AMDM37x_GPT_TWPS_TLDR_FLAG  (1 << 2)
#define AMDM37x_GPT_TWPS_TTGR_FLAG  (1 << 3)
#define AMDM37x_GPT_TWPS_TMAR_FLAG  (1 << 4)
#define AMDM37x_GPT_TWPS_TPIR_FLAG  (1 << 5)
#define AMDM37x_GPT_TWPS_TNIR_FLAG  (1 << 6)
#define AMDM37x_GPT_TWPS_TCVR_FLAG  (1 << 7)
#define AMDM37x_GPT_TWPS_TOCR_FLAG  (1 << 8)
#define AMDM37x_GPT_TWPS_TOWR_FLAG  (1 << 9)

	/** Timer match register */
	ioport32_t tmar;

	/** Capture value 1 register */
	const ioport32_t tcar1;

	/** Software interface control register */
	ioport32_t tsicr;
#define AMDM37x_GPT_TSICR_SFT_FLAG  (1 << 1)
#define AMDM37x_GPT_TSICR_POSTED_FLAG  (1 << 2)

	/** Capture value 2 register */
	const ioport32_t tcar2;

	/* GPT1,2,10 only (used for 1ms time period generation)*/

	/** Positive increment register */
	ioport32_t tpir;

	/** Negative increment register */
	ioport32_t tnir;

	/** Counter value register */
	ioport32_t tcvr;

	/** Mask the tick interrupt for selected number of ticks */
	ioport32_t tocr;

	/** Number of masked overflow interrupts */
	ioport32_t towr;
} amdm37x_gpt_regs_t;

typedef struct {
	amdm37x_gpt_regs_t *regs;
	bool special_available;
} amdm37x_gpt_t;

static inline void amdm37x_gpt_timer_ticks_init(
    amdm37x_gpt_t *timer, uintptr_t ioregs, size_t iosize, unsigned hz)
{
	/* Set 32768 Hz clock as source */
	// TODO find a nicer way to setup 32kHz clock source for timer1
	// reg 0x48004C40 is CM_CLKSEL_WKUP see page 485 of the manual
	ioport32_t *clksel = (void *) km_map(0x48004C40, 4, PAGE_NOT_CACHEABLE);
	*clksel &= ~1;
	km_unmap((uintptr_t)clksel, 4);

	assert(timer);
	/* Map control register */
	timer->regs = (void *) km_map(ioregs, iosize, PAGE_NOT_CACHEABLE);

	/* Reset the timer */
	timer->regs->tiocp_cfg |= AMDM37x_GPT_TIOCP_CFG_SOFTRESET_FLAG;

	while (!(timer->regs->tistat & AMDM37x_GPT_TISTAT_RESET_DONE_FLAG))
		;

	/* Set autoreload */
	timer->regs->tclr |= AMDM37x_GPT_TCLR_AR_FLAG;

	timer->special_available = ((ioregs == AMDM37x_GPT1_BASE_ADDRESS) ||
	    (ioregs == AMDM37x_GPT2_BASE_ADDRESS) ||
	    (ioregs == AMDM37x_GPT10_BASE_ADDRESS));
	/* Select reload value */
	timer->regs->tldr = 0xffffffff - (32768 / hz) + 1;
	/* Set current counter value */
	timer->regs->tccr = 0xffffffff - (32768 / hz) + 1;

	if (timer->special_available) {
		/* Set values according to formula (manual p. 2733) */
		/* Use temporary variables for easier debugging */
		const uint32_t tpir =
		    ((32768 / hz + 1) * 1000000) - (32768000L * (1000 / hz));
		const uint32_t tnir =
		    ((32768 / hz) * 1000000) - (32768000L * (1000 / hz));
		timer->regs->tpir = tpir;
		timer->regs->tnir = tnir;
	}

}

static inline void amdm37x_gpt_timer_ticks_start(amdm37x_gpt_t *timer)
{
	assert(timer);
	assert(timer->regs);
	/* Enable overflow interrupt */
	timer->regs->tier |= AMDM37x_GPT_TIER_OVF_IRQ_FLAG;
	/* Start timer */
	timer->regs->tclr |= AMDM37x_GPT_TCLR_ST_FLAG;
}

static inline bool amdm37x_gpt_irq_ack(amdm37x_gpt_t *timer)
{
	assert(timer);
	assert(timer->regs);
	/* Clear all pending interrupts */
	const uint32_t tisr = timer->regs->tisr;
	timer->regs->tisr = tisr;
	return tisr != 0;
}

#endif

/**
 * @}
 */
