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

#ifndef KERN_sparc64_CPU_H_
#define KERN_sparc64_CPU_H_

#define MANUF_FUJITSU		0x04
#define MANUF_ULTRASPARC	0x17	/**< UltraSPARC I, UltraSPARC II */
#define MANUF_SUN		0x3e

#define IMPL_ULTRASPARCI	0x10
#define IMPL_ULTRASPARCII	0x11
#define IMPL_ULTRASPARCII_I	0x12
#define IMPL_ULTRASPARCII_E	0x13
#define IMPL_ULTRASPARCIII	0x14
#define IMPL_ULTRASPARCIII_PLUS	0x15
#define IMPL_ULTRASPARCIII_I	0x16
#define IMPL_ULTRASPARCIV	0x18
#define IMPL_ULTRASPARCIV_PLUS	0x19

#define IMPL_SPARC64V		0x5

#ifndef __ASSEMBLER__

#include <arch/register.h>
#include <arch/regdef.h>
#include <arch/asm.h>

#if defined (SUN4U)
#include <arch/sun4u/cpu.h>
#elif defined (SUN4V)
#include <arch/sun4v/cpu.h>
#endif

#endif

#endif

/** @}
 */
