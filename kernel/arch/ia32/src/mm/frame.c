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

#define PHYSMEM_LIMIT32  UINT64_C(0x07c000000)
#define PHYSMEM_LIMIT64  UINT64_C(0x200000000)

size_t hardcoded_unmapped_ktext_size = 0;
size_t hardcoded_unmapped_kdata_size = 0;

uintptr_t last_frame = 0;

static void init_e820_memory(pfn_t minconf)
{
	unsigned int i;
	
	for (i = 0; i < e820counter; i++) {
		uint64_t base = e820table[i].base_address;
		uint64_t size = e820table[i].size;
		
#ifdef __32_BITS__
		/*
		 * XXX FIXME:
		 *
		 * Ignore zones which start above PHYSMEM_LIMIT32
		 * or clip zones which go beyond PHYSMEM_LIMIT32.
		 *
		 * The PHYSMEM_LIMIT32 (2 GB - 64 MB) is a rather
		 * arbitrary constant which allows to have at
		 * least 64 MB in the kernel address space to
		 * map hardware resources.
		 *
		 * The kernel uses fixed 1:1 identity mapping
		 * of the physical memory with 2:2 GB split.
		 * This is a severe limitation of the current
		 * kernel memory management.
		 *
		 */
		
		if (base > PHYSMEM_LIMIT32)
			continue;
		
		if (base + size > PHYSMEM_LIMIT32)
			size = PHYSMEM_LIMIT32 - base;
#endif
		
#ifdef __64_BITS__
		/*
		 * XXX FIXME:
		 *
		 * Ignore zones which start above PHYSMEM_LIMIT64
		 * or clip zones which go beyond PHYSMEM_LIMIT64.
		 *
		 * The PHYSMEM_LIMIT64 (8 GB) is the size of the
		 * fixed 1:1 identically mapped physical memory
		 * accessible during the bootstrap process.
		 * This is a severe limitation of the current
		 * kernel memory management.
		 *
		 */
		
		if (base > PHYSMEM_LIMIT64)
			continue;
		
		if (base + size > PHYSMEM_LIMIT64)
			size = PHYSMEM_LIMIT64 - base;
#endif
		
		if (e820table[i].type == MEMMAP_MEMORY_AVAILABLE) {
			/* To be safe, make the available zone possibly smaller */
			uint64_t new_base = ALIGN_UP(base, FRAME_SIZE);
			uint64_t new_size = ALIGN_DOWN(size - (new_base - base),
			    FRAME_SIZE);
			
			pfn_t pfn = ADDR2PFN(new_base);
			size_t count = SIZE2FRAMES(new_size);
			
			pfn_t conf;
			if ((minconf < pfn) || (minconf >= pfn + count))
				conf = pfn;
			else
				conf = minconf;
			
			zone_create(pfn, count, conf, ZONE_AVAILABLE);
			
			// XXX this has to be removed
			if (last_frame < ALIGN_UP(new_base + new_size, FRAME_SIZE))
				last_frame = ALIGN_UP(new_base + new_size, FRAME_SIZE);
		} else if ((e820table[i].type == MEMMAP_MEMORY_ACPI) ||
		    (e820table[i].type == MEMMAP_MEMORY_NVS)) {
			/* To be safe, make the firmware zone possibly larger */
			uint64_t new_base = ALIGN_DOWN(base, FRAME_SIZE);
			uint64_t new_size = ALIGN_UP(size + (base - new_base),
			    FRAME_SIZE);
			
			zone_create(ADDR2PFN(new_base), SIZE2FRAMES(new_size), 0,
			    ZONE_FIRMWARE);
		} else {
			/* To be safe, make the reserved zone possibly larger */
			uint64_t new_base = ALIGN_DOWN(base, FRAME_SIZE);
			uint64_t new_size = ALIGN_UP(size + (base - new_base),
			    FRAME_SIZE);
			
			zone_create(ADDR2PFN(new_base), SIZE2FRAMES(new_size), 0,
			    ZONE_RESERVED);
		}
	}
}

static const char *e820names[] = {
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
	printf("[base            ] [size            ] [name   ]\n");
	
	for (i = 0; i < e820counter; i++) {
		const char *name;
		
		if (e820table[i].type <= MEMMAP_MEMORY_UNUSABLE)
			name = e820names[e820table[i].type];
		else
			name = "invalid";
		
		printf("%#018" PRIx64 " %#018" PRIx64" %s\n", e820table[i].base_address,
		    e820table[i].size, name);
	}
}


void frame_low_arch_init(void)
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

void frame_high_arch_init(void)
{
}

/** @}
 */
