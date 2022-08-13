/*
 * SPDX-FileCopyrightText: 2005 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_sparc64_mm
 * @{
 */
/** @file
 */

#ifndef KERN_sparc64_FRAME_H_
#define KERN_sparc64_FRAME_H_

#if defined (SUN4U)

#include <arch/mm/sun4u/frame.h>

#elif defined (SUN4V)

#include <arch/mm/sun4v/frame.h>

#endif

#ifndef __ASSEMBLER__

#include <typedefs.h>

extern uintptr_t end_of_identity;

extern void frame_low_arch_init(void);
extern void frame_high_arch_init(void);
#define physmem_print()

#endif

#endif

/** @}
 */
