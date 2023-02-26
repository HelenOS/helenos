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

/** @addtogroup kernel_abs32le
 * @{
 */
/** @file
 */

#ifndef KERN_abs32le_ASM_H_
#define KERN_abs32le_ASM_H_

#include <typedefs.h>
#include <config.h>
#include <trace.h>

_NO_TRACE static inline void asm_delay_loop(uint32_t usec)
{
}

_NO_TRACE static inline __attribute__((noreturn)) void cpu_halt(void)
{
	/*
	 * On real hardware this should stop processing further
	 * instructions on the CPU (and possibly putting it into
	 * low-power mode) without any possibility of exitting
	 * this function.
	 */

	while (true)
		;
}

_NO_TRACE static inline void cpu_sleep(void)
{
	/*
	 * On real hardware this should put the CPU into low-power
	 * mode. However, the CPU is free to continue processing
	 * futher instructions any time. The CPU also wakes up
	 * upon an interrupt.
	 */
}

_NO_TRACE static inline void cpu_spin_hint(void)
{
	/*
	 * Some ISAs have a special instruction for the body of a busy wait loop,
	 * such as in spinlock and the like. Using it allows the CPU to optimize
	 * its operation. For an example, see the "pause" instruction on x86.
	 */
}

_NO_TRACE static inline void pio_write_8(ioport8_t *port, uint8_t val)
{
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
	return 0;
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
	return 0;
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
	return 0;
}

_NO_TRACE static inline ipl_t interrupts_enable(void)
{
	/*
	 * On real hardware this unconditionally enables preemption
	 * by internal and external interrupts.
	 *
	 * The return value stores the previous interrupt level.
	 */

	return 0;
}

_NO_TRACE static inline ipl_t interrupts_disable(void)
{
	/*
	 * On real hardware this disables preemption by the usual
	 * set of internal and external interrupts. This does not
	 * apply to special non-maskable interrupts and sychronous
	 * CPU exceptions.
	 *
	 * The return value stores the previous interrupt level.
	 */

	return 0;
}

_NO_TRACE static inline void interrupts_restore(ipl_t ipl)
{
	/*
	 * On real hardware this either enables or disables preemption
	 * according to the interrupt level value from the argument.
	 */
}

_NO_TRACE static inline ipl_t interrupts_read(void)
{
	/*
	 * On real hardware the return value stores the current interrupt
	 * level.
	 */

	return 0;
}

_NO_TRACE static inline bool interrupts_disabled(void)
{
	/*
	 * On real hardware the return value is true iff interrupts are
	 * disabled.
	 */

	return false;
}

/** Enables interrupts and blocks until an interrupt arrives,
 * atomically if possible on target architecture.
 * Disables interrupts again before returning to caller.
 */
_NO_TRACE static inline void cpu_interruptible_sleep(void)
{
	interrupts_enable();
	cpu_sleep();
	interrupts_disable();
}

#endif

/** @}
 */
