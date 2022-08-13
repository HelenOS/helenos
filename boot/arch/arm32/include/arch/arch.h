/*
 * SPDX-FileCopyrightText: 2010 Martin Decky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef BOOT_arm32_ARCH_H
#define BOOT_arm32_ARCH_H

#define PAGE_WIDTH  12
#define PAGE_SIZE   (1 << PAGE_WIDTH)

#define PTL0_ENTRIES     4096
#define PTL0_ENTRY_SIZE  4

/*
 * Address where the boot stage image starts (beginning of usable physical
 * memory).
 */
#ifdef MACHINE_gta02
#define BOOT_BASE	0x30008000
#elif defined MACHINE_beagleboardxm
#define BOOT_BASE	0x80000000
#elif defined MACHINE_beaglebone
#define BOOT_BASE       0x80000000
#elif defined MACHINE_raspberrypi
#define BOOT_BASE	0x00008000
#else
#define BOOT_BASE	0x00000000
#endif

#define BOOT_OFFSET	(BOOT_BASE + 0xa00000)

#ifdef MACHINE_beagleboardxm
#define PA_OFFSET 0
#elif defined MACHINE_beaglebone
#define PA_OFFSET 0
#else
#define PA_OFFSET 0x80000000
#endif

#ifndef __ASSEMBLER__
#define PA2KA(addr)  (((uintptr_t) (addr)) + PA_OFFSET)
#else
#define PA2KA(addr)  ((addr) + PA_OFFSET)
#endif

#endif

/** @}
 */
