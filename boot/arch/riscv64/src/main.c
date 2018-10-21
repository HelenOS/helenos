/*
 * Copyright (c) 2016 Martin Decky
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
#include <arch/arch.h>
#include <arch/asm.h>
#include <arch/ucb.h>
#include <arch/mm.h>
#include <arch/types.h>
#include <version.h>
#include <stddef.h>
#include <printf.h>
#include <macros.h>
#include <errno.h>
#include <align.h>
#include <str.h>
#include <halt.h>
#include <payload.h>
#include <kernel.h>

static bootinfo_t bootinfo;

void bootstrap(void)
{
	version_print();

	bootinfo.htif_frame = ((uintptr_t) &htif_page) >> PAGE_WIDTH;
	bootinfo.pt_frame = ((uintptr_t) &pt_page) >> PAGE_WIDTH;

	bootinfo.ucbinfo.tohost =
	    (volatile uint64_t *) PA2KA((uintptr_t) &tohost);
	bootinfo.ucbinfo.fromhost =
	    (volatile uint64_t *) PA2KA((uintptr_t) &fromhost);

	// FIXME TODO: read from device tree
	bootinfo.physmem_start = PHYSMEM_START;
	bootinfo.memmap.total = PHYSMEM_SIZE;
	bootinfo.memmap.cnt = 1;
	bootinfo.memmap.zones[0].start = (void *) PHYSMEM_START;
	bootinfo.memmap.zones[0].size = PHYSMEM_SIZE;

	printf("\nMemory statistics (total %" PRIu64 " MB, starting at %p)\n\n",
	    bootinfo.memmap.total >> 20, (void *) bootinfo.physmem_start);
	printf(" %p: boot info structure\n", &bootinfo);

	uint8_t *load_addr = (uint8_t *) BOOT_OFFSET;
	uintptr_t kernel_addr = PA2KA(load_addr);

	printf(" %p: inflate area\n", load_addr);
	printf(" %p: kernel entry point\n", (void *) kernel_addr);

	/* Find the end of the memory zone containing the load address. */
	uint8_t *end = NULL;
	for (size_t i = 0; i < bootinfo.memmap.cnt; i++) {
		memzone_t z = bootinfo.memmap.zones[i];

		if (z.start <= (void *) load_addr &&
		    z.start + z.size > (void *) load_addr) {
			end = z.start + z.size;
		}
	}

	// TODO: Cache-coherence callback?
	extract_payload(&bootinfo.taskmap, load_addr, end, kernel_addr, NULL);

	uintptr_t entry = check_kernel(load_addr);

	printf("Booting the kernel...\n");
	jump_to_kernel(PA2KA(&bootinfo), entry);
}
