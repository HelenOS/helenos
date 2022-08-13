/*
 * SPDX-FileCopyrightText: 2007 Pavel Jancik, Michal Kebrt
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_arm32_mm
 * @{
 */
/** @file
 *  @brief Frame related declarations.
 */

#ifndef KERN_arm32_FRAME_H_
#define KERN_arm32_FRAME_H_

#define FRAME_WIDTH  12  /* 4KB frames */
#define FRAME_SIZE   (1 << FRAME_WIDTH)

#define FRAME_LOWPRIO  0

#ifndef __ASSEMBLER__

#define BOOT_PAGE_TABLE_SIZE     0x4000

#ifdef MACHINE_gta02

#define PHYSMEM_START_ADDR       0x30008000
#define BOOT_PAGE_TABLE_ADDRESS  0x30010000

#elif defined MACHINE_beagleboardxm

#define PHYSMEM_START_ADDR       0x80000000
#define BOOT_PAGE_TABLE_ADDRESS  0x80008000

#elif defined MACHINE_beaglebone

#define PHYSMEM_START_ADDR       0x80000000
#define BOOT_PAGE_TABLE_ADDRESS  0x80008000

#elif defined MACHINE_raspberrypi

#define PHYSMEM_START_ADDR       0x00000000
#define BOOT_PAGE_TABLE_ADDRESS  0x00010000

#else

#define PHYSMEM_START_ADDR       0x00000000
#define BOOT_PAGE_TABLE_ADDRESS  0x00008000

#endif

#define BOOT_PAGE_TABLE_START_FRAME     (BOOT_PAGE_TABLE_ADDRESS >> FRAME_WIDTH)
#define BOOT_PAGE_TABLE_SIZE_IN_FRAMES  (BOOT_PAGE_TABLE_SIZE >> FRAME_WIDTH)

extern void frame_low_arch_init(void);
extern void frame_high_arch_init(void);
extern void boot_page_table_free(void);
#define physmem_print()

#endif /* __ASSEMBLER__ */

#endif

/** @}
 */
