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

/** @addtogroup ppc32interrupt
 * @{
 */
/** @file
 */

#include <ddi/irq.h>
#include <interrupt.h>
#include <arch/interrupt.h>
#include <typedefs.h>
#include <arch.h>
#include <time/clock.h>
#include <ipc/sysipc.h>
#include <arch/drivers/pic.h>
#include <arch/mm/tlb.h>
#include <print.h>

void start_decrementer(void)
{
	asm volatile (
		"mtdec %[dec]\n"
		:: [dec] "r" (1000)
	);
}

void istate_decode(istate_t *istate)
{
	printf("r0 =%p\tr1 =%p\tr2 =%p\n", istate->r0, istate->sp, istate->r2);
	printf("r3 =%p\tr4 =%p\tr5 =%p\n", istate->r3, istate->r4, istate->r5);
	printf("r6 =%p\tr7 =%p\tr8 =%p\n", istate->r6, istate->r7, istate->r8);
	printf("r9 =%p\tr10=%p\tr11=%p\n",
	    istate->r9, istate->r10, istate->r11);
	printf("r12=%p\tr13=%p\tr14=%p\n",
	    istate->r12, istate->r13, istate->r14);
	printf("r15=%p\tr16=%p\tr17=%p\n",
	    istate->r15, istate->r16, istate->r17);
	printf("r18=%p\tr19=%p\tr20=%p\n",
	    istate->r18, istate->r19, istate->r20);
	printf("r21=%p\tr22=%p\tr23=%p\n",
	    istate->r21, istate->r22, istate->r23);
	printf("r24=%p\tr25=%p\tr26=%p\n",
	    istate->r24, istate->r25, istate->r26);
	printf("r27=%p\tr28=%p\tr29=%p\n",
	    istate->r27, istate->r28, istate->r29);
	printf("r30=%p\tr31=%p\n", istate->r30, istate->r31);
	printf("cr =%p\tpc =%p\tlr =%p\n", istate->cr, istate->pc, istate->lr);
	printf("ctr=%p\txer=%p\tdar=%p\n",
	    istate->ctr, istate->xer, istate->dar);
	printf("srr1=%p\n", istate->srr1);
}

/** External interrupts handler
 *
 */
static void exception_external(unsigned int n, istate_t *istate)
{
	uint8_t inum;
	
	while ((inum = pic_get_pending()) != 255) {
		irq_t *irq = irq_dispatch_and_lock(inum);
		if (irq) {
			/*
			 * The IRQ handler was found.
			 */
			
			if (irq->preack) {
				/* Acknowledge the interrupt before processing */
				if (irq->cir)
					irq->cir(irq->cir_arg, irq->inr);
			}
			
			irq->handler(irq);
			
			if (!irq->preack) {
				if (irq->cir)
					irq->cir(irq->cir_arg, irq->inr);
			}
			
			irq_spinlock_unlock(&irq->lock, false);
		} else {
			/*
			 * Spurious interrupt.
			 */
#ifdef CONFIG_DEBUG
			printf("cpu%" PRIs ": spurious interrupt (inum=%" PRIu8 ")\n",
			    CPU->id, inum);
#endif
		}
	}
}

static void exception_decrementer(unsigned int n, istate_t *istate)
{
	start_decrementer();
	clock();
}

/* Initialize basic tables for exception dispatching */
void interrupt_init(void)
{
	exc_register(VECTOR_DATA_STORAGE, "data_storage", true,
	    pht_refill);
	exc_register(VECTOR_INSTRUCTION_STORAGE, "instruction_storage", true,
	    pht_refill);
	exc_register(VECTOR_EXTERNAL, "external", true,
	    exception_external);
	exc_register(VECTOR_DECREMENTER, "timer", true,
	    exception_decrementer);
}

/** @}
 */
