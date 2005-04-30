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
#include <panic.h>
#include <arch/i8259.h>
#include <func.h>
#include <cpu.h>
#include <arch/asm.h>
#include <mm/tlb.h>
#include <arch.h>

/*
 * Interrupt and exception dispatching.
 */

static iroutine ivt[IVT_ITEMS];

void (* disable_irqs_function)(__u16 irqmask) = NULL;
void (* enable_irqs_function)(__u16 irqmask) = NULL;
void (* eoi_function)(void) = NULL;

iroutine trap_register(__u8 n, iroutine f)
{
	iroutine old;
    
	old = ivt[n];
	ivt[n] = f;
    
    	return old;
}

/*
 * Called directly from the assembler code.
 * CPU is cpu_priority_high().
 */
void trap_dispatcher(__u8 n, __u32 stack[])
{
	ivt[n](n,stack);
}

void null_interrupt(__u8 n, __u32 stack[])
{
	printf("int %d: null_interrupt\n", n);
	printf("stack: %L, %L, %L, %L\n", stack[0], stack[1], stack[2], stack[3]);
	panic("unserviced interrupt\n");
}

void gp_fault(__u8 n, __u32 stack[])
{
	printf("stack[0]=%X, %%eip=%X, %%cs=%X, flags=%X\n", stack[0], stack[1], stack[2], stack[3]);
	printf("%%eax=%L, %%ebx=%L, %%ecx=%L, %%edx=%L,\n%%edi=%L, %%esi=%L, %%ebp=%L, %%esp=%L\n", stack[-2], stack[-5], stack[-3], stack[-4], stack[-9], stack[-8], stack[-1], stack);
	printf("stack: %X, %X, %X, %X\n", stack[4], stack[5], stack[6], stack[7]);
	panic("general protection fault\n");
}

void page_fault(__u8 n, __u32 stack[])
{
	printf("page fault address: %X\n", cpu_read_cr2());
	printf("stack[0]=%X, %%eip=%X, %%cs=%X, flags=%X\n", stack[0], stack[1], stack[2], stack[3]);
	printf("%%eax=%L, %%ebx=%L, %%ecx=%L, %%edx=%L,\n%%edi=%L, %%esi=%L, %%ebp=%L, %%esp=%L\n", stack[-2], stack[-5], stack[-3], stack[-4], stack[-9], stack[-8], stack[-1], stack);
	printf("stack: %X, %X, %X, %X\n", stack[4], stack[5], stack[6], stack[7]);
	printf("page fault\n");
	cpu_halt();
}

void syscall(__u8 n, __u32 stack[])
{
	printf("cpu%d: syscall\n", CPU->id);
	thread_usleep(1000);
}

void tlb_shootdown_ipi(__u8 n, __u32 stack[])
{
	trap_virtual_eoi();
	tlb_shootdown_ipi_recv();
}

void wakeup_ipi(__u8 n, __u32 stack[])
{
	trap_virtual_eoi();
}

void trap_virtual_enable_irqs(__u16 irqmask)
{
	if (enable_irqs_function)
		enable_irqs_function(irqmask);
	else
		panic(PANIC "no enable_irqs_function\n");
}

void trap_virtual_disable_irqs(__u16 irqmask)
{
	if (disable_irqs_function)
		disable_irqs_function(irqmask);
	else
		panic(PANIC "no disable_irqs_function\n");
}

void trap_virtual_eoi(void)
{
	if (eoi_function)
		eoi_function();
	else
		panic(PANIC "no eoi_function\n");

}
