/*
 * SPDX-FileCopyrightText: 2005 Martin Decky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_ppc32_mm
 * @{
 */
/** @file
 */

#ifndef KERN_ppc32_FRAME_H_
#define KERN_ppc32_FRAME_H_

#define FRAME_WIDTH  12  /* 4K */
#define FRAME_SIZE   (1 << FRAME_WIDTH)

#define FRAME_LOWPRIO  0

#ifndef __ASSEMBLER__

#include <stdint.h>
#include <trace.h>

_NO_TRACE static inline uint32_t physmem_top(void)
{
	uint32_t physmem;

	asm volatile (
	    "mfsprg3 %[physmem]\n"
	    : [physmem] "=r" (physmem)
	);

	return physmem;
}

extern void frame_low_arch_init(void);
extern void frame_high_arch_init(void);
extern void physmem_print(void);

#endif /* __ASSEMBLER__ */

#endif

/** @}
 */
