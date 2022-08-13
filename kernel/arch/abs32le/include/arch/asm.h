/*
 * SPDX-FileCopyrightText: 2010 Martin Decky
 *
 * SPDX-License-Identifier: BSD-3-Clause
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

#endif

/** @}
 */
