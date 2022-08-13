/*
 * SPDX-FileCopyrightText: 2015 Petr Pavlu
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_arm64_mm
 * @{
 */
/** @file
 * @brief Frame related declarations.
 */

#ifndef KERN_arm64_FRAME_H_
#define KERN_arm64_FRAME_H_

#include <arch/boot/boot.h>

#define FRAME_WIDTH  12  /* 4KB frames */
#define FRAME_SIZE   (1 << FRAME_WIDTH)

#define FRAME_LOWPRIO  0

#ifndef __ASSEMBLER__

extern void frame_low_arch_init(void);
extern void frame_high_arch_init(void);
extern void physmem_print(void);

extern memmap_t memmap;

#endif /* __ASSEMBLER__ */

#endif

/** @}
 */
