/*
 * Copyright (c) 2005 Martin Decky
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

/** @addtogroup kernel_ppc32
 * @{
 */
/** @file
 */

#ifndef KERN_ppc32_ASM_H_
#define KERN_ppc32_ASM_H_

#include <typedefs.h>
#include <config.h>
#include <arch/msr.h>
#include <arch/mm/asid.h>
#include <trace.h>

_NO_TRACE static inline void cpu_spin_hint(void)
{
}

_NO_TRACE static inline uint32_t msr_read(void)
{
	uint32_t msr;

	asm volatile (
	    "mfmsr %[msr]\n"
	    : [msr] "=r" (msr)
	);

	return msr;
}

_NO_TRACE static inline void msr_write(uint32_t msr)
{
	asm volatile (
	    "mtmsr %[msr]\n"
	    "isync\n"
	    :: [msr] "r" (msr)
	);
}

_NO_TRACE static inline void sr_set(uint32_t flags, asid_t asid, uint32_t sr)
{
	asm volatile (
	    "mtsrin %[value], %[sr]\n"
	    "sync\n"
	    "isync\n"
	    :: [value] "r" ((flags << 16) + (asid << 4) + sr),
	      [sr] "r" (sr << 28)
	);
}

_NO_TRACE static inline uint32_t sr_get(uint32_t vaddr)
{
	uint32_t vsid;

	asm volatile (
	    "mfsrin %[vsid], %[vaddr]\n"
	    : [vsid] "=r" (vsid)
	    : [vaddr] "r" (vaddr)
	);

	return vsid;
}

_NO_TRACE static inline uint32_t sdr1_get(void)
{
	uint32_t sdr1;

	asm volatile (
	    "mfsdr1 %[sdr1]\n"
	    : [sdr1] "=r" (sdr1)
	);

	return sdr1;
}

/** Enable interrupts.
 *
 * Enable interrupts and return previous
 * value of EE.
 *
 * @return Old interrupt priority level.
 *
 */
_NO_TRACE static inline ipl_t interrupts_enable(void)
{
	ipl_t ipl = msr_read();
	msr_write(ipl | MSR_EE);
	return ipl;
}

/** Disable interrupts.
 *
 * Disable interrupts and return previous
 * value of EE.
 *
 * @return Old interrupt priority level.
 *
 */
_NO_TRACE static inline ipl_t interrupts_disable(void)
{
	ipl_t ipl = msr_read();
	msr_write(ipl & (~MSR_EE));
	return ipl;
}

/** Restore interrupt priority level.
 *
 * Restore EE.
 *
 * @param ipl Saved interrupt priority level.
 *
 */
_NO_TRACE static inline void interrupts_restore(ipl_t ipl)
{
	msr_write((msr_read() & (~MSR_EE)) | (ipl & MSR_EE));
}

/** Return interrupt priority level.
 *
 * Return EE.
 *
 * @return Current interrupt priority level.
 *
 */
_NO_TRACE static inline ipl_t interrupts_read(void)
{
	return msr_read();
}

/** Check whether interrupts are disabled.
 *
 * @return True if interrupts are disabled.
 *
 */
_NO_TRACE static inline bool interrupts_disabled(void)
{
	return ((msr_read() & MSR_EE) == 0);
}

/** Enables interrupts and blocks until an interrupt arrives,
 * atomically if possible on target architecture.
 * Disables interrupts again before returning to caller.
 */
_NO_TRACE static inline void cpu_interruptible_sleep(void)
{
	// FIXME: do this properly
	interrupts_enable();
	interrupts_disable();
}

_NO_TRACE static inline void pio_write_8(ioport8_t *port, uint8_t v)
{
	*port = v;
}

_NO_TRACE static inline void pio_write_16(ioport16_t *port, uint16_t v)
{
	*port = v;
}

_NO_TRACE static inline void pio_write_32(ioport32_t *port, uint32_t v)
{
	*port = v;
}

_NO_TRACE static inline uint8_t pio_read_8(ioport8_t *port)
{
	return *port;
}

_NO_TRACE static inline uint16_t pio_read_16(ioport16_t *port)
{
	return *port;
}

_NO_TRACE static inline uint32_t pio_read_32(ioport32_t *port)
{
	return *port;
}

extern void cpu_halt(void) __attribute__((noreturn));
extern void asm_delay_loop(uint32_t t);
extern void userspace_asm(uintptr_t uspace_uarg, uintptr_t stack, uintptr_t entry);

#endif

/** @}
 */
