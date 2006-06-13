/*
 * Copyright (C) 2001-2004 Jakub Jermar
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

 /** @addtogroup ia32interrupt ia32
 * @ingroup interrupt
 * @{
 */
/** @file
 */

#include <arch/interrupt.h>
#include <syscall/syscall.h>
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
#include <symtab.h>
#include <proc/thread.h>
#include <proc/task.h>
#include <synch/spinlock.h>
#include <arch/ddi/ddi.h>
#include <ipc/sysipc.h>
#include <interrupt.h>

/*
 * Interrupt and exception dispatching.
 */

void (* disable_irqs_function)(__u16 irqmask) = NULL;
void (* enable_irqs_function)(__u16 irqmask) = NULL;
void (* eoi_function)(void) = NULL;

void PRINT_INFO_ERRCODE(istate_t *istate)
{
	char *symbol = get_symtab_entry(istate->eip);

	if (!symbol)
		symbol = "";

	if (CPU)
		printf("----------------EXCEPTION OCCURED (cpu%d)----------------\n", CPU->id);
	else
		printf("----------------EXCEPTION OCCURED----------------\n");
		
	printf("%%eip: %#x (%s)\n",istate->eip,symbol);
	printf("ERROR_WORD=%#x\n", istate->error_word);
	printf("%%cs=%#x,flags=%#x\n", istate->cs, istate->eflags);
	printf("%%eax=%#x, %%ecx=%#x, %%edx=%#x, %%esp=%#x\n",  istate->eax,istate->ecx,istate->edx,&istate->stack[0]);
#ifdef CONFIG_DEBUG_ALLREGS
	printf("%%esi=%#x, %%edi=%#x, %%ebp=%#x, %%ebx=%#x\n",  istate->esi,istate->edi,istate->ebp,istate->ebx);
#endif
	printf("stack: %#x, %#x, %#x, %#x\n", istate->stack[0], istate->stack[1], istate->stack[2], istate->stack[3]);
	printf("       %#x, %#x, %#x, %#x\n", istate->stack[4], istate->stack[5], istate->stack[6], istate->stack[7]);
}

void null_interrupt(int n, istate_t *istate)
{
	fault_if_from_uspace(istate, "unserviced interrupt: %d", n);

	PRINT_INFO_ERRCODE(istate);
	panic("unserviced interrupt: %d\n", n);
}

/** General Protection Fault. */
void gp_fault(int n, istate_t *istate)
{
	if (TASK) {
		count_t ver;
		
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
		fault_if_from_uspace(istate, "general protection fault");
	}

	PRINT_INFO_ERRCODE(istate);
	panic("general protection fault\n");
}

void ss_fault(int n, istate_t *istate)
{
	fault_if_from_uspace(istate, "stack fault");

	PRINT_INFO_ERRCODE(istate);
	panic("stack fault\n");
}

void simd_fp_exception(int n, istate_t *istate)
{
	__u32 mxcsr;
	asm
	(
		"stmxcsr %0;\n"
		:"=m"(mxcsr)
	);
	fault_if_from_uspace(istate, "SIMD FP exception(19), MXCSR: %#zX",
			     (__native)mxcsr);

	PRINT_INFO_ERRCODE(istate);
	printf("MXCSR: %#zX\n",(__native)(mxcsr));
	panic("SIMD FP exception(19)\n");
}

void nm_fault(int n, istate_t *istate)
{
#ifdef CONFIG_FPU_LAZY     
	scheduler_fpu_lazy_request();
#else
	fault_if_from_uspace(istate, "fpu fault");
	panic("fpu fault");
#endif
}

void syscall(int n, istate_t *istate)
{
	panic("Obsolete syscall handler.");
}

void tlb_shootdown_ipi(int n, istate_t *istate)
{
	trap_virtual_eoi();
	tlb_shootdown_ipi_recv();
}

void trap_virtual_enable_irqs(__u16 irqmask)
{
	if (enable_irqs_function)
		enable_irqs_function(irqmask);
	else
		panic("no enable_irqs_function\n");
}

void trap_virtual_disable_irqs(__u16 irqmask)
{
	if (disable_irqs_function)
		disable_irqs_function(irqmask);
	else
		panic("no disable_irqs_function\n");
}

void trap_virtual_eoi(void)
{
	if (eoi_function)
		eoi_function();
	else
		panic("no eoi_function\n");

}

static void ipc_int(int n, istate_t *istate)
{
	ipc_irq_send_notif(n-IVT_IRQBASE);
	trap_virtual_eoi();
}


/* Reregister irq to be IPC-ready */
void irq_ipc_bind_arch(__native irq)
{
	if (irq == IRQ_CLK)
		return;
	trap_virtual_enable_irqs(1 << irq);
	exc_register(IVT_IRQBASE+irq, "ipc_int", ipc_int);
}

 /** @}
 */

