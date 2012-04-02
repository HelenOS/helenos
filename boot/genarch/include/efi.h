/*
 * Copyright (c) 2011 Jakub Jermar
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

#ifndef BOOT_EFI_H_
#define BOOT_EFI_H_

#include <arch/types.h>

typedef struct {
	uint64_t signature;
	uint32_t revision;
	uint32_t header_size;
	uint32_t crc32;
	uint32_t reserved;
} efi_table_header_t;

#define SAL_SYSTEM_TABLE_GUID \
	{ \
		{ \
			0x32, 0x2d, 0x9d, 0xeb, 0x88, 0x2d, 0xd3, 0x11, \
			0x9a, 0x16, 0x00, 0x90, 0x27, 0x3f, 0xc1, 0x4d \
		} \
	}

typedef union {
	uint8_t bytes[16];
	struct {
		uint64_t low;
		uint64_t high;
	};
} efi_guid_t;

typedef struct {
	efi_guid_t guid;
	void *table;
} efi_configuration_table_t;

typedef struct {
	efi_table_header_t hdr;
	char *fw_vendor;
	uint32_t fw_revision;
	void *cons_in_handle;
	void *cons_in;
	void *cons_out_handle;
	void *cons_out;
	void *cons_err_handle;
	void *cons_err;
	void *runtime_services;
	void *boot_services;
	sysarg_t conf_table_entries;
	efi_configuration_table_t *conf_table;
} efi_system_table_t;

typedef enum {
	EFI_RESERVED,
	EFI_LOADER_CODE,
	EFI_LOADER_DATA,
	EFI_BOOT_SERVICES_CODE,
	EFI_BOOT_SERVICES_DATA,
	EFI_RUNTIME_SERVICES_CODE,
	EFI_RUNTIME_SERVICES_DATA,
	EFI_CONVENTIONAL_MEMORY,
	EFI_UNUSABLE_MEMORY,
	EFI_ACPI_RECLAIM_MEMORY,
	EFI_ACPI_MEMORY_NVS,
	EFI_MEMORY_MAPPED_IO,
	EFI_MEMORY_MAPPED_IO_PORT_SPACE,
	EFI_PAL_CODE
} efi_memory_type_t;

typedef struct {
	uint32_t type;
	uint64_t phys_start;
	uint64_t virt_start;
	uint64_t pages;
	uint64_t attribute;
} efi_v1_memdesc_t;

#define EFI_PAGE_SIZE	4096

extern void *efi_vendor_table_find(efi_system_table_t *, efi_guid_t);

#endif
