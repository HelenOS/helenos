/*
 * SPDX-FileCopyrightText: 2003-2004 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_mips32
 * @{
 */
/** @file
 */

#ifndef KERN_mips32_ASM_H_
#define KERN_mips32_ASM_H_

#include <typedefs.h>
#include <config.h>
#include <trace.h>

_NO_TRACE static inline void cpu_sleep(void)
{
	asm volatile ("wait");
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
extern void asm_delay_loop(uint32_t);
extern void userspace_asm(uintptr_t, uintptr_t, uintptr_t);

extern ipl_t interrupts_disable(void);
extern ipl_t interrupts_enable(void);
extern void interrupts_restore(ipl_t);
extern ipl_t interrupts_read(void);
extern bool interrupts_disabled(void);

#endif

/** @}
 */
