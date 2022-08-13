/*
 * SPDX-FileCopyrightText: 2003-2004 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
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
		CPU->missed_clock_ticks++;
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
