/*
 * Copyright (c) 2015 Petr Pavlu
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

/** @addtogroup kernel_arm64
 * @{
 */
/** @file
 * @brief Declarations of functions implemented in assembly.
 */

#ifndef KERN_arm64_ASM_H_
#define KERN_arm64_ASM_H_

#include <config.h>
#include <trace.h>

extern char exc_vector;

/*
 * Note: Function asm_delay_loop() is defined in arm64.c but declared here
 * because the generic kernel code expects it in arch/asm.h.
 */
extern void asm_delay_loop(uint32_t usec);

/** CPU specific way to sleep cpu. */
_NO_TRACE static inline void cpu_sleep(void)
{
	asm volatile ("wfe");
}

/** Enables interrupts and blocks until an interrupt arrives,
 * atomically if possible on target architecture.
 * Disables interrupts again before returning to caller.
 */
_NO_TRACE static inline void cpu_interruptible_sleep(void)
{
	// FIXME: do this atomically
	interrupts_enable();
	cpu_sleep();
	interrupts_disable();
}

/** Halts CPU. */
_NO_TRACE static inline __attribute__((noreturn)) void cpu_halt(void)
{
	while (true)
		;
}

_NO_TRACE static inline void cpu_spin_hint(void)
{
	asm volatile ("yield");
}

/** Output byte to port.
 *
 * @param port Port to write to.
 * @param val  Value to write.
 */
_NO_TRACE static inline void pio_write_8(ioport8_t *port, uint8_t val)
{
	*port = val;
}

/** Output half-word to port.
 *
 * @param port Port to write to.
 * @param val  Value to write.
 */
_NO_TRACE static inline void pio_write_16(ioport16_t *port, uint16_t val)
{
	*port = val;
}

/** Output word to port.
 *
 * @param port Port to write to.
 * @param val  Value to write.
 */
_NO_TRACE static inline void pio_write_32(ioport32_t *port, uint32_t val)
{
	*port = val;
}

/** Get byte from port.
 *
 * @param port Port to read from.
 * @return Value read.
 */
_NO_TRACE static inline uint8_t pio_read_8(const ioport8_t *port)
{
	return *port;
}

/** Get word from port.
 *
 * @param port Port to read from.
 * @return Value read.
 */
_NO_TRACE static inline uint16_t pio_read_16(const ioport16_t *port)
{
	return *port;
}

/** Get double word from port.
 *
 * @param port Port to read from.
 * @return Value read.
 */
_NO_TRACE static inline uint32_t pio_read_32(const ioport32_t *port)
{
	return *port;
}

#endif

/** @}
 */
