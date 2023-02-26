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

/** @addtogroup kernel_mips32_mm
 * @{
 */
/** @file
 */

#include <macros.h>
#include <arch/mm/frame.h>
#include <arch/mm/tlb.h>
#include <interrupt.h>
#include <mm/frame.h>
#include <mm/asid.h>
#include <config.h>
#ifdef MACHINE_msim
#include <arch/mach/msim/msim.h>
#endif
#include <arch/arch.h>
#include <stdio.h>

#define ZERO_PAGE_MASK    TLB_PAGE_MASK_256K
#define ZERO_FRAMES       2048
#define ZERO_PAGE_WIDTH   18  /* 256K */
#define ZERO_PAGE_SIZE    (1 << ZERO_PAGE_WIDTH)
#define ZERO_PAGE_ASID    ASID_INVALID
#define ZERO_PAGE_TLBI    0
#define ZERO_PAGE_ADDR    0
#define ZERO_PAGE_OFFSET  (ZERO_PAGE_SIZE / sizeof(uint32_t) - 1)
#define ZERO_PAGE_VALUE   (((volatile uint32_t *) ZERO_PAGE_ADDR)[ZERO_PAGE_OFFSET])

#define ZERO_PAGE_VALUE_KSEG1(frame) \
	(((volatile uint32_t *) PA2KSEG1(frame << ZERO_PAGE_WIDTH))[ZERO_PAGE_OFFSET])

#define MAX_REGIONS  32

typedef struct {
	pfn_t start;
	pfn_t count;
} phys_region_t;

static size_t phys_regions_count = 0;
static phys_region_t phys_regions[MAX_REGIONS];

/** Check whether frame is available
 *
 * Returns true if given frame is generally available for use.
 * Returns false if given frame is used for physical memory
 * mapped devices and cannot be used.
 *
 */
static bool frame_available(pfn_t frame)
{
#ifdef MACHINE_msim
	/* MSIM device (dprinter) */
	if (frame == (KSEG12PA(MSIM_VIDEORAM) >> ZERO_PAGE_WIDTH))
		return false;

	/* MSIM device (dkeyboard) */
	if (frame == (KSEG12PA(MSIM_KBD_ADDRESS) >> ZERO_PAGE_WIDTH))
		return false;
#endif

#if defined(MACHINE_lmalta) || defined(MACHINE_bmalta)
	if (frame >= (sdram_size >> ZERO_PAGE_WIDTH))
		return false;
#endif

	return true;
}

/** Check whether frame is safe to write
 *
 * Returns true if given frame is safe for read/write test.
 * Returns false if given frame should not be touched.
 *
 */
static bool frame_safe(pfn_t frame)
{
	/* Kernel structures */
	if ((frame << ZERO_PAGE_WIDTH) < KA2PA(config.base))
		return false;

	/* Kernel */
	if (overlaps(frame << ZERO_PAGE_WIDTH, ZERO_PAGE_SIZE,
	    KA2PA(config.base), config.kernel_size))
		return false;

	/* Boot allocations */
	if (overlaps(frame << ZERO_PAGE_WIDTH, ZERO_PAGE_SIZE,
	    KA2PA(ballocs.base), ballocs.size))
		return false;

	/* Init tasks */
	bool safe = true;
	size_t i;
	for (i = 0; i < init.cnt; i++)
		if (overlaps(frame << ZERO_PAGE_WIDTH, ZERO_PAGE_SIZE,
		    init.tasks[i].paddr, init.tasks[i].size)) {
			safe = false;
			break;
		}

	return safe;
}

static void frame_add_region(pfn_t start_frame, pfn_t end_frame, bool low)
{
	if (end_frame <= start_frame)
		return;

	uintptr_t base = start_frame << ZERO_PAGE_WIDTH;
	size_t size = (end_frame - start_frame) << ZERO_PAGE_WIDTH;

	if (!frame_adjust_zone_bounds(low, &base, &size))
		return;

	pfn_t first = ADDR2PFN(base);
	size_t count = SIZE2FRAMES(size);
	pfn_t conf_frame;

	if (low) {
		/* Interrupt vector frame is blacklisted */
		if (first == 0)
			conf_frame = 1;
		else
			conf_frame = first;
		zone_create(first, count, conf_frame,
		    ZONE_AVAILABLE | ZONE_LOWMEM);
	} else {
		conf_frame = zone_external_conf_alloc(count);
		if (conf_frame != 0)
			zone_create(first, count, conf_frame,
			    ZONE_AVAILABLE | ZONE_HIGHMEM);
	}

	if (phys_regions_count < MAX_REGIONS) {
		phys_regions[phys_regions_count].start = first;
		phys_regions[phys_regions_count].count = count;
		phys_regions_count++;
	}
}

/** Create memory zones
 *
 * Walk through available 256 KB chunks of physical
 * memory and create zones.
 *
 * Note: It is assumed that the TLB is not yet being
 * used in any way, thus there is no interference.
 *
 */
void frame_low_arch_init(void)
{
	ipl_t ipl = interrupts_disable();

	/* Clear and initialize TLB */
	cp0_pagemask_write(ZERO_PAGE_MASK);
	cp0_entry_lo0_write(0);
	cp0_entry_lo1_write(0);
	cp0_entry_hi_write(0);

	size_t i;
	for (i = 0; i < TLB_ENTRY_COUNT; i++) {
		cp0_index_write(i);
		tlbwi();
	}

	pfn_t start_frame = 0;
	pfn_t frame;
	bool avail = true;

	/* Walk through all 1 MB frames */
	for (frame = 0; frame < ZERO_FRAMES; frame++) {
		if (!frame_available(frame))
			avail = false;
		else {
			if (frame_safe(frame)) {
				entry_lo_t lo0;
				entry_lo_t lo1;
				entry_hi_t hi;
				tlb_prepare_entry_lo(&lo0, false, true, true, false, frame << (ZERO_PAGE_WIDTH - 12));
				tlb_prepare_entry_lo(&lo1, false, false, false, false, 0);
				tlb_prepare_entry_hi(&hi, ZERO_PAGE_ASID, ZERO_PAGE_ADDR);

				cp0_pagemask_write(ZERO_PAGE_MASK);
				cp0_entry_lo0_write(lo0.value);
				cp0_entry_lo1_write(lo1.value);
				cp0_entry_hi_write(hi.value);
				cp0_index_write(ZERO_PAGE_TLBI);
				tlbwi();

				ZERO_PAGE_VALUE = 0;
				if (ZERO_PAGE_VALUE != 0)
					avail = false;
				else {
					ZERO_PAGE_VALUE = 0xdeadbeef;
					if (ZERO_PAGE_VALUE != 0xdeadbeef)
						avail = false;
				}
			}
		}

		if (!avail) {
			frame_add_region(start_frame, frame, true);
			start_frame = frame + 1;
			avail = true;
		}
	}

	frame_add_region(start_frame, frame, true);

	/* Blacklist interrupt vector frame */
	frame_mark_unavailable(0, 1);

#if defined(MACHINE_lmalta) || defined(MACHINE_bmalta)
	/*
	 * Blacklist memory regions used by YAMON.
	 *
	 * The YAMON User's Manual vaguely says the following physical addresses
	 * are taken by YAMON:
	 *
	 * 0x1000	YAMON functions
	 * 0x5000	YAMON code
	 *
	 * These addresses overlap with the beginning of the SDRAM so we need to
	 * make sure they cannot be allocated.
	 *
	 * The User's Manual unfortunately does not say where does the SDRAM
	 * portion used by YAMON end.
	 *
	 * Looking into the YAMON 02.21 sources, it looks like the first free
	 * address is computed dynamically and depends on the size of the YAMON
	 * image. From the YAMON binary, it appears to be 0xc0d50 or roughly
	 * 772 KiB for that particular version.
	 *
	 * Linux is linked to 1MiB which seems to be a safe bet and a reasonable
	 * upper bound for memory taken by YAMON. We will use it too.
	 */
	frame_mark_unavailable(0, 1024 * 1024 / FRAME_SIZE);
#endif

	/* Cleanup */
	cp0_pagemask_write(ZERO_PAGE_MASK);
	cp0_entry_lo0_write(0);
	cp0_entry_lo1_write(0);
	cp0_entry_hi_write(0);
	cp0_index_write(ZERO_PAGE_TLBI);
	tlbwi();

	interrupts_restore(ipl);
}

void frame_high_arch_init(void)
{
}

void physmem_print(void)
{
	printf("[base    ] [size    ]\n");

	size_t i;
	for (i = 0; i < phys_regions_count; i++) {
		printf("%#010x %10u\n",
		    PFN2ADDR(phys_regions[i].start), PFN2ADDR(phys_regions[i].count));
	}
}

/** @}
 */
