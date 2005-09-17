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

#ifndef __ia32_ASM_H__
#define __ia32_ASM_H__

#include <arch/types.h>
#include <config.h>

extern __u32 interrupt_handler_size;

extern void paging_on(void);

extern void interrupt_handlers(void);

extern __u8 inb(int port);
extern __u16 inw(int port);
extern __u32 inl(int port);

extern void outw(int port, __u16 w);
extern void outl(int port, __u32 l);

extern void enable_l_apic_in_msr(void);


void asm_delay_loop(__u32 t);
void asm_fake_loop(__u32 t);


/** Halt CPU
 *
 * Halt the current CPU until interrupt event.
 */
static inline void cpu_halt(void) { __asm__("hlt\n"); };
static inline void cpu_sleep(void) { __asm__("hlt\n"); };

/** Read CR2
 *
 * Return value in CR2
 *
 * @return Value read.
 */
static inline __u32 read_cr2(void) { __u32 v; __asm__ volatile ("movl %%cr2,%0\n" : "=r" (v)); return v; }

/** Write CR3
 *
 * Write value to CR3.
 *
 * @param v Value to be written.
 */
static inline void write_cr3(__u32 v) { __asm__ volatile ("movl %0,%%cr3\n" : : "r" (v)); }

/** Read CR3
 *
 * Return value in CR3
 *
 * @return Value read.
 */
static inline __u32 read_cr3(void) { __u32 v; __asm__ volatile ("movl %%cr3,%0\n" : "=r" (v)); return v; }

/** Byte to port
 *
 * Output byte to port
 *
 * @param port Port to write to
 * @param val Value to write
 */
static inline void outb(__u16 port, __u8 val) { __asm__ volatile ("outb %b0, %w1\n" : : "a" (val), "d" (port) ); }


/** Set priority level low
 *
 * Enable interrupts and return previous
 * value of EFLAGS.
 */
static inline pri_t cpu_priority_low(void) {
	pri_t v;
	__asm__ volatile (
		"pushf\n"
		"popl %0\n"
		"sti\n"
		: "=r" (v)
	);
	return v;
}

/** Set priority level high
 *
 * Disable interrupts and return previous
 * value of EFLAGS.
 */
static inline pri_t cpu_priority_high(void) {
	pri_t v;
	__asm__ volatile (
		"pushf\n"
		"popl %0\n"
		"cli\n"
		: "=r" (v)
	);
	return v;
}

/** Restore priority level
 *
 * Restore EFLAGS.
 */
static inline void cpu_priority_restore(pri_t pri) {
	__asm__ volatile (
		"pushl %0\n"
		"popf\n"
		: : "r" (pri)
	);
}

/** Return raw priority level
 *
 * Return EFLAFS.
 */
static inline pri_t cpu_priority_read(void) {
	pri_t v;
	__asm__ volatile (
		"pushf\n"
		"popl %0\n"
		: "=r" (v)
	);
	return v;
}

/** Return base address of current stack
 *
 * Return the base address of the current stack.
 * The stack is assumed to be STACK_SIZE bytes long.
 * The stack must start on page boundary.
 */
static inline __address get_stack_base(void)
{
	__address v;
	
	__asm__ volatile ("andl %%esp, %0\n" : "=r" (v) : "0" (~(STACK_SIZE-1)));
	
	return v;
}

static inline __u64 rdtsc(void)
{
	__u64 v;
	
	__asm__ volatile("rdtsc\n" : "=A" (v));
	
	return v;
}


#endif
