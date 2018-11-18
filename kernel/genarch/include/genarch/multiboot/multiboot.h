/*
 * Copyright (c) 2009 Jiri Svoboda
 * All rights reserved.
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

/** @addtogroup kernel_genarch
 * @{
 */
/** @file
 */

#ifndef KERN_MULTIBOOT_H_
#define KERN_MULTIBOOT_H_

#include <genarch/multiboot/multiboot_memmap_struct.h>
#include <genarch/multiboot/multiboot_info_struct.h>

#define MULTIBOOT_HEADER_MAGIC  0x1badb002
#define MULTIBOOT_HEADER_FLAGS  0x00010003

#define MULTIBOOT_LOADER_MAGIC  0x2badb002

#define MULTIBOOT_INFO_FLAGS_MEM	0x01
#define MULTIBOOT_INFO_FLAGS_BOOT	0x02
#define MULTIBOOT_INFO_FLAGS_CMDLINE	0x04
#define MULTIBOOT_INFO_FLAGS_MODS	0x08
#define MULTIBOOT_INFO_FLAGS_SYMS1	0x10
#define MULTIBOOT_INFO_FLAGS_SYMS2	0x20
#define MULTIBOOT_INFO_FLAGS_MMAP	0x40

#ifndef __ASSEMBLER__

#include <typedefs.h>
#include <arch/boot/memmap.h>

/** Convert 32-bit multiboot address to a pointer. */
#define MULTIBOOT_PTR(mba)  ((void *) (uintptr_t) (mba))

/** Multiboot 32-bit address. */
typedef uint32_t mbaddr_t;

/** Multiboot module structure */
typedef struct {
	mbaddr_t start;
	mbaddr_t end;
	mbaddr_t string;
	uint32_t reserved;
} __attribute__((packed)) multiboot_module_t;

extern void multiboot_cmdline(const char *);

extern void multiboot_extract_command(char *, size_t, const char *);
extern void multiboot_extract_argument(char *, size_t, const char *);
extern void multiboot_info_parse(uint32_t, const multiboot_info_t *);

#endif /* __ASSEMBLER__ */

#endif

/** @}
 */
