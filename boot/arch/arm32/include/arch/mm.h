/*
 * SPDX-FileCopyrightText: 2007 Pavel Jancik
 * SPDX-FileCopyrightText: 2007 Michal Kebrt
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup boot_arm32
 * @{
 */
/** @file
 * @brief Memory management used while booting the kernel.
 *
 * So called "section" paging is used while booting the kernel. The term
 * "section" comes from the ARM architecture specification and stands for the
 * following: one-level paging, 1MB sized pages, 4096 entries in the page
 * table.
 */

#ifndef BOOT_arm32__MM_H
#define BOOT_arm32__MM_H

/** Describe "section" page table entry (one-level paging with 1 MB sized pages). */
#define PTE_DESCRIPTOR_SECTION  0x02
/** Shift of memory address in section descriptor */
#define PTE_SECTION_SHIFT  20

/** Page table access rights: user - no access, kernel - read/write. */
#define PTE_AP_USER_NO_KERNEL_RW  0x01

/** Start of memory mapped I/O area for GTA02 */
#define GTA02_IOMEM_START  0x48000000
/** End of memory mapped I/O area for GTA02 */
#define GTA02_IOMEM_END  0x60000000

/** Start of ram memory on BBxM */
#define BBXM_RAM_START   0x80000000
/** Start of ram memory on BBxM */
#define BBXM_RAM_END   0xc0000000

/** Start of ram memory on AM335x */
#define AM335x_RAM_START   0x80000000
/** End of ram memory on AM335x */
#define AM335x_RAM_END     0xC0000000

/** Start of ram memory on BCM2835 */
#define BCM2835_RAM_START   0
/** End of ram memory on BCM2835 */
#define BCM2835_RAM_END     0x20000000

/** Page table level 0 entry
 *
 * "section" format is used (one-level paging, 1 MB sized pages).
 * Used only while booting the kernel.
 */
typedef struct {
	unsigned int descriptor_type : 2;
	unsigned int bufferable : 1;
	unsigned int cacheable : 1;
	unsigned int xn : 1;
	unsigned int domain : 4;
	unsigned int should_be_zero_1 : 1;
	unsigned int access_permission_0 : 2;
	unsigned int tex : 3;
	unsigned int access_permission_1 : 1;
	unsigned int shareable : 1;
	unsigned int non_global : 1;
	unsigned int should_be_zero_2 : 1;
	unsigned int non_secure : 1;
	unsigned int section_base_addr : 12;
} __attribute__((packed)) pte_level0_section_t;

extern void mmu_start(void);

#endif

/** @}
 */
