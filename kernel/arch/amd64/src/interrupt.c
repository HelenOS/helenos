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

/** @addtogroup amd64interrupt
 * @{
 */
/** @file
 */

#include <arch/interrupt.h>
#include <print.h>
#include <debug.h>
#include <panic.h>
#include <arch/drivers/i8259.h>
#include <func.h>
#include <cpu.h>
#include <arch/asm.h>
#include <mm/tlb.h>
#include <mm/as.h>
#include <arch.h>
#include <arch/asm.h>
#include <proc/scheduler.h>
#include <proc/thread.h>
#include <proc/task.h>
#include <synch/spinlock.h>
#include <arch/ddi/ddi.h>
#include <interrupt.h>
#include <ddi/irq.h>
#include <symtab.h>

/*
 * Interrupt and exception dispatching.
 */

void (* disable_irqs_function)(uint16_t irqmask) = NULL;
void (* enable_irqs_function)(uint16_t irqmask) = NULL;
void (* eoi_function)(void) = NULL;

void decode_istate(int n, istate_t *istate)
{
	char *symbol;

	symbol = symtab_fmt_name_lookup(istate->rip);

	printf("-----EXCEPTION(%d) OCCURED----- ( %s )\n", n, __func__);
	printf("%%rip: %#llx (%s)\n", istate->rip, symbol);
	printf("ERROR_WORD=%#llx\n", istate->error_word);
	printf("%%cs=%#llx, rflags=%#llx, %%cr0=%#llx\n", istate->cs,
	    istate->rflags, read_cr0());
	printf("%%rax=%#llx, %%rcx=%#llx, %%rdx=%#llx\n", istate->rax,
	    istate->rcx, istate->rdx);
	printf("%%rsi=%#llx, %%rdi=%#llx, %%r8=%#llx\n", istate->rsi,
	    istate->rdi, istate->r8);
	printf("%%r9=%#llx, %%r10=%#llx, %%r11=%#llx\n", istate->r9,
	    istate->r10, istate->r11);
	printf("%%rsp=%#llx\n", &istate->stack[0]);
}

static void trap_virtual_eoi(void)
{
	if (eoi_function)
		eoi_function();
	else
		panic("No eoi_function.");

}

static void null_interrupt(int n, istate_t *istate)
{
	fault_if_from_uspace(istate, "Unserviced interrupt: %d.", n);
	decode_istate(n, istate);
	panic("Unserviced interrupt.");
}

static void de_fault(int n, istate_t *istate)
{
	fault_if_from_uspace(istate, "Divide error.");
	decode_istate(n, istate);
	panic("Divide error.");
}

/** General Protection Fault. */
static void gp_fault(int n, istate_t *istate)
{
	if (TASK) {
		size_t ver;

		spinlock_lock(&TASK->lock);
		ver = TASK->arch.iomapver;
		spinlock_unlock(&TASK->lock);

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

	decode_istate(n, istate);
	panic("General protection fault.");
}

static void ss_fault(int n, istate_t *istate)
{
	fault_if_from_uspace(istate, "Stack fault.");
	decode_istate(n, istate);
	panic("Stack fault.");
}

static void nm_fault(int n, istate_t *istate)
{
#ifdef CONFIG_FPU_LAZY
	scheduler_fpu_lazy_request();
#else
	fault_if_from_uspace(istate, "FPU fault.");
	panic("FPU fault.");
#endif
}

#ifdef CONFIG_SMP
static void tlb_shootdown_ipi(int n, istate_t *istate)
{
	trap_virtual_eoi();
	tlb_shootdown_ipi_recv();
}
#endif

/** Handler of IRQ exceptions */
static void irq_interrupt(int n, istate_t *istate)
{
	ASSERT(n >= IVT_IRQBASE);
	
	int inum = n - IVT_IRQBASE;
	bool ack = false;
	ASSERT(inum < IRQ_COUNT);
	ASSERT((inum != IRQ_PIC_SPUR) && (inum != IRQ_PIC1));
	
	irq_t *irq = irq_dispatch_and_lock(inum);
	if (irq) {
		/*
		 * The IRQ handler was found.
		 */
		 
		if (irq->preack) {
			/* Send EOI before processing the interrupt */
			trap_virtual_eoi();
			ack = true;
		}
		irq->handler(irq);
		spinlock_unlock(&irq->lock);
	} else {
		/*
		 * Spurious interrupt.
		 */
#ifdef CONFIG_DEBUG
		printf("cpu%d: spurious interrupt (inum=%d)\n", CPU->id, inum);
#endif
	}
	
	if (!ack)
		trap_virtual_eoi();
}

void interrupt_init(void)
{
	int i;
	
	for (i = 0; i < IVT_ITEMS; i++)
		exc_register(i, "null", (iroutine) null_interrupt);
	
	for (i = 0; i < IRQ_COUNT; i++) {
		if ((i != IRQ_PIC_SPUR) && (i != IRQ_PIC1))
			exc_register(IVT_IRQBASE + i, "irq",
			    (iroutine) irq_interrupt);
	}
	
	exc_register(0, "de_fault", (iroutine) de_fault);
	exc_register(7, "nm_fault", (iroutine) nm_fault);
	exc_register(12, "ss_fault", (iroutine) ss_fault);
	exc_register(13, "gp_fault", (iroutine) gp_fault);
	exc_register(14, "ident_mapper", (iroutine) ident_page_fault);
	
#ifdef CONFIG_SMP
	exc_register(VECTOR_TLB_SHOOTDOWN_IPI, "tlb_shootdown",
	    (iroutine) tlb_shootdown_ipi);
#endif
}

void trap_virtual_enable_irqs(uint16_t irqmask)
{
	if (enable_irqs_function)
		enable_irqs_function(irqmask);
	else
		panic("No enable_irqs_function.");
}

void trap_virtual_disable_irqs(uint16_t irqmask)
{
	if (disable_irqs_function)
		disable_irqs_function(irqmask);
	else
		panic("No disable_irqs_function.");
}

/** @}
 */
