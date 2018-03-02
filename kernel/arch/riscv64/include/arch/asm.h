/*
 * Copyright (c) 2016 Martin Decky
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

/** @addtogroup riscv64
 * @{
 */
/** @file
 */

#ifndef KERN_riscv64_ASM_H_
#define KERN_riscv64_ASM_H_

#include <arch/cpu.h>
#include <typedefs.h>
#include <config.h>
#include <arch/mm/asid.h>
#include <trace.h>

NO_TRACE static inline ipl_t interrupts_enable(void)
{
	ipl_t ipl;

	asm volatile (
		"csrrsi %[ipl], sstatus, " STRING(SSTATUS_SIE_MASK) "\n"
		: [ipl] "=r" (ipl)
	);

	return ipl;
}

NO_TRACE static inline ipl_t interrupts_disable(void)
{
	ipl_t ipl;

	asm volatile (
		"csrrci %[ipl], sstatus, " STRING(SSTATUS_SIE_MASK) "\n"
		: [ipl] "=r" (ipl)
	);

	return ipl;
}

NO_TRACE static inline void interrupts_restore(ipl_t ipl)
{
	if ((ipl & SSTATUS_SIE_MASK) == SSTATUS_SIE_MASK)
		interrupts_enable();
	else
		interrupts_disable();
}

NO_TRACE static inline ipl_t interrupts_read(void)
{
	ipl_t ipl;

	asm volatile (
		"csrr %[ipl], sstatus\n"
		: [ipl] "=r" (ipl)
	);

	return ipl;
}

NO_TRACE static inline bool interrupts_disabled(void)
{
	return ((interrupts_read() & SSTATUS_SIE_MASK) == 0);
}

NO_TRACE static inline uintptr_t get_stack_base(void)
{
	uintptr_t base;

	asm volatile (
		"and %[base], sp, %[mask]\n"
		: [base] "=r" (base)
		: [mask] "r" (~(STACK_SIZE - 1))
	);

	return base;
}

NO_TRACE static inline void cpu_sleep(void)
{
}

NO_TRACE static inline void pio_write_8(ioport8_t *port, uint8_t v)
{
	*port = v;
}

NO_TRACE static inline void pio_write_16(ioport16_t *port, uint16_t v)
{
	*port = v;
}

NO_TRACE static inline void pio_write_32(ioport32_t *port, uint32_t v)
{
	*port = v;
}

NO_TRACE static inline uint8_t pio_read_8(ioport8_t *port)
{
	return *port;
}

NO_TRACE static inline uint16_t pio_read_16(ioport16_t *port)
{
	return *port;
}

NO_TRACE static inline uint32_t pio_read_32(ioport32_t *port)
{
	return *port;
}

extern void cpu_halt(void) __attribute__((noreturn));
extern void asm_delay_loop(uint32_t t);
extern void userspace_asm(uintptr_t uspace_uarg, uintptr_t stack,
    uintptr_t entry);

#endif

/** @}
 */
