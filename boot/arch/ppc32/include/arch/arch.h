/*
 * SPDX-FileCopyrightText: 2006 Martin Decky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef BOOT_ppc32_ARCH_H_
#define BOOT_ppc32_ARCH_H_

#define PAGE_WIDTH  12
#define PAGE_SIZE   (1 << PAGE_WIDTH)

#define BOOT_OFFSET  0x8000

#define LOADER_ADDRESS  0x08000000

#ifndef __ASSEMBLER__
#define PA2KA(addr)  (((uintptr_t) (addr)) + 0x80000000)
#else
#define PA2KA(addr)  ((addr) + 0x80000000)
#endif

#endif
