/*
 * SPDX-FileCopyrightText: 2005 Ondrej Palkovsky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_amd64_mm
 * @{
 */
/** @file
 */

#ifndef KERN_amd64_FRAME_H_
#define KERN_amd64_FRAME_H_

#define FRAME_WIDTH  12  /* 4K */
#define FRAME_SIZE   (1 << FRAME_WIDTH)

#define FRAME_LOWPRIO  0x1000

#ifndef __ASSEMBLER__

extern void frame_low_arch_init(void);
extern void frame_high_arch_init(void);
extern void physmem_print(void);

#endif /* __ASSEMBLER__ */

#endif

/** @}
 */
