/*
 * SPDX-FileCopyrightText: 2005 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_sparc64
 * @{
 */
/** @file
 */

#ifndef KERN_sparc64_sun4u_CPU_H_
#define KERN_sparc64_sun4u_CPU_H_

#define MANUF_FUJITSU     0x04
#define MANUF_ULTRASPARC  0x17  /**< UltraSPARC I, UltraSPARC II */
#define MANUF_SUN         0x3e

#define IMPL_ULTRASPARCI         0x10
#define IMPL_ULTRASPARCII        0x11
#define IMPL_ULTRASPARCII_I      0x12
#define IMPL_ULTRASPARCII_E      0x13
#define IMPL_ULTRASPARCIII       0x14
#define IMPL_ULTRASPARCIII_PLUS  0x15
#define IMPL_ULTRASPARCIII_I     0x16
#define IMPL_ULTRASPARCIV        0x18
#define IMPL_ULTRASPARCIV_PLUS   0x19

#define IMPL_SPARC64V  0x5

#ifndef __ASSEMBLER__

#include <arch/register.h>
#include <arch/regdef.h>
#include <arch/asm.h>
#include <arch/arch.h>
#include <stdint.h>
#include <trace.h>

typedef struct {
	/** Processor ID as read from UPA_CONFIG/FIREPLANE_CONFIG. */
	uint32_t mid;
	ver_reg_t ver;
	/** Processor frequency in Hz. */
	uint32_t clock_frequency;
	/** Next clock interrupt should be generated when the TICK register
	 * matches this value.
	 */
	uint64_t next_tick_cmpr;
} cpu_arch_t;

/** Read the module ID (agent ID/CPUID) of the current CPU.
 *
 */
_NO_TRACE static inline uint32_t read_mid(void)
{
	uint64_t icbus_config = asi_u64_read(ASI_ICBUS_CONFIG, 0);
	icbus_config = icbus_config >> ICBUS_CONFIG_MID_SHIFT;

#if defined (US)
	return icbus_config & 0x1f;
#elif defined (US3)
	if (((ver_reg_t) ver_read()).impl == IMPL_ULTRASPARCIII_I)
		return icbus_config & 0x1f;
	else
		return icbus_config & 0x3ff;
#endif
}

#endif

#endif

/** @}
 */
