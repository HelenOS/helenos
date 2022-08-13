/*
 * SPDX-FileCopyrightText: 2005 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_sparc64_interrupt
 * @{
 */
/** @file
 */

#include <arch/barrier.h>
#include <arch/interrupt.h>
#include <arch/sparc64.h>
#include <arch/trap/interrupt.h>
#include <interrupt.h>
#include <ddi/irq.h>
#include <stdint.h>
#include <debug.h>
#include <arch/asm.h>
#include <barrier.h>
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
