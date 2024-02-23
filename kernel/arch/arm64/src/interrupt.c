/*
 * Copyright (c) 2015 Petr Pavlu
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

/** @addtogroup kernel_arm64
 * @{
 */
/** @file
 * @brief Interrupts controlling routines.
 */

#include <arch/interrupt.h>
#include <arch/machine_func.h>
#include <ddi/irq.h>
#include <interrupt.h>
#include <time/clock.h>

static irq_t timer_irq;
static uint64_t timer_increment;

/** Disable interrupts.
 *
 * @return Old interrupt priority level.
 */
ipl_t interrupts_disable(void)
{
	uint64_t daif = DAIF_read();

	DAIF_write(daif | DAIF_IRQ_FLAG);

	return daif & DAIF_IRQ_FLAG;
}

/** Enable interrupts.
 *
 * @return Old interrupt priority level.
 */
ipl_t interrupts_enable(void)
{
	uint64_t daif = DAIF_read();

	DAIF_write(daif & ~DAIF_IRQ_FLAG);

	return daif & DAIF_IRQ_FLAG;
}

/** Restore interrupt priority level.
 *
 * @param ipl Saved interrupt priority level.
 */
void interrupts_restore(ipl_t ipl)
{
	uint64_t daif = DAIF_read();

	DAIF_write((daif & ~DAIF_IRQ_FLAG) | (ipl & DAIF_IRQ_FLAG));
}

/** Read interrupt priority level.
 *
 * @return Current interrupt priority level.
 */
ipl_t interrupts_read(void)
{
	return DAIF_read() & DAIF_IRQ_FLAG;
}

/** Check interrupts state.
 *
 * @return True if interrupts are disabled.
 */
bool interrupts_disabled(void)
{
	return DAIF_read() & DAIF_IRQ_FLAG;
}

/** Suspend the virtual timer. */
static void timer_suspend(void)
{
	uint64_t cntv_ctl = CNTV_CTL_EL0_read();

	CNTV_CTL_EL0_write(cntv_ctl | CNTV_CTL_IMASK_FLAG);
}

/** Start the virtual timer. */
static void timer_start(void)
{
	uint64_t cntfrq = CNTFRQ_EL0_read();
	uint64_t cntvct = CNTVCT_EL0_read();
	uint64_t cntv_ctl = CNTV_CTL_EL0_read();

	/* Calculate the increment. */
	timer_increment = cntfrq / HZ;

	/* Program the timer. */
	CNTV_CVAL_EL0_write(cntvct + timer_increment);
	CNTV_CTL_EL0_write(
	    (cntv_ctl & ~CNTV_CTL_IMASK_FLAG) | CNTV_CTL_ENABLE_FLAG);
}

/** Claim the virtual timer interrupt. */
static irq_ownership_t timer_claim(irq_t *irq)
{
	return IRQ_ACCEPT;
}

/** Handle the virtual timer interrupt. */
static void timer_irq_handler(irq_t *irq)
{
	uint64_t cntvct = CNTVCT_EL0_read();
	uint64_t cntv_cval = CNTV_CVAL_EL0_read();

	uint64_t drift = cntvct - cntv_cval;
	while (drift > timer_increment) {
		drift -= timer_increment;
		CPU_LOCAL->missed_clock_ticks++;
	}
	CNTV_CVAL_EL0_write(cntvct + timer_increment - drift);

	/*
	 * We are holding a lock which prevents preemption.
	 * Release the lock, call clock() and reacquire the lock again.
	 */
	irq_spinlock_unlock(&irq->lock, false);
	clock();
	irq_spinlock_lock(&irq->lock, false);
}

/** Initialize basic tables for exception dispatching. */
void interrupt_init(void)
{
	size_t irq_count = machine_get_irq_count();
	irq_init(irq_count, irq_count);

	/* Initialize virtual timer. */
	timer_suspend();
	inr_t timer_inr = machine_enable_vtimer_irq();

	irq_initialize(&timer_irq);
	timer_irq.inr = timer_inr;
	timer_irq.claim = timer_claim;
	timer_irq.handler = timer_irq_handler;
	irq_register(&timer_irq);

	timer_start();
}

/** @}
 */
