/*
 * Copyright (c) 2003-2004 Jakub Jermar
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

/** @addtogroup mips32interrupt
 * @{
 */
/** @file
 */

#include <interrupt.h>
#include <arch/interrupt.h>
#include <typedefs.h>
#include <arch.h>
#include <arch/cp0.h>
#include <arch/smp/dorder.h>
#include <time/clock.h>
#include <ipc/sysipc.h>

#define IRQ_COUNT   8
#define TIMER_IRQ   7

#ifdef MACHINE_msim
#define DORDER_IRQ  5
#endif

function virtual_timer_fnc = NULL;
static irq_t timer_irq;

#ifdef MACHINE_msim
static irq_t dorder_irq;
#endif

// TODO: This is SMP unsafe!!!

uint32_t count_hi = 0;
static unsigned long nextcount;
static unsigned long lastcount;

/** Disable interrupts.
 *
 * @return Old interrupt priority level.
 */
ipl_t interrupts_disable(void)
{
	ipl_t ipl = (ipl_t) cp0_status_read();
	cp0_status_write(ipl & ~cp0_status_ie_enabled_bit);
	return ipl;
}

/** Enable interrupts.
 *
 * @return Old interrupt priority level.
 */
ipl_t interrupts_enable(void)
{
	ipl_t ipl = (ipl_t) cp0_status_read();
	cp0_status_write(ipl | cp0_status_ie_enabled_bit);
	return ipl;
}

/** Restore interrupt priority level.
 *
 * @param ipl Saved interrupt priority level.
 */
void interrupts_restore(ipl_t ipl)
{
	cp0_status_write(cp0_status_read() | (ipl & cp0_status_ie_enabled_bit));
}

/** Read interrupt priority level.
 *
 * @return Current interrupt priority level.
 */
ipl_t interrupts_read(void)
{
	return cp0_status_read();
}

/** Check interrupts state.
 *
 * @return True if interrupts are disabled.
 *
 */
bool interrupts_disabled(void)
{
	return !(cp0_status_read() & cp0_status_ie_enabled_bit);
}

/** Start hardware clock
 *
 */
static void timer_start(void)
{
	lastcount = cp0_count_read();
	nextcount = cp0_compare_value + cp0_count_read();
	cp0_compare_write(nextcount);
}

static irq_ownership_t timer_claim(irq_t *irq)
{
	return IRQ_ACCEPT;
}

static void timer_irq_handler(irq_t *irq)
{
	if (cp0_count_read() < lastcount)
		/* Count overflow detected */
		count_hi++;

	lastcount = cp0_count_read();

	unsigned long drift = cp0_count_read() - nextcount;
	while (drift > cp0_compare_value) {
		drift -= cp0_compare_value;
		CPU->missed_clock_ticks++;
	}

	nextcount = cp0_count_read() + cp0_compare_value - drift;
	cp0_compare_write(nextcount);

	/*
	 * We are holding a lock which prevents preemption.
	 * Release the lock, call clock() and reacquire the lock again.
	 */
	irq_spinlock_unlock(&irq->lock, false);
	clock();
	irq_spinlock_lock(&irq->lock, false);

	if (virtual_timer_fnc != NULL)
		virtual_timer_fnc();
}

#ifdef MACHINE_msim
static irq_ownership_t dorder_claim(irq_t *irq)
{
	return IRQ_ACCEPT;
}

static void dorder_irq_handler(irq_t *irq)
{
	dorder_ipi_ack(1 << dorder_cpuid());
}
#endif

/* Initialize basic tables for exception dispatching */
void interrupt_init(void)
{
	irq_init(IRQ_COUNT, IRQ_COUNT);

	irq_initialize(&timer_irq);
	timer_irq.inr = TIMER_IRQ;
	timer_irq.claim = timer_claim;
	timer_irq.handler = timer_irq_handler;
	irq_register(&timer_irq);

	timer_start();
	cp0_unmask_int(TIMER_IRQ);

#ifdef MACHINE_msim
	irq_initialize(&dorder_irq);
	dorder_irq.inr = DORDER_IRQ;
	dorder_irq.claim = dorder_claim;
	dorder_irq.handler = dorder_irq_handler;
	irq_register(&dorder_irq);

	cp0_unmask_int(DORDER_IRQ);
#endif
}

/** @}
 */
