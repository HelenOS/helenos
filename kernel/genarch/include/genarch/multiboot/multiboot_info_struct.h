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

#define MULTIBOOT_INFO_OFFSET_FLAGS               0x00
#define MULTIBOOT_INFO_OFFSET_MEM_LOWER           0x04
#define MULTIBOOT_INFO_OFFSET_MEM_UPPER           0x08
#define MULTIBOOT_INFO_OFFSET_BOOT_DEVICE         0x0c
#define MULTIBOOT_INFO_OFFSET_CMD_LINE            0x10
#define MULTIBOOT_INFO_OFFSET_MODS_COUNT          0x14
#define MULTIBOOT_INFO_OFFSET_MODS_ADDR           0x18
#define MULTIBOOT_INFO_OFFSET_SYMS                0x1c
#define MULTIBOOT_INFO_OFFSET_MMAP_LENGTH         0x2c
#define MULTIBOOT_INFO_OFFSET_MMAP_ADDR           0x30
#define MULTIBOOT_INFO_OFFSET_DRIVES_LENGTH       0x34
#define MULTIBOOT_INFO_OFFSET_DRIVES_ADDR         0x38
#define MULTIBOOT_INFO_OFFSET_CONFIG_TABLE        0x3c
#define MULTIBOOT_INFO_OFFSET_BOOT_LOADER_NAME    0x40
#define MULTIBOOT_INFO_OFFSET_APM_TABLE           0x44
#define MULTIBOOT_INFO_OFFSET_VBE_CONTROL_INFO    0x48
#define MULTIBOOT_INFO_OFFSET_VBE_MODE_INFO       0x4c
#define MULTIBOOT_INFO_OFFSET_VBE_MODE            0x50
#define MULTIBOOT_INFO_OFFSET_VBE_INTERFACE_SEG   0x52
#define MULTIBOOT_INFO_OFFSET_VBE_INTERFACE_OFF   0x54
#define MULTIBOOT_INFO_OFFSET_VBE_INTERFACE_LEN   0x56
#define MULTIBOOT_INFO_OFFSET_FRAMEBUFFER_ADDR    0x58
#define MULTIBOOT_INFO_OFFSET_FRAMEBUFFER_PITCH   0x60
#define MULTIBOOT_INFO_OFFSET_FRAMEBUFFER_WIDTH   0x64
#define MULTIBOOT_INFO_OFFSET_FRAMEBUFFER_HEIGHT  0x68
#define MULTIBOOT_INFO_OFFSET_FRAMEBUFFER_BPP     0x6c
#define MULTIBOOT_INFO_OFFSET_FRAMEBUFFER_TYPE    0x6d

#define MULTIBOOT_INFO_OFFSET_FRAMEBUFFER_PALETTE_ADDR          0x6e
#define MULTIBOOT_INFO_OFFSET_FRAMEBUFFER_PALETTE_NUM_COLORS    0x72

#define MULTIBOOT_INFO_OFFSET_FRAMEBUFFER_RED_FIELD_POSITION    0x6e
#define MULTIBOOT_INFO_OFFSET_FRAMEBUFFER_RED_MASK_SIZE         0x6f
#define MULTIBOOT_INFO_OFFSET_FRAMEBUFFER_GREEN_FIELD_POSITION  0x70
#define MULTIBOOT_INFO_OFFSET_FRAMEBUFFER_GREEN_MASK_SIZE       0x71
#define MULTIBOOT_INFO_OFFSET_FRAMEBUFFER_BLUE_FIELD_POSITION   0x72
#define MULTIBOOT_INFO_OFFSET_FRAMEBUFFER_BLUE_MASK_SIZE        0x73

#define MULTIBOOT_INFO_SIZE                0x76

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
	uint32_t drives_length;
	uint32_t drives_addr;
	uint32_t config_table;
	uint32_t boot_loader_name;
	uint32_t apm_table;
	uint32_t vbe_control_info;
	uint32_t vbe_mode_info;
	uint16_t vbe_mode;
	uint16_t vbe_interface_seg;
	uint16_t vbe_interface_off;
	uint16_t vbe_interface_len;
	uint64_t framebuffer_addr;
	uint32_t framebuffer_pitch;
	uint32_t framebuffer_width;
	uint32_t framebuffer_height;
	uint8_t framebuffer_bpp;
	uint8_t framebuffer_type;
	union {
		struct {
			uint32_t framebuffer_palette_addr;
			uint32_t framebuffer_palette_num_colors;
		} __attribute__((packed));
		struct {
			uint8_t framebuffer_red_field_position;
			uint8_t framebuffer_red_mask_size;
			uint8_t framebuffer_green_field_position;
			uint8_t framebuffer_green_mask_size;
			uint8_t framebuffer_blue_field_position;
			uint8_t framebuffer_blue_mask_size;
		} __attribute__((packed));
	} __attribute__((packed));
} __attribute__((packed)) multiboot_info_t;

#endif
#endif
