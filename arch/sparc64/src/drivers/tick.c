/*
 * Copyright (C) 2005 Jakub Jermar
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

 /** @addtogroup sparc64	
 * @{
 */
/** @file
 */

#include <arch/drivers/tick.h>
#include <arch/interrupt.h>
#include <arch/asm.h>
#include <arch/register.h>
#include <debug.h>
#include <time/clock.h>
#include <typedefs.h>

/** Initialize tick interrupt. */
void tick_init(void)
{
	tick_compare_reg_t compare;
	
	interrupt_register(14, "tick_int", tick_interrupt);
	compare.int_dis = false;
	compare.tick_cmpr = TICK_DELTA;
	tick_compare_write(compare.value);
	tick_write(0);
}

/** Process tick interrupt.
 *
 * @param n Interrupt Level, 14,  (can be ignored)
 * @param istate Interrupted state.
 */
void tick_interrupt(int n, istate_t *istate)
{
	softint_reg_t softint, clear;
	
	softint.value = softint_read();
	
	/*
	 * Make sure we are servicing interrupt_level_14
	 */
	ASSERT(n == 14);
	
	/*
	 * Make sure we are servicing TICK_INT.
	 */
	ASSERT(softint.tick_int);

	/*
	 * Clear tick interrupt.
	 */
	clear.value = 0;
	clear.tick_int = 1;
	clear_softint_write(clear.value);
	
	/*
	 * Restart counter.
	 */
	tick_write(0);
	
	clock();
}

 /** @}
 */

