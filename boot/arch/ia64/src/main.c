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
#include <memstr.h>
#include <version.h>
#include <macros.h>
#include <align.h>
#include <str.h>
#include <errno.h>
#include <inflate.h>
#include "../../components.h"

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

			switch ((efi_memory_type_t) md->type) {
			case EFI_CONVENTIONAL_MEMORY:
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

	printf(" %p|%p: boot info structure\n", &bootinfo, &bootinfo);
	printf(" %p|%p: kernel entry point\n",
	    (void *) KERNEL_ADDRESS, (void *) KERNEL_ADDRESS);
	printf(" %p|%p: loader entry point\n",
	    (void *) LOADER_ADDRESS, (void *) LOADER_ADDRESS);

	size_t i;
	for (i = 0; i < COMPONENTS; i++)
		printf(" %p|%p: %s image (%zu/%zu bytes)\n", components[i].addr,
		    components[i].addr, components[i].name,
		    components[i].inflated, components[i].size);

	void *dest[COMPONENTS];
	size_t top = KERNEL_ADDRESS;
	size_t cnt = 0;
	bootinfo.taskmap.cnt = 0;
	for (i = 0; i < min(COMPONENTS, TASKMAP_MAX_RECORDS); i++) {
		top = ALIGN_UP(top, PAGE_SIZE);

		if (i > 0) {
			bootinfo.taskmap.tasks[bootinfo.taskmap.cnt].addr =
			    (void *) top;
			bootinfo.taskmap.tasks[bootinfo.taskmap.cnt].size =
			    components[i].inflated;

			str_cpy(bootinfo.taskmap.tasks[bootinfo.taskmap.cnt].name,
			    BOOTINFO_TASK_NAME_BUFLEN, components[i].name);

			bootinfo.taskmap.cnt++;
		}

		dest[i] = (void *) top;
		top += components[i].inflated;
		cnt++;
	}

	printf("\nInflating components ... ");

	/*
	 * We will use the next available address for a copy of each component to
	 * make sure that inflate() works with disjunctive memory regions.
	 */
	top = ALIGN_UP(top, PAGE_SIZE);

	for (i = cnt; i > 0; i--) {
		printf("%s ", components[i - 1].name);

		/*
		 * Copy the component to a location which is guaranteed not to
		 * overlap with the destination for inflate().
		 */
		memmove((void *) top, components[i - 1].addr, components[i - 1].size);

		int err = inflate((void *) top, components[i - 1].size,
		    dest[i - 1], components[i - 1].inflated);

		if (err != EOK) {
			printf("\n%s: Inflating error %d, halting.\n",
			    components[i - 1].name, err);
			halt();
		}
	}

	printf(".\n");

	read_efi_memmap();
	read_sal_configuration();
	read_pal_configuration();

	printf("Booting the kernel ...\n");
	jump_to_kernel(&bootinfo);
}
