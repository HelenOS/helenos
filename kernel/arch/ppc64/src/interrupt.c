/*
 * Copyright (c) 2006 Martin Decky
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

/** @addtogroup ppc64interrupt
 * @{
 */
/** @file
 */

#include <ddi/irq.h>
#include <interrupt.h>
#include <arch/interrupt.h>
#include <arch/types.h>
#include <arch.h>
#include <time/clock.h>
#include <ipc/sysipc.h>
#include <arch/drivers/pic.h>
#include <arch/mm/tlb.h>
#include <print.h>


void start_decrementer(void)
{
	asm volatile (
		"mtdec %0\n"
		:
		: "r" (1000)
	);
}


/** Handler of external interrupts */
static void exception_external(int n, istate_t *istate)
{
	int inum;
	
	while ((inum = pic_get_pending()) != -1) {
		irq_t *irq = irq_dispatch_and_lock(inum);
		if (irq) {
			/*
			 * The IRQ handler was found.
			 */
			irq->handler(irq, irq->arg);
			spinlock_unlock(&irq->lock);
		} else {
			/*
			 * Spurious interrupt.
			 */
#ifdef CONFIG_DEBUG
			printf("cpu%d: spurious interrupt (inum=%d)\n", CPU->id, inum);
#endif
		}
		pic_ack_interrupt(inum);
	}
}


static void exception_decrementer(int n, istate_t *istate)
{
	clock();
	start_decrementer();
}


/* Initialize basic tables for exception dispatching */
void interrupt_init(void)
{
	exc_register(VECTOR_EXTERNAL, "external", exception_external);
	exc_register(VECTOR_DECREMENTER, "timer", exception_decrementer);
}

/** @}
 */
