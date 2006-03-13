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

#include <arch/interrupt.h>
#include <syscall/syscall.h>
#include <print.h>
#include <debug.h>
#include <panic.h>
#include <arch/i8259.h>
#include <func.h>
#include <cpu.h>
#include <arch/asm.h>
#include <mm/tlb.h>
#include <mm/as.h>
#include <arch.h>
#include <symtab.h>
#include <proc/thread.h>

/*
 * Interrupt and exception dispatching.
 */

void (* disable_irqs_function)(__u16 irqmask) = NULL;
void (* enable_irqs_function)(__u16 irqmask) = NULL;
void (* eoi_function)(void) = NULL;

#define PRINT_INFO_ERRCODE(istate) do { \
	char *symbol = get_symtab_entry(istate->eip); \
	if (!symbol) \
		symbol = ""; \
	printf("----------------EXCEPTION OCCURED----------------\n"); \
	printf("%%eip: %X (%s)\n",istate->eip,symbol); \
	printf("ERROR_WORD=%X\n", istate->error_word); \
	printf("%%cs=%X,flags=%X\n", istate->cs, istate->eflags); \
	printf("%%eax=%X, %%ebx=%X, %%ecx=%X, %%edx=%X\n",\
	       istate->eax,istate->ebx,istate->ecx,istate->edx); \
	printf("%%esi=%X, %%edi=%X, %%ebp=%X, %%esp=%X\n",\
	       istate->esi,istate->edi,istate->ebp,istate->esp); \
	printf("stack: %X, %X, %X, %X\n", istate->stack[0], istate->stack[1], istate->stack[2], istate->stack[3]); \
	printf("       %X, %X, %X, %X\n", istate->stack[4], istate->stack[5], istate->stack[6], istate->stack[7]); \
} while(0)

void null_interrupt(int n, istate_t *istate)
{
	PRINT_INFO_ERRCODE(istate);
	panic("unserviced interrupt: %d\n", n);
}

void gp_fault(int n, istate_t *istate)
{
	PRINT_INFO_ERRCODE(istate);
	panic("general protection fault\n");
}

void ss_fault(int n, istate_t *istate)
{
	PRINT_INFO_ERRCODE(istate);
	panic("stack fault\n");
}

void nm_fault(int n, istate_t *istate)
{
#ifdef CONFIG_FPU_LAZY     
	scheduler_fpu_lazy_request();
#else
	panic("fpu fault");
#endif
}

void page_fault(int n, istate_t *istate)
{
	__address page;

	page = read_cr2();
	if (!as_page_fault(page)) {
		PRINT_INFO_ERRCODE(istate);
		printf("page fault address: %X\n", page);
		panic("page fault\n");
	}
}

void syscall(int n, istate_t *istate)
{
	interrupts_enable();
	if (istate->edx < SYSCALL_END)
		istate->eax = syscall_table[istate->edx](istate->eax, istate->ebx, istate->ecx);
	else
		panic("Undefined syscall %d", istate->edx);
	interrupts_disable();
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
