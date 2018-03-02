/*
 * Copyright (c) 2005 Jakub Jermar
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

/** @addtogroup sparc64mm
 * @{
 */
/** @file
 */

#include <arch/mm/frame.h>
#include <mm/frame.h>
#include <arch/boot/boot.h>
#include <typedefs.h>
#include <config.h>
#include <align.h>
#include <macros.h>

/** Create memory zones according to information stored in memmap.
 *
 * Walk the memory map and create frame zones according to it.
 */
static void frame_common_arch_init(bool low)
{
	unsigned int i;

	for (i = 0; i < memmap.cnt; i++) {
		uintptr_t base;
		size_t size;

		/*
		 * The memmap is created by HelenOS boot loader.
		 * It already contains no holes.
		 */

		/* To be safe, make the available zone possibly smaller */
		base = ALIGN_UP((uintptr_t) memmap.zones[i].start, FRAME_SIZE);
		size = ALIGN_DOWN(memmap.zones[i].size -
		    (base - ((uintptr_t) memmap.zones[i].start)), FRAME_SIZE);

		if (!frame_adjust_zone_bounds(low, &base, &size))
			continue;

		pfn_t confdata;
		pfn_t pfn = ADDR2PFN(base);
		size_t count = SIZE2FRAMES(size);

		if (low) {
			confdata = pfn;
			if (confdata == ADDR2PFN(KA2PA(PFN2ADDR(0))))
				confdata = ADDR2PFN(KA2PA(PFN2ADDR(2)));

			zone_create(pfn, count, confdata,
			    ZONE_AVAILABLE | ZONE_LOWMEM);
		} else {
			confdata = zone_external_conf_alloc(count);
			if (confdata != 0)
				zone_create(pfn, count, confdata,
				    ZONE_AVAILABLE | ZONE_HIGHMEM);
		}
	}
}


void frame_low_arch_init(void)
{
	if (config.cpu_active > 1)
		return;

	frame_common_arch_init(true);

	/*
	 * On sparc64, physical memory can start on a non-zero address.
	 * The generic frame_init() only marks PFN 0 as not free, so we
	 * must mark the physically first frame not free explicitly
	 * here, no matter what is its address.
	 */
	frame_mark_unavailable(ADDR2PFN(KA2PA(PFN2ADDR(0))), 1);

	/* PA2KA will work only on low-memory. */
	end_of_identity = PA2KA(config.physmem_end - FRAME_SIZE) + PAGE_SIZE;
}

void frame_high_arch_init(void)
{
	if (config.cpu_active > 1)
		return;

	frame_common_arch_init(false);
}

/** @}
 */
