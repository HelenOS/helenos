/*
 * SPDX-FileCopyrightText: 2009 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
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
