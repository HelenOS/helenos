/*
 * SPDX-FileCopyrightText: 2014 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup sysinst
 * @{
 */
/**
 * @file
 * @brief
 */

#ifndef GRUB_H
#define GRUB_H

enum {
	grub_boot_machine_kernel_sector = 0x5c,
	grub_boot_machine_boot_drive = 0x64
};

enum {
	/* 8086 segment (16*pa) where to load GRUB core image */
	grub_boot_i386_pc_kernel_seg = 0x800
};

typedef struct {
	uint64_t start;
	uint16_t len;
	uint16_t segment;
} __attribute__((packed)) grub_boot_blocklist_t;

#endif

/** @}
 */
