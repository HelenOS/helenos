/*
 * Copyright (c) 2001-2004 Jakub Jermar
 * Copyright (c) 2005 Sergey Bondari
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

/** @addtogroup kernel_ia32
 * @{
 */
/** @file
 */

#ifndef KERN_ia32_ASM_H_
#define KERN_ia32_ASM_H_

#include <arch/pm.h>
#include <arch/cpu.h>
#include <typedefs.h>
#include <config.h>
#include <trace.h>

/** Halt CPU
 *
 * Halt the current CPU.
 *
 */
_NO_TRACE static inline __attribute__((noreturn)) void cpu_halt(void)
{
	while (true) {
		asm volatile (
		    "hlt\n"
		);
	}
}

_NO_TRACE static inline void cpu_sleep(void)
{
	asm volatile (
	    "hlt\n"
	);
}

#define GEN_READ_REG(reg) _NO_TRACE static inline sysarg_t read_ ##reg (void) \
	{ \
		sysarg_t res; \
		asm volatile ( \
			"movl %%" #reg ", %[res]" \
			: [res] "=r" (res) \
		); \
		return res; \
	}

#define GEN_WRITE_REG(reg) _NO_TRACE static inline void write_ ##reg (sysarg_t regn) \
	{ \
		asm volatile ( \
			"movl %[regn], %%" #reg \
			:: [regn] "r" (regn) \
		); \
	}

GEN_READ_REG(cr0);
GEN_READ_REG(cr2);
GEN_READ_REG(cr3);
GEN_WRITE_REG(cr3);

GEN_WRITE_REG(cr0);

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

#define IO_SPACE_BOUNDARY	((void *) (64 * 1024))

/** Byte to port
 *
 * Output byte to port
 *
 * @param port Port to write to
 * @param val Value to write
 *
 */
_NO_TRACE static inline void pio_write_8(ioport8_t *port, uint8_t val)
{
	if (port < (ioport8_t *) IO_SPACE_BOUNDARY) {
		asm volatile (
		    "outb %b[val], %w[port]\n"
		    :: [val] "a" (val), [port] "d" (port)
		);
	} else
		*port = val;
}

/** Word to port
 *
 * Output word to port
 *
 * @param port Port to write to
 * @param val Value to write
 *
 */
_NO_TRACE static inline void pio_write_16(ioport16_t *port, uint16_t val)
{
	if (port < (ioport16_t *) IO_SPACE_BOUNDARY) {
		asm volatile (
		    "outw %w[val], %w[port]\n"
		    :: [val] "a" (val), [port] "d" (port)
		);
	} else
		*port = val;
}

/** Double word to port
 *
 * Output double word to port
 *
 * @param port Port to write to
 * @param val Value to write
 *
 */
_NO_TRACE static inline void pio_write_32(ioport32_t *port, uint32_t val)
{
	if (port < (ioport32_t *) IO_SPACE_BOUNDARY) {
		asm volatile (
		    "outl %[val], %w[port]\n"
		    :: [val] "a" (val), [port] "d" (port)
		);
	} else
		*port = val;
}

/** Byte from port
 *
 * Get byte from port
 *
 * @param port Port to read from
 * @return Value read
 *
 */
_NO_TRACE static inline uint8_t pio_read_8(ioport8_t *port)
{
	if (((void *)port) < IO_SPACE_BOUNDARY) {
		uint8_t val;

		asm volatile (
		    "inb %w[port], %b[val]\n"
		    : [val] "=a" (val)
		    : [port] "d" (port)
		);

		return val;
	} else
		return (uint8_t) *port;
}

/** Word from port
 *
 * Get word from port
 *
 * @param port Port to read from
 * @return Value read
 *
 */
_NO_TRACE static inline uint16_t pio_read_16(ioport16_t *port)
{
	if (((void *)port) < IO_SPACE_BOUNDARY) {
		uint16_t val;

		asm volatile (
		    "inw %w[port], %w[val]\n"
		    : [val] "=a" (val)
		    : [port] "d" (port)
		);

		return val;
	} else
		return (uint16_t) *port;
}

/** Double word from port
 *
 * Get double word from port
 *
 * @param port Port to read from
 * @return Value read
 *
 */
_NO_TRACE static inline uint32_t pio_read_32(ioport32_t *port)
{
	if (((void *)port) < IO_SPACE_BOUNDARY) {
		uint32_t val;

		asm volatile (
		    "inl %w[port], %[val]\n"
		    : [val] "=a" (val)
		    : [port] "d" (port)
		);

		return val;
	} else
		return (uint32_t) *port;
}

_NO_TRACE static inline uint32_t read_eflags(void)
{
	uint32_t eflags;

	asm volatile (
	    "pushf\n"
	    "popl %[v]\n"
	    : [v] "=r" (eflags)
	);

	return eflags;
}

_NO_TRACE static inline void write_eflags(uint32_t eflags)
{
	asm volatile (
	    "pushl %[v]\n"
	    "popf\n"
	    :: [v] "r" (eflags)
	);
}

/** Return interrupt priority level.
 *
 * @return Current interrupt priority level.
 *
 */
_NO_TRACE static inline ipl_t interrupts_read(void)
{
	return (ipl_t) read_eflags();
}

/** Enable interrupts.
 *
 * Enable interrupts and return the previous interrupt priority level.
 *
 * @return Old interrupt priority level.
 *
 */
_NO_TRACE static inline ipl_t interrupts_enable(void)
{
	ipl_t ipl = interrupts_read();

	asm volatile ("sti\n");

	return ipl;
}

/** Disable interrupts.
 *
 * Disable interrupts and return the previous interrupt priority level.
 *
 * @return Old interrupt priority level.
 *
 */
_NO_TRACE static inline ipl_t interrupts_disable(void)
{
	ipl_t ipl = interrupts_read();

	asm volatile ("cli\n");

	return ipl;
}

/** Restore interrupt priority level.
 *
 * Restore a saved interrupt priority level.
 *
 * @param ipl Saved interrupt priority level.
 *
 */
_NO_TRACE static inline void interrupts_restore(ipl_t ipl)
{
	write_eflags((uint32_t) ipl);
}

/** Check interrupts state.
 *
 * @return True if interrupts are disabled.
 *
 */
_NO_TRACE static inline bool interrupts_disabled(void)
{
	return ((read_eflags() & EFLAGS_IF) == 0);
}

#ifndef PROCESSOR_i486

/** Write to MSR */
_NO_TRACE static inline void write_msr(uint32_t msr, uint64_t value)
{
	asm volatile (
	    "wrmsr"
	    :: "c" (msr),
	      "a" ((uint32_t) (value)),
	      "d" ((uint32_t) (value >> 32))
	);
}

_NO_TRACE static inline uint64_t read_msr(uint32_t msr)
{
	uint32_t ax, dx;

	asm volatile (
	    "rdmsr"
	    : "=a" (ax),
	      "=d" (dx)
	    : "c" (msr)
	);

	return ((uint64_t) dx << 32) | ax;
}

#endif /* PROCESSOR_i486 */

/** Return base address of current stack
 *
 * Return the base address of the current stack.
 * The stack is assumed to be STACK_SIZE bytes long.
 * The stack must start on page boundary.
 *
 */
_NO_TRACE static inline uintptr_t get_stack_base(void)
{
	uintptr_t v;

	asm volatile (
	    "andl %%esp, %[v]\n"
	    : [v] "=r" (v)
	    : "0" (~(STACK_SIZE - 1))
	);

	return v;
}

/** Invalidate TLB Entry.
 *
 * @param addr Address on a page whose TLB entry is to be invalidated.
 *
 */
_NO_TRACE static inline void invlpg(uintptr_t addr)
{
	asm volatile (
	    "invlpg %[addr]\n"
	    :: [addr] "m" (*(sysarg_t *) addr)
	);
}

/** Load GDTR register from memory.
 *
 * @param gdtr_reg Address of memory from where to load GDTR.
 *
 */
_NO_TRACE static inline void gdtr_load(ptr_16_32_t *gdtr_reg)
{
	asm volatile (
	    "lgdtl %[gdtr_reg]\n"
	    :: [gdtr_reg] "m" (*gdtr_reg)
	);
}

/** Store GDTR register to memory.
 *
 * @param gdtr_reg Address of memory to where to load GDTR.
 *
 */
_NO_TRACE static inline void gdtr_store(ptr_16_32_t *gdtr_reg)
{
	asm volatile (
	    "sgdtl %[gdtr_reg]\n"
	    : [gdtr_reg] "=m" (*gdtr_reg)
	);
}

/** Load IDTR register from memory.
 *
 * @param idtr_reg Address of memory from where to load IDTR.
 *
 */
_NO_TRACE static inline void idtr_load(ptr_16_32_t *idtr_reg)
{
	asm volatile (
	    "lidtl %[idtr_reg]\n"
	    :: [idtr_reg] "m" (*idtr_reg)
	);
}

/** Load TR from descriptor table.
 *
 * @param sel Selector specifying descriptor of TSS segment.
 *
 */
_NO_TRACE static inline void tr_load(uint16_t sel)
{
	asm volatile (
	    "ltr %[sel]"
	    :: [sel] "r" (sel)
	);
}

/** Load GS from descriptor table.
 *
 * @param sel Selector specifying descriptor of the GS segment.
 *
 */
_NO_TRACE static inline void gs_load(uint16_t sel)
{
	asm volatile (
	    "mov %[sel], %%gs"
	    :: [sel] "r" (sel)
	);
}

extern void paging_on(void);
extern void enable_l_apic_in_msr(void);

extern void asm_delay_loop(uint32_t);
extern void asm_fake_loop(uint32_t);

extern uintptr_t int_syscall;

extern uintptr_t int_0;
extern uintptr_t int_1;
extern uintptr_t int_2;
extern uintptr_t int_3;
extern uintptr_t int_4;
extern uintptr_t int_5;
extern uintptr_t int_6;
extern uintptr_t int_7;
extern uintptr_t int_8;
extern uintptr_t int_9;
extern uintptr_t int_10;
extern uintptr_t int_11;
extern uintptr_t int_12;
extern uintptr_t int_13;
extern uintptr_t int_14;
extern uintptr_t int_15;
extern uintptr_t int_16;
extern uintptr_t int_17;
extern uintptr_t int_18;
extern uintptr_t int_19;
extern uintptr_t int_20;
extern uintptr_t int_21;
extern uintptr_t int_22;
extern uintptr_t int_23;
extern uintptr_t int_24;
extern uintptr_t int_25;
extern uintptr_t int_26;
extern uintptr_t int_27;
extern uintptr_t int_28;
extern uintptr_t int_29;
extern uintptr_t int_30;
extern uintptr_t int_31;
extern uintptr_t int_32;
extern uintptr_t int_33;
extern uintptr_t int_34;
extern uintptr_t int_35;
extern uintptr_t int_36;
extern uintptr_t int_37;
extern uintptr_t int_38;
extern uintptr_t int_39;
extern uintptr_t int_40;
extern uintptr_t int_41;
extern uintptr_t int_42;
extern uintptr_t int_43;
extern uintptr_t int_44;
extern uintptr_t int_45;
extern uintptr_t int_46;
extern uintptr_t int_47;
extern uintptr_t int_48;
extern uintptr_t int_49;
extern uintptr_t int_50;
extern uintptr_t int_51;
extern uintptr_t int_52;
extern uintptr_t int_53;
extern uintptr_t int_54;
extern uintptr_t int_55;
extern uintptr_t int_56;
extern uintptr_t int_57;
extern uintptr_t int_58;
extern uintptr_t int_59;
extern uintptr_t int_60;
extern uintptr_t int_61;
extern uintptr_t int_62;
extern uintptr_t int_63;

#endif

/** @}
 */
