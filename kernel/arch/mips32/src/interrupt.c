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

/** @addtogroup kernel_mips32_interrupt
 * @{
 */
/** @file
 */

#include <interrupt.h>
#include <arch/interrupt.h>
#include <typedefs.h>
#include <arch.h>
#include <arch/cp0.h>
#include <time/clock.h>
#include <ipc/sysipc.h>

// TODO: This is SMP unsafe!!!

uint32_t count_hi = 0;
static unsigned long nextcount;
static unsigned long lastcount;

/** Table of interrupt handlers. */
int_handler_t int_handler[MIPS_INTERRUPTS] = { };

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

static void timer_interrupt_handler(unsigned int intr)
{
	if (cp0_count_read() < lastcount)
		/* Count overflow detected */
		count_hi++;

	lastcount = cp0_count_read();

	unsigned long drift = cp0_count_read() - nextcount;
	while (drift > cp0_compare_value) {
		drift -= cp0_compare_value;
		CPU_LOCAL->missed_clock_ticks++;
	}

	nextcount = cp0_count_read() + cp0_compare_value - drift;
	cp0_compare_write(nextcount);

	clock();
}

/* Initialize basic tables for exception dispatching */
void interrupt_init(void)
{
	int_handler[INT_TIMER] = timer_interrupt_handler;

	timer_start();
	cp0_unmask_int(INT_TIMER);
}

/** @}
 */
