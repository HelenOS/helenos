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

/** Interval Timer driver. */
 
#include <arch/drivers/it.h>
#include <arch/interrupt.h>
#include <arch/register.h>
#include <arch/asm.h>
#include <arch/barrier.h>
#include <time/clock.h>

/** Initialize Interval Timer. */
void it_init(void)
{
	cr_itv_t itv;

	/* initialize Interval Timer external interrupt vector */
	itv.value = itv_read();
	itv.vector = INTERRUPT_TIMER;
	itv.m = 0;
	itv_write(itv.value);

	/* set Interval Timer Counter to zero */
	itc_write(0);
	
	/* generate first Interval Timer interrupt in IT_DELTA ticks */
	itm_write(IT_DELTA);

	/* propagate changes */
	srlz_d();
}

/** Process Interval Timer interrupt. */
void it_interrupt(void)
{
	eoi_write(EOI);
	itm_write(itc_read() + IT_DELTA);	/* program next interruption */
	srlz_d();				/* propagate changes */
	clock();
}
