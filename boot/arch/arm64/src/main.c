/*
 * Copyright (c) 2015 Petr Pavlu
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

/** @addtogroup boot_arm64
 * @{
 */
/** @file
 * @brief Bootstrap.
 */

#include <stddef.h>
#include <align.h>
#include <arch/arch.h>
#include <arch/asm.h>
#include <arch/barrier.h>
#include <arch/main.h>
#include <arch/regutils.h>
#include <arch/types.h>
#include <errno.h>
#include <inflate.h>
#include <kernel.h>
#include <macros.h>
#include <mem.h>
#include <payload.h>
#include <printf.h>
#include <putchar.h>
#include <str.h>
#include <version.h>

static efi_system_table_t *efi_system_table;

/** Translate given UEFI memory type to the bootinfo memory type.
 *
 * @param type UEFI memory type.
 */
static memtype_t get_memtype(uint32_t type)
{
	switch (type) {
	case EFI_RESERVED:
	case EFI_RUNTIME_SERVICES_CODE:
	case EFI_RUNTIME_SERVICES_DATA:
	case EFI_UNUSABLE_MEMORY:
	case EFI_ACPI_MEMORY_NVS:
	case EFI_MEMORY_MAPPED_IO:
	case EFI_MEMORY_MAPPED_IO_PORT_SPACE:
	case EFI_PAL_CODE:
		return MEMTYPE_UNUSABLE;
	case EFI_LOADER_CODE:
	case EFI_LOADER_DATA:
	case EFI_BOOT_SERVICES_CODE:
	case EFI_BOOT_SERVICES_DATA:
	case EFI_CONVENTIONAL_MEMORY:
	case EFI_PERSISTENT_MEMORY:
		return MEMTYPE_AVAILABLE;
	case EFI_ACPI_RECLAIM_MEMORY:
		return MEMTYPE_ACPI_RECLAIM;
	}

	return MEMTYPE_UNUSABLE;
}

/** Send a byte to the UEFI console output.
 *
 * @param byte Byte to send.
 */
static void scons_sendb(uint8_t byte)
{
	int16_t out[2] = { byte, '\0' };
	efi_system_table->cons_out->output_string(efi_system_table->cons_out,
	    out);
}

/** Display a character.
 *
 * @param ch Character to display.
 *
 */
void putuchar(char32_t ch)
{
	if (ch == '\n')
		scons_sendb('\r');

	if (ascii_check(ch))
		scons_sendb((uint8_t) ch);
	else
		scons_sendb('?');
}

efi_status_t bootstrap(void *efi_handle_in,
    efi_system_table_t *efi_system_table_in, void *load_address)
{
	efi_status_t status;
	uint64_t memmap = 0;
	sysarg_t memmap_size;
	sysarg_t memmap_key;
	sysarg_t memmap_descriptor_size;
	uint32_t memmap_descriptor_version;
	uint64_t alloc_addr = 0;
	sysarg_t alloc_pages = 0;

	/*
	 * Bootinfo structure is dynamically allocated in the ARM64 port. It is
	 * placed directly after the inflated components. This assures that if
	 * the kernel identity maps the first gigabyte of the main memory in the
	 * kernel/upper address space then it can access the bootinfo because
	 * the inflated components and bootinfo can always fit in this area.
	 */
	bootinfo_t *bootinfo;

	efi_system_table = efi_system_table_in;

	version_print();

	printf("Boot loader: %p -> %p\n", loader_start, loader_end);
	printf("\nMemory statistics\n");
	printf(" %p|%p: loader\n", load_address, load_address);
	printf(" %p|%p: UEFI system table\n", efi_system_table_in,
	    efi_system_table_in);

	/* Obtain memory map. */
	status = efi_get_memory_map(efi_system_table, &memmap_size,
	    (efi_v1_memdesc_t **) &memmap, &memmap_key, &memmap_descriptor_size,
	    &memmap_descriptor_version);
	if (status != EFI_SUCCESS) {
		printf("Error: Unable to obtain initial memory map, status "
		    "code: %" PRIx64 ".\n", status);
		goto fail;
	}

	/* Find start of usable RAM. */
	uint64_t memory_base = (uint64_t) -1;
	for (sysarg_t i = 0; i < memmap_size / memmap_descriptor_size; i++) {
		efi_v1_memdesc_t *desc = (void *) memmap +
		    (i * memmap_descriptor_size);
		if (get_memtype(desc->type) != MEMTYPE_AVAILABLE ||
		    !(desc->attribute & EFI_MEMORY_WB))
			continue;

		if (desc->phys_start < memory_base)
			memory_base = desc->phys_start;
	}

	/* Deallocate memory holding the map. */
	efi_system_table->boot_services->free_pool((void *) memmap);
	memmap = 0;

	if (memory_base == (uint64_t) -1) {
		printf("Error: Memory map does not contain any usable RAM.\n");
		status = EFI_UNSUPPORTED;
		goto fail;
	}

	/*
	 * Check that everything is aligned on a 4 KiB boundary and the kernel can
	 * be placed by the decompression code at a correct address.
	 */

	/* Statically check PAGE_SIZE and BOOT_OFFSET. */
	_Static_assert(PAGE_SIZE == 4096, "PAGE_SIZE must be equal to 4096");
	_Static_assert(IS_ALIGNED(BOOT_OFFSET, PAGE_SIZE),
	    "BOOT_OFFSET must be a multiple of PAGE_SIZE");

	/*
	 * Dynamically check the memory base. The condition should be always
	 * true because UEFI guarantees each physical/virtual address in the
	 * memory map is aligned on a 4 KiB boundary.
	 */
	if (!IS_ALIGNED(memory_base, PAGE_SIZE)) {
		printf("Error: Start of usable RAM (%p) is not aligned on a "
		    "4 KiB boundary.\n", (void *) memory_base);
		status = EFI_UNSUPPORTED;
		goto fail;
	}

	/*
	 * Calculate where the components (including the kernel) will get
	 * placed.
	 */
	uint64_t decompress_base = memory_base + BOOT_OFFSET;
	printf(" %p|%p: kernel entry point\n", (void *) decompress_base,
	    (void *) decompress_base);

	/*
	 * Allocate memory for the decompressed components and for the bootinfo.
	 */
	uint64_t component_pages =
	    ALIGN_UP(payload_unpacked_size(), EFI_PAGE_SIZE) / EFI_PAGE_SIZE;
	uint64_t bootinfo_pages = ALIGN_UP(sizeof(*bootinfo), EFI_PAGE_SIZE) /
	    EFI_PAGE_SIZE;
	alloc_pages = component_pages + bootinfo_pages;
	alloc_addr = decompress_base;
	status = efi_system_table->boot_services->allocate_pages(
	    EFI_ALLOCATE_ADDRESS, EFI_LOADER_CODE, alloc_pages, &alloc_addr);
	if (status != EFI_SUCCESS) {
		printf("Error: Unable to allocate memory for inflated "
		    "components and bootinfo, status code: %" PRIx64 ".\n",
		    status);
		goto fail;
	}

	bootinfo = (void *) alloc_addr + component_pages * EFI_PAGE_SIZE;
	printf(" %p|%p: boot info structure\n", bootinfo, bootinfo);
	memset(bootinfo, 0, sizeof(*bootinfo));

	/* Decompress the components. */
	uint8_t *kernel_dest = (uint8_t *) alloc_addr;
	uint8_t *ram_end = kernel_dest + component_pages * EFI_PAGE_SIZE;

	extract_payload(&bootinfo->taskmap, kernel_dest, ram_end,
	    (uintptr_t) kernel_dest, smc_coherence);

	/* Get final memory map. */
	status = efi_get_memory_map(efi_system_table, &memmap_size,
	    (efi_v1_memdesc_t **) &memmap, &memmap_key, &memmap_descriptor_size,
	    &memmap_descriptor_version);
	if (status != EFI_SUCCESS) {
		printf("Error: Unable to obtain final memory map, status code: "
		    "%" PRIx64 ".\n", status);
		goto fail;
	}

	/* Convert the UEFI memory map to the bootinfo representation. */
	size_t cnt = 0;
	memtype_t current_type = MEMTYPE_UNUSABLE;
	void *current_start = 0;
	size_t current_size = 0;
	sysarg_t memmap_items_count = memmap_size / memmap_descriptor_size;
	for (sysarg_t i = 0; i < memmap_items_count; i++) {
		efi_v1_memdesc_t *desc = (void *) memmap +
		    (i * memmap_descriptor_size);

		/* Get type of the new area. */
		memtype_t type;
		if (!(desc->attribute & EFI_MEMORY_WB))
			type = MEMTYPE_UNUSABLE;
		else
			type = get_memtype(desc->type);

		/* Try to merge the new area with the previous one. */
		if (type == current_type &&
		    (uint64_t)current_start + current_size == desc->phys_start) {
			current_size += desc->pages * EFI_PAGE_SIZE;
			if (i != memmap_items_count - 1)
				continue;
		}

		/* Record the previous area. */
		if (current_type != MEMTYPE_UNUSABLE) {
			if (cnt >= MEMMAP_MAX_RECORDS) {
				printf("Error: Too many usable memory "
				    "areas.\n");
				status = EFI_UNSUPPORTED;
				goto fail;
			}
			bootinfo->memmap.zones[cnt].type = current_type;
			bootinfo->memmap.zones[cnt].start = current_start;
			bootinfo->memmap.zones[cnt].size = current_size;
			cnt++;
		}

		/* Remember the new area. */
		current_type = type;
		current_start = (void *) desc->phys_start;
		current_size = desc->pages * EFI_PAGE_SIZE;
	}
	bootinfo->memmap.cnt = cnt;

	/* Flush the data cache containing bootinfo. */
	dcache_flush(bootinfo, sizeof(*bootinfo));

	uintptr_t entry = check_kernel_translated((void *) decompress_base,
	    BOOT_OFFSET);

	printf("Booting the kernel...\n");

	/* Exit boot services. This is a point of no return. */
	efi_system_table->boot_services->exit_boot_services(efi_handle_in,
	    memmap_key);

	entry = memory_base + KA2PA(entry);
	jump_to_kernel((void *) entry, bootinfo);

fail:
	if (memmap != 0)
		efi_system_table->boot_services->free_pool((void *) memmap);

	if (alloc_addr != 0)
		efi_system_table->boot_services->free_pages(alloc_addr,
		    alloc_pages);

	return status;
}

/** @}
 */
