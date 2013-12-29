/*
 * Copyright (c) 2010 Martin Decky
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

/** @addtogroup sparc32
 * @{
 */
/** @file
 */

#ifndef KERN_sparc32_ASM_H_
#define KERN_sparc32_ASM_H_

#include <typedefs.h>
#include <config.h>
#include <trace.h>
#include <arch/register.h>

NO_TRACE static inline void asm_delay_loop(uint32_t usec)
{
	// FIXME TODO
}

NO_TRACE static inline __attribute__((noreturn)) void cpu_halt(void)
{
	// FIXME TODO
	while (true);
}

NO_TRACE static inline void cpu_sleep(void)
{
	// FIXME TODO
}

NO_TRACE static inline void pio_write_8(ioport8_t *port, uint8_t val)
{
	*port = val;
}

NO_TRACE static inline void pio_write_16(ioport16_t *port, uint16_t val)
{
	*port = val;
}

NO_TRACE static inline void pio_write_32(ioport32_t *port, uint32_t val)
{
	*port = val;
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

NO_TRACE static inline uint32_t psr_read(void)
{
	uint32_t v;
	
	asm volatile (
		"mov %%psr, %[v]\n"
		: [v] "=r" (v)
	);
	
	return v;
}

NO_TRACE static inline uint32_t wim_read(void)
{
	uint32_t v;
	
	asm volatile (
		"mov %%wim, %[v]\n"
		: [v] "=r" (v)
	);
	
	return v;
}

NO_TRACE static inline uint32_t asi_u32_read(unsigned int asi, uintptr_t va)
{
	uint32_t v;
	
	asm volatile (
		"lda [%[va]] %[asi], %[v]\n"
		: [v] "=r" (v)
		: [va] "r" (va),
		  [asi] "i" (asi)
	);
	
	return v;
}

NO_TRACE static inline void asi_u32_write(unsigned int asi, uintptr_t va,
    uint32_t v)
{
	asm volatile (
		"sta %[v], [%[va]] %[asi]\n"
		:: [v] "r" (v),
		   [va] "r" (va),
		   [asi] "i" (asi)
		: "memory"
	);
}

NO_TRACE static inline void psr_write(uint32_t psr)
{
	asm volatile (
		"mov %[v], %%psr\n"
		:: [v] "r" (psr)
	);
}

NO_TRACE static inline void wim_write(uint32_t wim)
{
	asm volatile (
		"mov %[v], %%wim\n"
		:: [v] "r" (wim)
	);
}

NO_TRACE static inline ipl_t interrupts_enable(void)
{
	psr_reg_t psr;
	psr.value = psr_read();
	
	ipl_t pil;
	pil = psr.pil;
	
	psr.pil = 0x00;
	psr_write(psr.value);
	
	return pil;
}

NO_TRACE static inline ipl_t interrupts_disable(void)
{
	psr_reg_t psr;
	psr.value = psr_read();
	
	ipl_t pil;
	pil = psr.pil;
	
	psr.pil = 0x0f;
	psr_write(psr.value);
	
	return pil;
}

NO_TRACE static inline void interrupts_restore(ipl_t ipl)
{
	psr_reg_t psr;
	psr.value = psr_read();
	psr.pil = ipl;
	psr_write(psr.value);
}

NO_TRACE static inline ipl_t interrupts_read(void)
{
	psr_reg_t psr;
	psr.value = psr_read();
	return psr.pil;
}

NO_TRACE static inline bool interrupts_disabled(void)
{
	psr_reg_t psr;
	psr.value = psr_read();
	return (psr.pil == 0x0f);
}

NO_TRACE static inline uintptr_t get_stack_base(void)
{
	uintptr_t v;
	
	asm volatile (
		"and %%sp, %[size], %[v]\n" 
		: [v] "=r" (v)
		: [size] "r" (~(STACK_SIZE - 1))
	);
	
	return v;
}

#endif

/** @}
 */
