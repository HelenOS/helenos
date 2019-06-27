/*
 * Copyright (c) 2001-2004 Jakub Jermar
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

/** @addtogroup kernel_amd64_interrupt
 * @{
 */
/** @file
 */

#include <arch/interrupt.h>
#include <assert.h>
#include <log.h>
#include <panic.h>
#include <genarch/drivers/i8259/i8259.h>
#include <genarch/pic/pic_ops.h>
#include <halt.h>
#include <cpu.h>
#include <arch/asm.h>
#include <mm/tlb.h>
#include <mm/as.h>
#include <arch.h>
#include <proc/scheduler.h>
#include <proc/thread.h>
#include <proc/task.h>
#include <synch/spinlock.h>
#include <arch/ddi/ddi.h>
#include <interrupt.h>
#include <ddi/irq.h>
#include <symtab.h>
#include <stacktrace.h>

/*
 * Interrupt and exception dispatching.
 */
pic_ops_t *pic_ops = NULL;

void istate_decode(istate_t *istate)
{
	log_printf("cs =%0#18" PRIx64 "\trip=%0#18" PRIx64 "\t"
	    "rfl=%0#18" PRIx64 "\terr=%0#18" PRIx64 "\n",
	    istate->cs, istate->rip, istate->rflags, istate->error_word);

	if (istate_from_uspace(istate))
		log_printf("ss =%0#18" PRIx64 "\n", istate->ss);

	log_printf("rax=%0#18" PRIx64 "\trbx=%0#18" PRIx64 "\t"
	    "rcx=%0#18" PRIx64 "\trdx=%0#18" PRIx64 "\n",
	    istate->rax, istate->rbx, istate->rcx, istate->rdx);

	log_printf("rsi=%0#18" PRIx64 "\trdi=%0#18" PRIx64 "\t"
	    "rbp=%0#18" PRIx64 "\trsp=%0#18" PRIx64 "\n",
	    istate->rsi, istate->rdi, istate->rbp,
	    istate_from_uspace(istate) ? istate->rsp :
	    (uintptr_t) &istate->rsp);

	log_printf("r8 =%0#18" PRIx64 "\tr9 =%0#18" PRIx64 "\t"
	    "r10=%0#18" PRIx64 "\tr11=%0#18" PRIx64 "\n",
	    istate->r8, istate->r9, istate->r10, istate->r11);

	log_printf("r12=%0#18" PRIx64 "\tr13=%0#18" PRIx64 "\t"
	    "r14=%0#18" PRIx64 "\tr15=%0#18" PRIx64 "\n",
	    istate->r12, istate->r13, istate->r14, istate->r15);
}

static void null_interrupt(unsigned int n, istate_t *istate)
{
	fault_if_from_uspace(istate, "Unserviced interrupt: %u.", n);
	panic_badtrap(istate, n, "Unserviced interrupt.");
}

static void de_fault(unsigned int n, istate_t *istate)
{
	fault_if_from_uspace(istate, "Divide error.");
	panic_badtrap(istate, n, "Divide error.");
}

/** General Protection Fault.
 *
 */
static void gp_fault(unsigned int n, istate_t *istate)
{
	if (TASK) {
		irq_spinlock_lock(&TASK->lock, false);
		size_t ver = TASK->arch.iomapver;
		irq_spinlock_unlock(&TASK->lock, false);

		if (CPU->arch.iomapver_copy != ver) {
			/*
			 * This fault can be caused by an early access
			 * to I/O port because of an out-dated
			 * I/O Permission bitmap installed on CPU.
			 * Install the fresh copy and restart
			 * the instruction.
			 */
			io_perm_bitmap_install();
			return;
		}
		fault_if_from_uspace(istate, "General protection fault.");
	}
	panic_badtrap(istate, n, "General protection fault.");
}

static void ss_fault(unsigned int n, istate_t *istate)
{
	fault_if_from_uspace(istate, "Stack fault.");
	panic_badtrap(istate, n, "Stack fault.");
}

static void nm_fault(unsigned int n, istate_t *istate)
{
#ifdef CONFIG_FPU_LAZY
	scheduler_fpu_lazy_request();
#else
	fault_if_from_uspace(istate, "FPU fault.");
	panic("FPU fault.");
#endif
}

#ifdef CONFIG_SMP
static void tlb_shootdown_ipi(unsigned int n, istate_t *istate)
{
	pic_ops->eoi(0);
	tlb_shootdown_ipi_recv();
}
#endif

/** Handler of IRQ exceptions.
 *
 */
static void irq_interrupt(unsigned int n, istate_t *istate)
{
	assert(n >= IVT_IRQBASE);

	unsigned int inum = n - IVT_IRQBASE;
	bool ack = false;
	assert(inum < IRQ_COUNT);
	assert(inum != IRQ_PIC1);

	irq_t *irq = irq_dispatch_and_lock(inum);
	if (irq) {
		/*
		 * The IRQ handler was found.
		 */

		if (irq->preack) {
			/* Send EOI before processing the interrupt */
			pic_ops->eoi(inum);
			ack = true;
		}
		irq->handler(irq);
		irq_spinlock_unlock(&irq->lock, false);
	} else {
#ifdef CONFIG_DEBUG
		log(LF_ARCH, LVL_DEBUG, "cpu%u: unhandled IRQ %u", CPU->id,
		    inum);
#endif
	}

	if (!ack)
		pic_ops->eoi(inum);
}

static void pic_spurious(unsigned int n, istate_t *istate)
{
	unsigned int inum = n - IVT_IRQBASE;
	if (!pic_ops->is_spurious(inum)) {
		/* This is actually not a spurious IRQ, so proceed as usual. */
		irq_interrupt(n, istate);
		return;
	}
	pic_ops->handle_spurious(n);
#ifdef CONFIG_DEBUG
	log(LF_ARCH, LVL_DEBUG, "cpu%u: PIC spurious interrupt %u", CPU->id,
	    inum);
#endif
}

void interrupt_init(void)
{
	unsigned int i;

	for (i = 0; i < IVT_ITEMS; i++)
		exc_register(i, "null", false, (iroutine_t) null_interrupt);

	for (i = 0; i < IRQ_COUNT; i++) {
		if ((i != IRQ_PIC0_SPUR) && (i != IRQ_PIC1_SPUR) &&
		    (i != IRQ_PIC1))
			exc_register(IVT_IRQBASE + i, "irq", true,
			    (iroutine_t) irq_interrupt);
	}

	exc_register(VECTOR_DE, "de_fault", true, (iroutine_t) de_fault);
	exc_register(VECTOR_NM, "nm_fault", true, (iroutine_t) nm_fault);
	exc_register(VECTOR_SS, "ss_fault", true, (iroutine_t) ss_fault);
	exc_register(VECTOR_GP, "gp_fault", true, (iroutine_t) gp_fault);
	exc_register(VECTOR_PIC0_SPUR, "pic0_spurious", true,
	    (iroutine_t) pic_spurious);
	exc_register(VECTOR_PIC1_SPUR, "pic1_spurious", true,
	    (iroutine_t) pic_spurious);

#ifdef CONFIG_SMP
	exc_register(VECTOR_TLB_SHOOTDOWN_IPI, "tlb_shootdown", true,
	    (iroutine_t) tlb_shootdown_ipi);
#endif
}

/** @}
 */
