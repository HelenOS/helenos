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

/** @addtogroup riscv64mm
 * @{
 */

#include <mm/frame.h>
#include <arch/boot/boot.h>
#include <arch/mm/frame.h>
#include <arch/drivers/ucb.h>
#include <mm/as.h>
#include <config.h>
#include <panic.h>
#include <debug.h>
#include <align.h>
#include <macros.h>
#include <print.h>

uintptr_t physmem_start;
uintptr_t htif_frame;
uintptr_t pt_frame;
memmap_t memmap;

void physmem_print(void)
{
}

static void frame_common_arch_init(bool low)
{
	pfn_t minconf =
	    max3(ADDR2PFN(physmem_start), htif_frame + 1, pt_frame + 1);

	for (size_t i = 0; i < memmap.cnt; i++) {
		/* To be safe, make the available zone possibly smaller */
		uintptr_t base = ALIGN_UP((uintptr_t) memmap.zones[i].start,
		    FRAME_SIZE);
		size_t size = ALIGN_DOWN(memmap.zones[i].size -
		    (base - ((uintptr_t) memmap.zones[i].start)), FRAME_SIZE);

		if (!frame_adjust_zone_bounds(low, &base, &size))
			return;

		pfn_t pfn = ADDR2PFN(base);
		size_t count = SIZE2FRAMES(size);
		pfn_t conf;

		if (low) {
			if ((minconf < pfn) || (minconf >= pfn + count))
				conf = pfn;
			else
				conf = minconf;

			zone_create(pfn, count, conf,
			    ZONE_AVAILABLE | ZONE_LOWMEM);
		} else {
			conf = zone_external_conf_alloc(count);
			if (conf != 0)
				zone_create(pfn, count, conf,
				    ZONE_AVAILABLE | ZONE_HIGHMEM);
		}
	}
}

void frame_low_arch_init(void)
{
	frame_common_arch_init(true);

	frame_mark_unavailable(htif_frame, 1);
	frame_mark_unavailable(pt_frame, 1);
}

void frame_high_arch_init(void)
{
	frame_common_arch_init(false);
}

/** @}
 */
