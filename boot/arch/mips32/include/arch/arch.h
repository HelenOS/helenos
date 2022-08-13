/*
 * SPDX-FileCopyrightText: 2006 Martin Decky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef BOOT_mips32_ARCH_H_
#define BOOT_mips32_ARCH_H_

#define PAGE_WIDTH  14
#define PAGE_SIZE   (1 << PAGE_WIDTH)

#if defined(MACHINE_msim)
#define CPUMAP_OFFSET    0x00001000
#define STACK_OFFSET     0x00002000
#define BOOTINFO_OFFSET  0x00003000
#define BOOT_OFFSET      0x00100000
#define LOADER_OFFSET    0x1fc00000

#define MSIM_VIDEORAM_ADDRESS  0xb0000000
#define MSIM_DORDER_ADDRESS    0xb0000100
#endif

#if defined(MACHINE_lmalta) || defined(MACHINE_bmalta)
#define CPUMAP_OFFSET    0x00100000
#define STACK_OFFSET     0x00101000
#define BOOTINFO_OFFSET  0x00102000
#define BOOT_OFFSET      0x00200000
#define LOADER_OFFSET    0x00103000

#define YAMON_SUBR_BASE		PA2KA(0x1fc00500)
#define YAMON_SUBR_PRINT_COUNT	(YAMON_SUBR_BASE + 0x4)
#endif

#ifndef __ASSEMBLER__
#define PA2KA(addr)    (((uintptr_t) (addr)) + 0x80000000)
#define PA2KSEG(addr)  (((uintptr_t) (addr)) + 0xa0000000)
#define KA2PA(addr)    (((uintptr_t) (addr)) - 0x80000000)
#define KSEG2PA(addr)  (((uintptr_t) (addr)) - 0xa0000000)
#else
#define PA2KA(addr)    ((addr) + 0x80000000)
#define KSEG2PA(addr)  ((addr) - 0xa0000000)
#endif

#endif
