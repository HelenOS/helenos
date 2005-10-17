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
#include <print.h>
#include <debug.h>
#include <panic.h>
#include <arch/i8259.h>
#include <func.h>
#include <cpu.h>
#include <arch/asm.h>
#include <mm/tlb.h>
#include <arch.h>
#include <symtab.h>
#include <arch/asm.h>
#include <proc/scheduler.h>
#include <proc/thread.h>


static void messy_stack_trace(__native *stack)
{
	__native *upper_limit = (__native *)(((__native)get_stack_base()) + STACK_SIZE);
	char *symbol;

	printf("Stack contents: ");
	while (stack < upper_limit) {
		symbol = get_symtab_entry((__address)*stack);
		if (symbol)
			printf("%s, ", symbol);
		stack++;
	}
	printf("\n");
}

static void print_info_errcode(__u8 n, __native x[])
{
	char *symbol;

	if (!(symbol=get_symtab_entry(x[1])))
		symbol = "";

	printf("-----EXCEPTION(%d) OCCURED----- ( %s )\n",n,__FUNCTION__);
	printf("%%rip: %Q (%s)\n",x[1],symbol);
	printf("ERROR_WORD=%Q\n", x[0]);
	printf("%%rcs=%Q,flags=%Q, %%cr0=%Q\n", x[2], x[3],read_cr0());
	printf("%%rax=%Q, %%rbx=%Q, %%rcx=%Q\n",x[-2],x[-3],x[-4]);
	printf("%%rdx=%Q, %%rsi=%Q, %%rdi=%Q\n",x[-5],x[-6],x[-7]);
	printf("%%r8 =%Q, %%r9 =%Q, %%r10=%Q\n",x[-8],x[-9],x[-10]);
	printf("%%r11=%Q, %%r12=%Q, %%r13=%Q\n",x[-11],x[-12],x[-13]);
	printf("%%r14=%Q, %%r15=%Q, %%rsp=%Q\n",x[-14],x[-15],x);
	printf("%%rbp=%Q\n",x[-1]);
	printf("stack: %Q, %Q, %Q\n", x[5], x[6], x[7]);
	printf("       %Q, %Q, %Q\n", x[8], x[9], x[10]);
	printf("       %Q, %Q, %Q\n", x[11], x[12], x[13]);
	printf("       %Q, %Q, %Q\n", x[14], x[15], x[16]);
	printf("       %Q, %Q, %Q\n", x[17], x[18], x[19]);
	printf("       %Q, %Q, %Q\n", x[20], x[21], x[22]);
	printf("       %Q, %Q, %Q\n", x[23], x[24], x[25]);
	messy_stack_trace(&x[5]);
}

/*
 * Interrupt and exception dispatching.
 */

static iroutine ivt[IVT_ITEMS];

void (* disable_irqs_function)(__u16 irqmask) = NULL;
void (* enable_irqs_function)(__u16 irqmask) = NULL;
void (* eoi_function)(void) = NULL;

iroutine trap_register(__u8 n, iroutine f)
{
	ASSERT(n < IVT_ITEMS);
	
	iroutine old;
	
	old = ivt[n];
	ivt[n] = f;
	
	return old;
}

/*
 * Called directly from the assembler code.
 * CPU is interrupts_disable()'d.
 */
void trap_dispatcher(__u8 n, __native stack[])
{
	ASSERT(n < IVT_ITEMS);
	
	ivt[n](n, stack);
}

void null_interrupt(__u8 n, __native stack[])
{
	printf("-----EXCEPTION(%d) OCCURED----- ( %s )\n",n,__FUNCTION__); \
	printf("stack: %L, %L, %L, %L\n", stack[0], stack[1], stack[2], stack[3]);
	panic("unserviced interrupt\n");
}

void gp_fault(__u8 n, __native stack[])
{
	print_info_errcode(n,stack);
	panic("general protection fault\n");
}

void ss_fault(__u8 n, __native stack[])
{
	print_info_errcode(n,stack);
	panic("stack fault\n");
}


void nm_fault(__u8 n, __native stack[])
{
#ifdef FPU_LAZY     
	scheduler_fpu_lazy_request();
#else
	panic("fpu fault");
#endif
}



void page_fault(__u8 n, __native stack[])
{
	print_info_errcode(n,stack);
	printf("Page fault address: %Q\n", read_cr2());
	panic("page fault\n");
}

void syscall(__u8 n, __native stack[])
{
	printf("cpu%d: syscall\n", CPU->id);
	thread_usleep(1000);
}

void tlb_shootdown_ipi(__u8 n, __native stack[])
{
	trap_virtual_eoi();
	tlb_shootdown_ipi_recv();
}

void wakeup_ipi(__u8 n, __native stack[])
{
	trap_virtual_eoi();
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
