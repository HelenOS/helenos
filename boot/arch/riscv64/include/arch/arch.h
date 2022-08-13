/*
 * SPDX-FileCopyrightText: 2016 Martin Decky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef BOOT_riscv64_ARCH_H_
#define BOOT_riscv64_ARCH_H_

#define PAGE_WIDTH  12
#define PAGE_SIZE   (1 << PAGE_WIDTH)

#define BOOT_STACK_SIZE  PAGE_SIZE

#define PHYSMEM_START  0x40000000
#define PHYSMEM_SIZE   1073741824
#define BOOT_OFFSET    0x48000000

#define DEFAULT_MTVEC      0x00000100
#define TRAP_VECTOR_RESET  0x0100

#endif
