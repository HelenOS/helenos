/*
 * Copyright (c) 2005 Martin Decky
 * Copyright (c) 2006 Jakub Jermar
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

#include <arch/main.h>
#include <arch/types.h>
#include <arch/arch.h>
#include <arch/asm.h>
#include <genarch/efi.h>
#include <arch/sal.h>
#include <arch/pal.h>
#include <halt.h>
#include <printf.h>
#include <mem.h>
#include <version.h>
#include <macros.h>
#include <align.h>
#include <str.h>
#include <errno.h>
#include <payload.h>
#include <kernel.h>

#define DEFAULT_MEMORY_BASE		0x4000000ULL
#define DEFAULT_MEMORY_SIZE		(256 * 1024 * 1024)
#define DEFAULT_LEGACY_IO_BASE		0x00000FFFFC000000ULL
#define DEFAULT_LEGACY_IO_SIZE		0x4000000ULL

#define DEFAULT_FREQ_SCALE		0x0000000100000001ULL	/* 1/1 */
#define DEFAULT_SYS_FREQ		100000000ULL		/* 100MHz */

#define MEMMAP_FREE_MEM		0
#define MEMMAP_IO		1
#define MEMMAP_IO_PORTS		2

extern boot_param_t *bootpar;

static bootinfo_t bootinfo;

static void read_efi_memmap(void)
{
	memmap_item_t *memmap = bootinfo.memmap;
	size_t items = 0;

	if (!bootpar) {
		/* Fake-up a memory map for simulators. */
		memmap[items].base = DEFAULT_MEMORY_BASE;
		memmap[items].size = DEFAULT_MEMORY_SIZE;
		memmap[items].type = MEMMAP_FREE_MEM;
		items++;

		memmap[items].base = DEFAULT_LEGACY_IO_BASE;
		memmap[items].size = DEFAULT_LEGACY_IO_SIZE;
		memmap[items].type = MEMMAP_IO_PORTS;
		items++;
	} else {
		char *cur, *mm_base = (char *) bootpar->efi_memmap;
		size_t mm_size = bootpar->efi_memmap_sz;
		size_t md_size = bootpar->efi_memdesc_sz;

		/*
		 * Walk the EFI memory map using the V1 memory descriptor
		 * format. The actual memory descriptor can use newer format,
		 * but it must always be backwards compatible with the V1
		 * format.
		 */
		for (cur = mm_base;
		    (cur < mm_base + (mm_size - md_size)) &&
		    (items < MEMMAP_ITEMS);
		    cur += md_size) {
			efi_v1_memdesc_t *md = (efi_v1_memdesc_t *) cur;
			memmap_item_t *o = NULL;

			if (items)
				o = &memmap[items - 1];

			switch ((efi_memory_type_t) md->type) {
			case EFI_LOADER_CODE:
			case EFI_LOADER_DATA:
			case EFI_BOOT_SERVICES_CODE:
			case EFI_BOOT_SERVICES_DATA:
			case EFI_CONVENTIONAL_MEMORY:
				if (o && o->type == MEMMAP_FREE_MEM &&
				    o->base + o->size == md->phys_start) {
					o->size += md->pages * EFI_PAGE_SIZE;
					continue;
				}
				memmap[items].type = MEMMAP_FREE_MEM;
				break;
			case EFI_MEMORY_MAPPED_IO:
				memmap[items].type = MEMMAP_IO;
				break;
			case EFI_MEMORY_MAPPED_IO_PORT_SPACE:
				memmap[items].type = MEMMAP_IO_PORTS;
				break;
			default:
				continue;
			}

			memmap[items].base = md->phys_start;
			memmap[items].size = md->pages * EFI_PAGE_SIZE;
			items++;
		}
	}

	bootinfo.memmap_items = items;
}

static void read_pal_configuration(void)
{
	if (bootpar) {
		bootinfo.freq_scale = pal_proc_freq_ratio();
	} else {
		/* Configure default values for simulators. */
		bootinfo.freq_scale = DEFAULT_FREQ_SCALE;
	}
}

static void read_sal_configuration(void)
{
	if (bootpar && bootpar->efi_system_table) {
		efi_guid_t sal_guid = SAL_SYSTEM_TABLE_GUID;
		sal_system_table_header_t *sal_st;

		sal_st = efi_vendor_table_find(
		    (efi_system_table_t *) bootpar->efi_system_table, sal_guid);

		sal_system_table_parse(sal_st);

		bootinfo.sys_freq = sal_base_clock_frequency();
	} else {
		/* Configure default values for simulators. */
		bootinfo.sys_freq = DEFAULT_SYS_FREQ;
	}
}

void bootstrap(void)
{
	version_print();

	printf("Boot loader: %p -> %p\n", loader_start, loader_end);
	printf(" %p|%p: boot info structure\n", &bootinfo, &bootinfo);
	printf(" %p|%p: kernel entry point\n",
	    (void *) KERNEL_ADDRESS, (void *) KERNEL_ADDRESS);
	printf(" %p|%p: loader entry point\n",
	    (void *) LOADER_ADDRESS, (void *) LOADER_ADDRESS);

	read_efi_memmap();
	read_sal_configuration();
	read_pal_configuration();

	uint8_t *kernel_start = (uint8_t *) KERNEL_ADDRESS;
	uint8_t *ram_end = NULL;

	/* Find the end of the memory area occupied by the kernel. */
	for (unsigned i = 0; i < bootinfo.memmap_items; i++) {
		memmap_item_t m = bootinfo.memmap[i];
		if (m.type == MEMMAP_FREE_MEM &&
		    m.base <= (uintptr_t) kernel_start &&
		    m.base + m.size > (uintptr_t) kernel_start) {
			ram_end = (uint8_t *) (m.base + m.size);
		}
	}

	if (ram_end == NULL) {
		printf("Memory map doesn't contain usable area at kernel's address.\n");
		halt();
	}

	// FIXME: Correct kernel's logical address.
	extract_payload(&bootinfo.taskmap, kernel_start, ram_end,
	    (uintptr_t) kernel_start, NULL);

	uintptr_t entry = check_kernel(kernel_start);

	// FIXME: kernel's entry point is linked at a different address than
	//        where it is run from.
	entry = entry - KERNEL_VADDRESS + KERNEL_ADDRESS;

	printf("Booting the kernel at %p...\n", (void *) entry);
	jump_to_kernel(&bootinfo, (void *) entry);
}
