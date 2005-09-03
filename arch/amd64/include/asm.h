/*
 * Copyright (C) 2005 Jakub Jermar
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

#ifndef __amd64_ASM_H__
#define __amd64_ASM_H__

#include <arch/types.h>
#include <config.h>


void asm_delay_loop(__u32 t);
void asm_fake_loop(__u32 t);

static inline __address get_stack_base(void)
{
	__address v;
	
	__asm__ volatile ("andq %%rsp, %0\n" : "=r" (v) : "0" (~((__u64)STACK_SIZE-1)));
	
	return v;
}

static inline void cpu_sleep(void) { __asm__("hlt"); };
static inline void cpu_halt(void) { __asm__("hlt"); };


static inline __u8 inb(__u16 port) 
{
	__u8 out;

	__asm__ volatile (
		"mov %1, %%dx;"
		"inb %%dx,%%al;"
		"mov %%al, %0;"
		:"=m"(out)
		:"m"(port)
		:"%rdx","%rax"
		);
	return out;
}

static inline __u8 outb(__u16 port,__u8 b) 
{
	__asm__ volatile (
		"mov %0,%%dx;"
		"mov %1,%%al;"
		"outb %%al,%%dx;"
		:
		:"m"( port), "m" (b)
		:"%rdx","%rax"
		);
}

/** Set priority level low
 *
 * Enable interrupts and return previous
 * value of EFLAGS.
 */
static inline pri_t cpu_priority_low(void) {
	pri_t v;
	__asm__ volatile (
		"pushfq\n"
		"popq %0\n"
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
		"pushfq\n"
		"popq %0\n"
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
		"pushq %0\n"
		"popfq\n"
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
		"pushfq\n"
		"popq %0\n"
		: "=r" (v)
	);
	return v;
}

/** Read CR2
 *
 * Return value in CR2
 *
 * @return Value read.
 */
static inline __u64 read_cr2(void) { __u64 v; __asm__ volatile ("movq %%cr2,%0" : "=r" (v)); return v; }

/** Write CR3
 *
 * Write value to CR3.
 *
 * @param v Value to be written.
 */
static inline void write_cr3(__u64 v) { __asm__ volatile ("movq %0,%%cr3\n" : : "r" (v)); }

/** Read CR3
 *
 * Return value in CR3
 *
 * @return Value read.
 */
static inline __u64 read_cr3(void) { __u64 v; __asm__ volatile ("movq %%cr3,%0" : "=r" (v)); return v; }


extern size_t interrupt_handler_size;
extern void interrupt_handlers(void);

#endif
