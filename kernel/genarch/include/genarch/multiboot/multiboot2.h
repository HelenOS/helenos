/*
 * Copyright (c) 2011 Martin Decky
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

#ifndef KERN_MULTIBOOT2_H_
#define KERN_MULTIBOOT2_H_

#define MULTIBOOT2_HEADER_MAGIC      0xe85250d6
#define MULTIBOOT2_HEADER_ARCH_I386  0

#define MULTIBOOT2_LOADER_MAGIC  0x36d76289

#define MULTIBOOT2_FLAGS_REQUIRED  0
#define MULTIBOOT2_FLAGS_CONSOLE   0x03

#define MULTIBOOT2_TAG_TERMINATOR     0
#define MULTIBOOT2_TAG_INFO_REQ       1
#define MULTIBOOT2_TAG_ADDRESS        2
#define MULTIBOOT2_TAG_ENTRY_ADDRESS  3
#define MULTIBOOT2_TAG_FLAGS          4
#define MULTIBOOT2_TAG_FRAMEBUFFER    5
#define MULTIBOOT2_TAG_MODULE_ALIGN   6

#define MULTIBOOT2_TAG_CMDLINE 1
#define MULTIBOOT2_TAG_MODULE  3
#define MULTIBOOT2_TAG_MEMMAP  6
#define MULTIBOOT2_TAG_FBINFO  8

#define MULTIBOOT2_VISUAL_INDEXED  0
#define MULTIBOOT2_VISUAL_RGB      1
#define MULTIBOOT2_VISUAL_EGA      2

#ifndef __ASSEMBLER__

#include <typedefs.h>
#include <arch/boot/memmap.h>

/** Multiboot2 32-bit address. */
typedef uint32_t mb2addr_t;

/** Multiboot2 information structure */
typedef struct {
	uint32_t size;
	uint32_t reserved;
} __attribute__((packed)) multiboot2_info_t;

/** Multiboot2 cmdline structure */
typedef struct {
	char string[0];
} __attribute__((packed)) multiboot2_cmdline_t;

/** Multiboot2 modules structure */
typedef struct {
	mb2addr_t start;
	mb2addr_t end;
	char string[];
} __attribute__((packed)) multiboot2_module_t;

/** Multiboot2 memmap structure */
typedef struct {
	uint32_t entry_size;
	uint32_t entry_version;
} __attribute__((packed)) multiboot2_memmap_t;

/** Multiboot2 memmap entry structure */
typedef struct {
	uint64_t base_address;
	uint64_t size;
	uint32_t type;
	uint32_t reserved;
} __attribute__((packed)) multiboot2_memmap_entry_t;

/** Multiboot2 palette structure */
typedef struct {
	uint8_t red;
	uint8_t green;
	uint8_t blue;
} __attribute__((packed)) multiboot2_colorinfo_palette_t;

/** Multiboot2 indexed color information structure */
typedef struct {
	uint32_t colors;
	multiboot2_colorinfo_palette_t palette[];
} __attribute__((packed)) multiboot2_colorinfo_indexed_t;

/** Multiboot2 RGB color information structure */
typedef struct {
	uint8_t red_pos;
	uint8_t red_size;
	uint8_t green_pos;
	uint8_t green_size;
	uint8_t blue_pos;
	uint8_t blue_size;
} __attribute__((packed)) multiboot2_colorinfo_rgb_t;

/** Multiboot2 framebuffer information structure */
typedef struct {
	uint64_t addr;
	uint32_t scanline;
	uint32_t width;
	uint32_t height;
	uint8_t bpp;
	uint8_t visual;
	uint8_t reserved;
	union {
		multiboot2_colorinfo_indexed_t indexed;
		multiboot2_colorinfo_rgb_t rgb;
	};
} __attribute__((packed)) multiboot2_fbinfo_t;

/** Generic multiboot2 tag */
typedef struct {
	uint32_t type;
	uint32_t size;
	union {
		multiboot2_cmdline_t cmdline;
		multiboot2_module_t module;
		multiboot2_memmap_t memmap;
		multiboot2_fbinfo_t fbinfo;
	};
} __attribute__((packed)) multiboot2_tag_t;

extern void multiboot2_info_parse(uint32_t, const multiboot2_info_t *);

#endif /* __ASSEMBLER__ */

#endif

/** @}
 */
