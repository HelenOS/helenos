/*
 * SPDX-FileCopyrightText: 2016 Martin Decky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_riscv64
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

_NO_TRACE static inline ipl_t interrupts_enable(void)
{
	ipl_t ipl;

	asm volatile (
	    "csrrsi %[ipl], sstatus, " STRING(SSTATUS_SIE_MASK) "\n"
	    : [ipl] "=r" (ipl)
	);

	return ipl;
}

_NO_TRACE static inline ipl_t interrupts_disable(void)
{
	ipl_t ipl;

	asm volatile (
	    "csrrci %[ipl], sstatus, " STRING(SSTATUS_SIE_MASK) "\n"
	    : [ipl] "=r" (ipl)
	);

	return ipl;
}

_NO_TRACE static inline void interrupts_restore(ipl_t ipl)
{
	if ((ipl & SSTATUS_SIE_MASK) == SSTATUS_SIE_MASK)
		interrupts_enable();
	else
		interrupts_disable();
}

_NO_TRACE static inline ipl_t interrupts_read(void)
{
	ipl_t ipl;

	asm volatile (
	    "csrr %[ipl], sstatus\n"
	    : [ipl] "=r" (ipl)
	);

	return ipl;
}

_NO_TRACE static inline bool interrupts_disabled(void)
{
	return ((interrupts_read() & SSTATUS_SIE_MASK) == 0);
}

_NO_TRACE static inline void cpu_sleep(void)
{
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
extern void userspace_asm(uintptr_t uspace_uarg, uintptr_t stack,
    uintptr_t entry);

#endif

/** @}
 */
