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

/** @addtogroup sparc64interrupt
 * @{
 */
/** @file
 */

#include <arch/interrupt.h>
#include <arch/sparc64.h>
#include <arch/trap/interrupt.h>
#include <interrupt.h>
#include <ddi/irq.h>
#include <stdint.h>
#include <debug.h>
#include <arch/asm.h>
#include <arch/barrier.h>
#include <log.h>
#include <arch.h>
#include <mm/tlb.h>
#include <config.h>
#include <synch/spinlock.h>

/** Process hardware interrupt.
 *
 * @param n Ignored.
 * @param istate Ignored.
 */
void interrupt(unsigned int n, istate_t *istate)
{
	uint64_t status = asi_u64_read(ASI_INTR_DISPATCH_STATUS, 0);
	if (status & (!INTR_DISPATCH_STATUS_BUSY))
		panic("Interrupt Dispatch Status busy bit not set\n");

	uint64_t intrcv = asi_u64_read(ASI_INTR_RECEIVE, 0);
#if defined (US)
	uint64_t data0 = asi_u64_read(ASI_INTR_R, ASI_UDB_INTR_R_DATA_0);
#elif defined (US3)
	uint64_t data0 = asi_u64_read(ASI_INTR_R, VA_INTR_R_DATA_0);
#endif

	irq_t *irq = irq_dispatch_and_lock(data0);
	if (irq) {
		/*
		 * The IRQ handler was found.
		 */
		irq->handler(irq);

		/*
		 * See if there is a clear-interrupt-routine and call it.
		 */
		if (irq->cir)
			irq->cir(irq->cir_arg, irq->inr);

		irq_spinlock_unlock(&irq->lock, false);
	} else if (data0 > config.base) {
		/*
		 * This is a cross-call.
		 * data0 contains address of the kernel function.
		 * We call the function only after we verify
		 * it is one of the supported ones.
		 */
#ifdef CONFIG_SMP
		if (data0 == (uintptr_t) tlb_shootdown_ipi_recv)
			tlb_shootdown_ipi_recv();
#endif
	} else {
		/*
		 * Spurious interrupt.
		 */
#ifdef CONFIG_DEBUG
		log(LF_ARCH, LVL_DEBUG,
		    "cpu%u: spurious interrupt (intrcv=%#" PRIx64 ", data0=%#"
		    PRIx64 ")", CPU->id, intrcv, data0);
#else
		(void) intrcv;
#endif
	}

	membar();
	asi_u64_write(ASI_INTR_RECEIVE, 0, 0);
}

/** @}
 */
