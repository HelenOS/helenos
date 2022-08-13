/*
 * SPDX-FileCopyrightText: 2015 Petr Pavlu
 *
 * SPDX-License-Identifier: BSD-3-Clause
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

/** Halts CPU. */
_NO_TRACE static inline __attribute__((noreturn)) void cpu_halt(void)
{
	while (true)
		;
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
