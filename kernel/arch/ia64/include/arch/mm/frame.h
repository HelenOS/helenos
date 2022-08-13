/*
 * SPDX-FileCopyrightText: 2005 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_ia64_mm
 * @{
 */
/** @file
 */

#ifndef KERN_ia64_FRAME_H_
#define KERN_ia64_FRAME_H_

#define FRAME_WIDTH  14  /* 16K */
#define FRAME_SIZE   (1 << FRAME_WIDTH)

#define FRAME_LOWPRIO  0

#ifndef __ASSEMBLER__

#include <typedefs.h>

extern uintptr_t end_of_identity;

extern void frame_low_arch_init(void);
extern void frame_high_arch_init(void);
#define physmem_print()

#endif /* __ASSEMBLER__ */

#endif

/** @}
 */
