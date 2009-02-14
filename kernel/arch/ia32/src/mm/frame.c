/*
 * Copyright (c) 2008 Jakub Jermar
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

/** @addtogroup ia32mm	
 * @{
 */
/** @file
 * @ingroup ia32mm, amd64mm
 */

#include <mm/frame.h>
#include <arch/mm/frame.h>
#include <mm/as.h>
#include <config.h>
#include <arch/boot/boot.h>
#include <arch/boot/memmap.h>
#include <panic.h>
#include <debug.h>
#include <align.h>
#include <macros.h>

#include <print.h>

size_t hardcoded_unmapped_ktext_size = 0;
size_t hardcoded_unmapped_kdata_size = 0;

uintptr_t last_frame = 0;

static void init_e820_memory(pfn_t minconf)
{
	unsigned int i;
	pfn_t start, conf;
	size_t size;

	for (i = 0; i < e820counter; i++) {
		if (e820table[i].type == MEMMAP_MEMORY_AVAILABLE) {
			start = ADDR2PFN(ALIGN_UP(e820table[i].base_address, FRAME_SIZE));
			size = SIZE2FRAMES(ALIGN_DOWN(e820table[i].size, FRAME_SIZE));
			
			if ((minconf < start) || (minconf >= start + size))
				conf = start;
			else
				conf = minconf;
			
			zone_create(start, size, conf, 0);
			
			if (last_frame < ALIGN_UP(e820table[i].base_address +
			    e820table[i].size, FRAME_SIZE))
				last_frame =
				    ALIGN_UP(e820table[i].base_address + e820table[i].size, FRAME_SIZE);
		}			
	}
}

static char *e820names[] = {
	"invalid",
	"available",
	"reserved",
	"acpi",
	"nvs",
	"unusable"
};


void physmem_print(void)
{
	unsigned int i;
	char *name;
	
	printf("Base               Size               Name\n");
	printf("------------------ ------------------ ---------\n");
		
	for (i = 0; i < e820counter; i++) {
		if (e820table[i].type <= MEMMAP_MEMORY_UNUSABLE)
			name = e820names[e820table[i].type];
		else
			name = "invalid";
		
		printf("%#18llx %#18llx %s\n", e820table[i].base_address,
			e820table[i].size, name);
	}
}


void frame_arch_init(void)
{
	pfn_t minconf;
	
	if (config.cpu_active == 1) {
		minconf = 1;
#ifdef CONFIG_SMP
		minconf = max(minconf,
			ADDR2PFN(AP_BOOT_OFFSET + hardcoded_unmapped_ktext_size +
			hardcoded_unmapped_kdata_size));
#endif
		init_e820_memory(minconf);

		/* Reserve frame 0 (BIOS data) */
		frame_mark_unavailable(0, 1);
		
#ifdef CONFIG_SMP
		/* Reserve AP real mode bootstrap memory */
		frame_mark_unavailable(AP_BOOT_OFFSET >> FRAME_WIDTH, 
			(hardcoded_unmapped_ktext_size +
			hardcoded_unmapped_kdata_size) >> FRAME_WIDTH);
#endif
	}
}

/** @}
 */
