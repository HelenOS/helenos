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

#define EFI_SUCCESS  0
#define EFI_ERROR(code) (((sysarg_t) 1 << (sizeof(sysarg_t) * 8 - 1)) | (code))
#define EFI_LOAD_ERROR  EFI_ERROR(1)
#define EFI_UNSUPPORTED  EFI_ERROR(3)
#define EFI_BUFFER_TOO_SMALL  EFI_ERROR(5)

typedef uint64_t efi_status_t;

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

typedef enum {
	EFI_ALLOCATE_ANY_PAGES,
	EFI_ALLOCATE_MAX_ADDRESS,
	EFI_ALLOCATE_ADDRESS
} efi_allocate_type_t;

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
	EFI_PAL_CODE,
	EFI_PERSISTENT_MEMORY
} efi_memory_type_t;

#define EFI_MEMORY_UC             UINT64_C(0x0000000000000001)
#define EFI_MEMORY_WC             UINT64_C(0x0000000000000002)
#define EFI_MEMORY_WT             UINT64_C(0x0000000000000004)
#define EFI_MEMORY_WB             UINT64_C(0x0000000000000008)
#define EFI_MEMORY_UCE            UINT64_C(0x0000000000000010)
#define EFI_MEMORY_WP             UINT64_C(0x0000000000001000)
#define EFI_MEMORY_RP             UINT64_C(0x0000000000002000)
#define EFI_MEMORY_XP             UINT64_C(0x0000000000004000)
#define EFI_MEMORY_NV             UINT64_C(0x0000000000008000)
#define EFI_MEMORY_MORE_RELIABLE  UINT64_C(0x0000000000010000)
#define EFI_MEMORY_RO             UINT64_C(0x0000000000020000)
#define EFI_MEMORY_RUNTIME        UINT64_C(0x8000000000000000)

typedef struct {
	uint32_t type;
	uint64_t phys_start;
	uint64_t virt_start;
	uint64_t pages;
	uint64_t attribute;
} efi_v1_memdesc_t;

typedef struct {
	efi_guid_t guid;
	void *table;
} efi_configuration_table_t;

typedef struct efi_simple_text_output_protocol {
	void *reset;
	efi_status_t (*output_string)(struct efi_simple_text_output_protocol *,
	    int16_t *);
	void *test_string;
	void *query_mode;
	void *set_mode;
	void *set_attribute;
	void *clear_screen;
	void *set_cursor_position;
	void *enable_cursor;
	void *mode;
} efi_simple_text_output_protocol_t;

typedef struct {
	efi_table_header_t hdr;
	void *raise_TPL;
	void *restore_TPL;
	efi_status_t (*allocate_pages)(efi_allocate_type_t, efi_memory_type_t,
	    sysarg_t, uint64_t *);
	efi_status_t (*free_pages)(uint64_t, sysarg_t);
	efi_status_t (*get_memory_map)(sysarg_t *, efi_v1_memdesc_t *,
	    sysarg_t *, sysarg_t *, uint32_t *);
	efi_status_t (*allocate_pool)(efi_memory_type_t, sysarg_t, void **);
	efi_status_t (*free_pool)(void *);
	void *create_event;
	void *set_timer;
	void *wait_for_event;
	void *signal_event;
	void *close_event;
	void *check_event;
	void *install_protocol_interface;
	void *reinstall_protocol_interface;
	void *uninstall_protocol_interface;
	void *handle_protocol;
	void *reserved;
	void *register_protocol_notify;
	void *locate_handle;
	void *locate_device_path;
	void *install_configuration_table;
	void *load_image;
	void *start_image;
	void *exit;
	void *unload_image;
	efi_status_t (*exit_boot_services)(void *, sysarg_t);
	void *get_next_monotonic_count;
	void *stall;
	void *set_watchdog_timer;
	void *connect_controller;
	void *disconnect_controller;
	void *open_protocol;
	void *close_protocol;
	void *open_protocol_information;
	void *protocols_per_handle;
	void *locate_handle_buffer;
	void *locate_protocol;
	void *install_multiple_protocol_interfaces;
	void *uninstall_multiple_protocol_intefaces;
	void *calculate_crc32;
	void *copy_mem;
	void *set_mem;
	void *create_event_ex;
} efi_boot_services_t;

typedef struct {
	efi_table_header_t hdr;
	char *fw_vendor;
	uint32_t fw_revision;
	void *cons_in_handle;
	void *cons_in;
	void *cons_out_handle;
	efi_simple_text_output_protocol_t *cons_out;
	void *cons_err_handle;
	efi_simple_text_output_protocol_t *cons_err;
	void *runtime_services;
	efi_boot_services_t *boot_services;
	sysarg_t conf_table_entries;
	efi_configuration_table_t *conf_table;
} efi_system_table_t;

#define EFI_PAGE_SIZE	4096

extern void *efi_vendor_table_find(efi_system_table_t *, efi_guid_t);
extern efi_status_t efi_get_memory_map(efi_system_table_t *, sysarg_t *,
    efi_v1_memdesc_t **, sysarg_t *, sysarg_t *, uint32_t *);

#endif
