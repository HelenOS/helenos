/*
 * SPDX-FileCopyrightText: 2005 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_ia64
 * @{
 */
/** @file
 */

/** Interval Timer driver. */

#include <arch/drivers/it.h>
#include <arch/interrupt.h>
#include <arch/register.h>
#include <arch/asm.h>
#include <barrier.h>
#include <time/clock.h>
#include <ddi/irq.h>
#include <arch.h>

#define IT_SERVICE_CLOCKS  64

#define FREQ_NUMERATOR_SHIFT  32
#define FREQ_NUMERATOR_MASK   0xffffffff00000000ULL

#define FREQ_DENOMINATOR_SHIFT  0
#define FREQ_DENOMINATOR_MASK   0xffffffffULL

uint64_t it_delta;

static irq_t it_irq;

static irq_ownership_t it_claim(irq_t *);
static void it_interrupt(irq_t *);

/** Initialize Interval Timer. */
void it_init(void)
{
	if (config.cpu_active == 1) {
		irq_initialize(&it_irq);
		it_irq.inr = INTERRUPT_TIMER;
		it_irq.claim = it_claim;
		it_irq.handler = it_interrupt;
		irq_register(&it_irq);

		uint64_t base_freq;
		base_freq  = ((bootinfo->freq_scale) & FREQ_NUMERATOR_MASK) >>
		    FREQ_NUMERATOR_SHIFT;
		base_freq *= bootinfo->sys_freq;
		base_freq /= ((bootinfo->freq_scale) & FREQ_DENOMINATOR_MASK) >>
		    FREQ_DENOMINATOR_SHIFT;

		it_delta = base_freq / HZ;
	}

	/* Initialize Interval Timer external interrupt vector */
	cr_itv_t itv;

	itv.value = itv_read();
	itv.vector = INTERRUPT_TIMER;
	itv.m = 0;
	itv_write(itv.value);

	/* Set Interval Timer Counter to zero */
	itc_write(0);

	/* Generate first Interval Timer interrupt in IT_DELTA ticks */
	itm_write(IT_DELTA);

	/* Propagate changes */
	srlz_d();
}

/** Always claim ownership for this IRQ.
 *
 * Other devices are responsible to avoid using INR 0.
 *
 * @return Always IRQ_ACCEPT.
 *
 */
irq_ownership_t it_claim(irq_t *irq)
{
	return IRQ_ACCEPT;
}

/** Process Interval Timer interrupt. */
void it_interrupt(irq_t *irq)
{
	eoi_write(EOI);

	int64_t itm = itm_read();

	while (true) {
		int64_t itc = itc_read();
		itc += IT_SERVICE_CLOCKS;

		itm += IT_DELTA;
		if (itm - itc < 0)
			CPU->missed_clock_ticks++;
		else
			break;
	}

	itm_write(itm);
	srlz_d();  /* Propagate changes */

	/*
	 * We are holding a lock which prevents preemption.
	 * Release the lock, call clock() and reacquire the lock again.
	 */
	irq_spinlock_unlock(&irq->lock, false);
	clock();
	irq_spinlock_lock(&irq->lock, false);
}

/** @}
 */
