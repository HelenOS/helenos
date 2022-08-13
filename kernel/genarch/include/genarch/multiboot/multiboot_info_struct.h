/*
 * SPDX-FileCopyrightText: 2016 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef KERN_MULTIBOOT_INFO_STRUCT_H_
#define KERN_MULTIBOOT_INFO_STRUCT_H_

#define MULTIBOOT_INFO_OFFSET_FLAGS        0x00
#define MULTIBOOT_INFO_OFFSET_MEM_LOWER    0x04
#define MULTIBOOT_INFO_OFFSET_MEM_UPPER    0x08
#define MULTIBOOT_INFO_OFFSET_BOOT_DEVICE  0x0c
#define MULTIBOOT_INFO_OFFSET_CMD_LINE     0x10
#define MULTIBOOT_INFO_OFFSET_MODS_COUNT   0x14
#define MULTIBOOT_INFO_OFFSET_MODS_ADDR    0x18
#define MULTIBOOT_INFO_OFFSET_SYMS         0x1c
#define MULTIBOOT_INFO_OFFSET_MMAP_LENGTH  0x2c
#define MULTIBOOT_INFO_OFFSET_MMAP_ADDR    0x30
#define MULTIBOOT_INFO_SIZE                0x34

#ifndef __ASSEMBLER__

#include <stdint.h>

typedef struct multiboot_info {
	uint32_t flags;
	uint32_t mem_lower;
	uint32_t mem_upper;
	uint32_t boot_device;
	uint32_t cmd_line;
	uint32_t mods_count;
	uint32_t mods_addr;
	uint32_t syms[4];
	uint32_t mmap_length;
	uint32_t mmap_addr;
} __attribute__((packed)) multiboot_info_t;

#endif
#endif
