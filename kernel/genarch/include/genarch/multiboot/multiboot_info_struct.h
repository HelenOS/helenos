/*
 * Copyright (c) 2016 Jakub Jermar
 * All rights preserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * - Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 * - Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the distribution.
 * - The name of the author may not be used to endorse or promote products
 *   derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
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
