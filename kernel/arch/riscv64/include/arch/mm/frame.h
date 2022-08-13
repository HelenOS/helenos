/*
 * SPDX-FileCopyrightText: 2016 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_riscv64_mm
 * @{
 */
/** @file
 */

#ifndef KERN_riscv64_FRAME_H_
#define KERN_riscv64_FRAME_H_

#define FRAME_WIDTH  12  /* 4 KiB */
#define FRAME_SIZE   (1 << FRAME_WIDTH)

#define FRAME_LOWPRIO  0

#ifndef __ASSEMBLER__

#include <arch/boot/boot.h>

extern uintptr_t physmem_start;
extern uintptr_t htif_frame;
extern uintptr_t pt_frame;
extern memmap_t memmap;

extern void frame_low_arch_init(void);
extern void frame_high_arch_init(void);
extern void physmem_print(void);

#endif /* __ASSEMBLER__ */

#endif

/** @}
 */
