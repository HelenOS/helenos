/*
 * SPDX-FileCopyrightText: 2005 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_sparc64
 * @{
 */
/** @file
 */

#include <arch/drivers/tick.h>
#include <arch/interrupt.h>
#include <arch/trap/interrupt.h>
#include <arch/sparc64.h>
#include <arch/asm.h>
#include <arch/register.h>
#include <arch/cpu.h>
#include <arch/boot/boot.h>
#include <time/clock.h>
#include <arch.h>
#include <assert.h>

/** Initialize tick and stick interrupt. */
void tick_init(void)
{
	/* initialize TICK interrupt */
	tick_compare_reg_t compare;
	softint_reg_t clear;

	compare.int_dis = false;
	compare.tick_cmpr = tick_counter_read() +
	    CPU->arch.clock_frequency / HZ;
	CPU->arch.next_tick_cmpr = compare.tick_cmpr;
	tick_compare_write(compare.value);

	clear.value = 0;
	clear.tick_int = 1;
	clear_softint_write(clear.value);

#if defined (US3) || defined (SUN4V)
	/* disable STICK interrupts and clear any pending ones */
	tick_compare_reg_t stick_compare;

	stick_compare.value = stick_compare_read();
	stick_compare.int_dis = true;
	stick_compare.tick_cmpr = 0;
	stick_compare_write(stick_compare.value);

	clear.value = 0;
	clear.stick_int = 1;
	clear_softint_write(clear.value);
#endif
}

/** Process tick interrupt.
 *
 * @param n      Trap type (0x4e, can be ignored)
 * @param istate Interrupted state.
 *
 */
void tick_interrupt(unsigned int n, istate_t *istate)
{
	softint_reg_t softint, clear;
	uint64_t drift;

	softint.value = softint_read();

	/*
	 * Make sure we are servicing interrupt_level_14
	 */
	assert(n == TT_INTERRUPT_LEVEL_14);

	/*
	 * Make sure we are servicing TICK_INT.
	 */
	assert(softint.tick_int);

	/*
	 * Clear tick interrupt.
	 */
	clear.value = 0;
	clear.tick_int = 1;
	clear_softint_write(clear.value);

	/*
	 * Reprogram the compare register.
	 * For now, we can ignore the potential of the registers to overflow.
	 * On a 360MHz Ultra 60, the 63-bit compare counter will overflow in
	 * about 812 years. If there was a 2GHz UltraSPARC computer, it would
	 * overflow only in 146 years.
	 */
	drift = tick_counter_read() - CPU->arch.next_tick_cmpr;
	while (drift > CPU->arch.clock_frequency / HZ) {
		drift -= CPU->arch.clock_frequency / HZ;
		CPU->missed_clock_ticks++;
	}
	CPU->arch.next_tick_cmpr = tick_counter_read() +
	    (CPU->arch.clock_frequency / HZ) - drift;
	tick_compare_write(CPU->arch.next_tick_cmpr);
	clock();
}

/** @}
 */
