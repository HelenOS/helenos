/*
 * Copyright (c) 2005 Jakub Jermar
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
		CPU_LOCAL->missed_clock_ticks++;
	}
	CPU->arch.next_tick_cmpr = tick_counter_read() +
	    (CPU->arch.clock_frequency / HZ) - drift;
	tick_compare_write(CPU->arch.next_tick_cmpr);
	clock();
}

/** @}
 */
