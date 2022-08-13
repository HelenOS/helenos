/*
 * SPDX-FileCopyrightText: 2016 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef KERN_MULTIBOOT_MEMMAP_STRUCT_H_
#define KERN_MULTIBOOT_MEMMAP_STRUCT_H_

#include <arch/boot/memmap_struct.h>

#define MULTIBOOT_MEMMAP_OFFSET_SIZE     0x00
#define MULTIBOOT_MEMMAP_OFFSET_MM_INFO  0x04
#define MULTIBOOT_MEMMAP_SIZE            0x18

#define MULTIBOOT_MEMMAP_SIZE_SIZE       0x04

#ifndef __ASSEMBLER__

#include <stdint.h>

typedef struct multiboot_memmap {
	uint32_t size;
	e820memmap_t mm_info;
} __attribute__((packed)) multiboot_memmap_t;

#endif
#endif
