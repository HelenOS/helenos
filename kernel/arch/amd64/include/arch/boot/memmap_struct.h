/*
 * SPDX-FileCopyrightText: 2016 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef KERN_ARCH_BOOT_MEMMAP_STRUCT_H_
#define KERN_ARCH_BOOT_MEMMAP_STRUCT_H_

#define E820MEMMAP_OFFSET_BASE_ADDRESS  0x00
#define E820MEMMAP_OFFSET_SIZE          0x08
#define E820MEMMAP_OFFSET_TYPE          0x10
#define E820MEMMAP_SIZE                 0x14

#ifndef __ASSEMBLER__

#include <stdint.h>

typedef struct e820memmap {
	uint64_t base_address;
	uint64_t size;
	uint32_t type;
} __attribute__((packed)) e820memmap_t;

#endif
#endif
