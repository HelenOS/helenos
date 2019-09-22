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

/** @addtogroup kernel_ppc32_interrupt
 * @{
 */
/** @file
 */

#include <ddi/irq.h>
#include <interrupt.h>
#include <arch/interrupt.h>
#include <arch/istate.h>
#include <stdint.h>
#include <arch.h>
#include <ipc/sysipc.h>
#include <arch/drivers/pic.h>
#include <arch/mm/tlb.h>
#include <arch/mm/pht.h>
#include <log.h>

static uint32_t decrementer_value;

void decrementer_start(uint32_t val)
{
	decrementer_value = val;
	decrementer_restart();
}

void decrementer_restart(void)
{
	asm volatile (
	    "mtdec %[dec]\n"
	    :: [dec] "r" (decrementer_value)
	);
}

void istate_decode(istate_t *istate)
{
	log_printf("r0 =%0#10" PRIx32 "\tr1 =%0#10" PRIx32 "\t"
	    "r2 =%0#10" PRIx32 "\n", istate->r0, istate->sp, istate->r2);

	log_printf("r3 =%0#10" PRIx32 "\tr4 =%0#10" PRIx32 "\t"
	    "r5 =%0#10" PRIx32 "\n", istate->r3, istate->r4, istate->r5);

	log_printf("r6 =%0#10" PRIx32 "\tr7 =%0#10" PRIx32 "\t"
	    "r8 =%0#10" PRIx32 "\n", istate->r6, istate->r7, istate->r8);

	log_printf("r9 =%0#10" PRIx32 "\tr10=%0#10" PRIx32 "\t"
	    "r11=%0#10" PRIx32 "\n", istate->r9, istate->r10, istate->r11);

	log_printf("r12=%0#10" PRIx32 "\tr13=%0#10" PRIx32 "\t"
	    "r14=%0#10" PRIx32 "\n", istate->r12, istate->r13, istate->r14);

	log_printf("r15=%0#10" PRIx32 "\tr16=%0#10" PRIx32 "\t"
	    "r17=%0#10" PRIx32 "\n", istate->r15, istate->r16, istate->r17);

	log_printf("r18=%0#10" PRIx32 "\tr19=%0#10" PRIx32 "\t"
	    "r20=%0#10" PRIx32 "\n", istate->r18, istate->r19, istate->r20);

	log_printf("r21=%0#10" PRIx32 "\tr22=%0#10" PRIx32 "\t"
	    "r23=%0#10" PRIx32 "\n", istate->r21, istate->r22, istate->r23);

	log_printf("r24=%0#10" PRIx32 "\tr25=%0#10" PRIx32 "\t"
	    "r26=%0#10" PRIx32 "\n", istate->r24, istate->r25, istate->r26);

	log_printf("r27=%0#10" PRIx32 "\tr28=%0#10" PRIx32 "\t"
	    "r29=%0#10" PRIx32 "\n", istate->r27, istate->r28, istate->r29);

	log_printf("r30=%0#10" PRIx32 "\tr31=%0#10" PRIx32 "\n",
	    istate->r30, istate->r31);

	log_printf("cr =%0#10" PRIx32 "\tpc =%0#10" PRIx32 "\t"
	    "lr =%0#10" PRIx32 "\n", istate->cr, istate->pc, istate->lr);

	log_printf("ctr=%0#10" PRIx32 "\txer=%0#10" PRIx32 "\t"
	    "dar=%0#10" PRIx32 "\n", istate->ctr, istate->xer, istate->dar);

	log_printf("srr1=%0#10" PRIx32 "\n", istate->srr1);
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
			log(LF_ARCH, LVL_DEBUG, "cpu%u: spurious interrupt"
			    " (inum=%" PRIu8 ")", CPU->id, inum);
#endif
			pic_ack_interrupt(NULL, inum);
			break;
		}
	}
}

static void exception_fp_unavailable(unsigned int n, istate_t *istate)
{
#ifdef CONFIG_FPU_LAZY
	scheduler_fpu_lazy_request();
	/*
	 * Propagate MSR_FP from MSR back to istate's SRR1, which will become
	 * the next MSR.
	 */
	istate->srr1 |= msr_read() & MSR_FP;
#else
	fault_if_from_uspace(istate, "FPU fault.");
	panic_badtrap(istate, n, "FPU fault.");
#endif
}

static void exception_decrementer(unsigned int n, istate_t *istate)
{
	decrementer_restart();
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
	exc_register(VECTOR_FP_UNAVAILABLE, "fp_unavailable", true,
	    exception_fp_unavailable);
	exc_register(VECTOR_DECREMENTER, "timer", true,
	    exception_decrementer);
	exc_register(VECTOR_ITLB_MISS, "itlb_miss", true,
	    tlb_refill);
	exc_register(VECTOR_DTLB_MISS_LOAD, "dtlb_miss_load", true,
	    tlb_refill);
	exc_register(VECTOR_DTLB_MISS_STORE, "dtlb_miss_store", true,
	    tlb_refill);
}

/** @}
 */
