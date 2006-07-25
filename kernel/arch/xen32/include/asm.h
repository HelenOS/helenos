/*
 * Copyright (C) 2001-2004 Jakub Jermar
 * Copyright (C) 2005 Sergey Bondari
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

/** @addtogroup xen32
 * @{
 */
/** @file
 */

#ifndef __xen32_ASM_H__
#define __xen32_ASM_H__

#include <arch/pm.h>
#include <arch/types.h>
#include <config.h>

extern uint32_t interrupt_handler_size;

extern void paging_on(void);

extern void interrupt_handlers(void);

extern void enable_l_apic_in_msr(void);


extern void asm_delay_loop(uint32_t t);
extern void asm_fake_loop(uint32_t t);


/** Halt CPU
 *
 * Halt the current CPU until interrupt event.
 */
static inline void cpu_halt(void) { __asm__("hlt\n"); };
static inline void cpu_sleep(void) { __asm__("hlt\n"); };

#define GEN_READ_REG(reg) static inline unative_t read_ ##reg (void) \
    { \
	unative_t res; \
	__asm__ volatile ("movl %%" #reg ", %0" : "=r" (res) ); \
	return res; \
    }

#define GEN_WRITE_REG(reg) static inline void write_ ##reg (unative_t regn) \
    { \
	__asm__ volatile ("movl %0, %%" #reg : : "r" (regn)); \
    }

GEN_READ_REG(cr0);
GEN_READ_REG(cr2);
GEN_READ_REG(cr3);
GEN_WRITE_REG(cr3);

GEN_READ_REG(dr0);
GEN_READ_REG(dr1);
GEN_READ_REG(dr2);
GEN_READ_REG(dr3);
GEN_READ_REG(dr6);
GEN_READ_REG(dr7);

GEN_WRITE_REG(dr0);
GEN_WRITE_REG(dr1);
GEN_WRITE_REG(dr2);
GEN_WRITE_REG(dr3);
GEN_WRITE_REG(dr6);
GEN_WRITE_REG(dr7);

/** Byte to port
 *
 * Output byte to port
 *
 * @param port Port to write to
 * @param val Value to write
 */
static inline void outb(uint16_t port, uint8_t val) { __asm__ volatile ("outb %b0, %w1\n" : : "a" (val), "d" (port) ); }

/** Word to port
 *
 * Output word to port
 *
 * @param port Port to write to
 * @param val Value to write
 */
static inline void outw(uint16_t port, uint16_t val) { __asm__ volatile ("outw %w0, %w1\n" : : "a" (val), "d" (port) ); }

/** Double word to port
 *
 * Output double word to port
 *
 * @param port Port to write to
 * @param val Value to write
 */
static inline void outl(uint16_t port, uint32_t val) { __asm__ volatile ("outl %l0, %w1\n" : : "a" (val), "d" (port) ); }

/** Byte from port
 *
 * Get byte from port
 *
 * @param port Port to read from
 * @return Value read
 */
static inline uint8_t inb(uint16_t port) { uint8_t val; __asm__ volatile ("inb %w1, %b0 \n" : "=a" (val) : "d" (port) ); return val; }

/** Word from port
 *
 * Get word from port
 *
 * @param port Port to read from
 * @return Value read
 */
static inline uint16_t inw(uint16_t port) { uint16_t val; __asm__ volatile ("inw %w1, %w0 \n" : "=a" (val) : "d" (port) ); return val; }

/** Double word from port
 *
 * Get double word from port
 *
 * @param port Port to read from
 * @return Value read
 */
static inline uint32_t inl(uint16_t port) { uint32_t val; __asm__ volatile ("inl %w1, %l0 \n" : "=a" (val) : "d" (port) ); return val; }

/** Enable interrupts.
 *
 * Enable interrupts and return previous
 * value of EFLAGS.
 *
 * @return Old interrupt priority level.
 */
static inline ipl_t interrupts_enable(void)
{
	ipl_t v = 0;
/*	__asm__ volatile (
		"pushf\n\t"
		"popl %0\n\t"
		"sti\n"
		: "=r" (v)
	);*/
	return v;
}

/** Disable interrupts.
 *
 * Disable interrupts and return previous
 * value of EFLAGS.
 *
 * @return Old interrupt priority level.
 */
static inline ipl_t interrupts_disable(void)
{
	ipl_t v = 0;
/*	__asm__ volatile (
		"pushf\n\t"
		"popl %0\n\t"
		"cli\n"
		: "=r" (v)
	);*/
	return v;
}

/** Restore interrupt priority level.
 *
 * Restore EFLAGS.
 *
 * @param ipl Saved interrupt priority level.
 */
static inline void interrupts_restore(ipl_t ipl)
{
/*	__asm__ volatile (
		"pushl %0\n\t"
		"popf\n"
		: : "r" (ipl)
	);*/
}

/** Return interrupt priority level.
 *
 * @return EFLAFS.
 */
static inline ipl_t interrupts_read(void)
{
	ipl_t v = 0;
/*	__asm__ volatile (
		"pushf\n\t"
		"popl %0\n"
		: "=r" (v)
	);*/
	return v;
}

/** Return base address of current stack
 *
 * Return the base address of the current stack.
 * The stack is assumed to be STACK_SIZE bytes long.
 * The stack must start on page boundary.
 */
static inline uintptr_t get_stack_base(void)
{
	uintptr_t v;
	
	__asm__ volatile ("andl %%esp, %0\n" : "=r" (v) : "0" (~(STACK_SIZE-1)));
	
	return v;
}

static inline uint64_t rdtsc(void)
{
	uint64_t v;
	
	__asm__ volatile("rdtsc\n" : "=A" (v));
	
	return v;
}

/** Return current IP address */
static inline uintptr_t * get_ip() 
{
	uintptr_t *ip;

	__asm__ volatile (
		"mov %%eip, %0"
		: "=r" (ip)
		);
	return ip;
}

/** Invalidate TLB Entry.
 *
 * @param addr Address on a page whose TLB entry is to be invalidated.
 */
static inline void invlpg(uintptr_t addr)
{
	__asm__ volatile ("invlpg %0\n" :: "m" (*(unative_t *)addr));
}

/** Load GDTR register from memory.
 *
 * @param gdtr_reg Address of memory from where to load GDTR.
 */
static inline void gdtr_load(ptr_16_32_t *gdtr_reg)
{
	__asm__ volatile ("lgdtl %0\n" : : "m" (*gdtr_reg));
}

/** Store GDTR register to memory.
 *
 * @param gdtr_reg Address of memory to where to load GDTR.
 */
static inline void gdtr_store(ptr_16_32_t *gdtr_reg)
{
	__asm__ volatile ("sgdtl %0\n" : : "m" (*gdtr_reg));
}

/** Load IDTR register from memory.
 *
 * @param idtr_reg Address of memory from where to load IDTR.
 */
static inline void idtr_load(ptr_16_32_t *idtr_reg)
{
	__asm__ volatile ("lidtl %0\n" : : "m" (*idtr_reg));
}

/** Load TR from descriptor table.
 *
 * @param sel Selector specifying descriptor of TSS segment.
 */
static inline void tr_load(uint16_t sel)
{
	__asm__ volatile ("ltr %0" : : "r" (sel));
}

#endif

/** @}
 */
