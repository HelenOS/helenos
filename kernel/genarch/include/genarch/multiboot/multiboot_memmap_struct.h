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
