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

/** @addtogroup mips32mm	
 * @{
 */
/** @file
 */

#include <macros.h>
#include <arch/mm/frame.h>
#include <arch/mm/tlb.h>
#include <mm/frame.h>
#include <mm/asid.h>
#include <config.h>
#include <arch/drivers/msim.h>
#include <arch/drivers/serial.h>
#include <print.h>
#include <debug.h>

#define TLB_PAGE_MASK_1M	(0xff << 13)

#define ZERO_FRAMES			4096
#define ZERO_PAGE_WIDTH		20  /* 1M */
#define ZERO_PAGE_SIZE		(1 << ZERO_PAGE_WIDTH)
#define ZERO_PAGE_ASID		ASID_INVALID
#define ZERO_PAGE_TLBI		0
#define ZERO_PAGE_ADDR		0
#define ZERO_PAGE_OFFSET	(ZERO_PAGE_SIZE / sizeof(uint32_t) - 1)
#define ZERO_PAGE_VALUE		(*((volatile uint32_t *) ZERO_PAGE_ADDR + ZERO_PAGE_OFFSET))

#define MAX_REGIONS			32

typedef struct {
	pfn_t start;
	pfn_t count;
} phys_region_t;

static count_t phys_regions_count = 0;
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
	/* MSIM device (dprinter) */
	if (frame == (KA2PA(MSIM_VIDEORAM) >> ZERO_PAGE_WIDTH))
		return false;
	
	/* MSIM device (dkeyboard) */
	if (frame == (KA2PA(MSIM_KBD_ADDRESS) >> ZERO_PAGE_WIDTH))
		return false;
	
	/* Simics device (serial line) */
	if (frame == (KA2PA(SERIAL_ADDRESS) >> ZERO_PAGE_WIDTH))
		return false;
	
	return true;
}


/** Check whether frame is safe to write
 *
 * Returns true if given frame is safe for read/write test.
 * Returns false if given frame should not be touched.
 *
 */
static bool frame_safe(pfn_t frame) __attribute__((unused));
static bool frame_safe(pfn_t frame)
{
	/* Kernel structures */
	if ((frame << ZERO_PAGE_WIDTH) < KA2PA(config.base))
		return false;
	
	/* Kernel */
	if (overlaps(frame << ZERO_PAGE_WIDTH, ZERO_PAGE_SIZE,
	    KA2PA(config.base), config.kernel_size))
		return false;
	
	/* Kernel stack */
	if (overlaps(frame << ZERO_PAGE_WIDTH, ZERO_PAGE_SIZE,
	    KA2PA(config.stack_base), config.stack_size))
		return false;
	
	/* Init tasks */
	bool safe = true;
	count_t i;
	for (i = 0; i < init.cnt; i++)
		if (overlaps(frame << ZERO_PAGE_WIDTH, ZERO_PAGE_SIZE,
		    KA2PA(init.tasks[i].addr), init.tasks[i].size)) {
			safe = false;
			break;
		}
	
	return safe;
}

static void frame_add_region(pfn_t start_frame, pfn_t end_frame)
{
	if (end_frame > start_frame) {
		/* Convert 1M frames to 16K frames */
		pfn_t first = ADDR2PFN(start_frame << ZERO_PAGE_WIDTH);
		pfn_t count = ADDR2PFN((end_frame - start_frame - 1) << ZERO_PAGE_WIDTH);
		
		/* Interrupt vector frame is blacklisted */
		pfn_t conf_frame;
		if (start_frame == 0)
			conf_frame = 1;
		else
			conf_frame = first;
		
		zone_create(first, count, conf_frame, 0);
		
		if (phys_regions_count < MAX_REGIONS) {
			phys_regions[phys_regions_count].start = first;
			phys_regions[phys_regions_count].count = count;
			phys_regions_count++;
		}
	}
}


/** Create memory zones
 *
 * Walk through available 1 MB chunks of physical
 * memory and create zones.
 *
 */
void frame_arch_init(void)
{
	cp0_index_write(ZERO_PAGE_TLBI);
	tlbr();
	
	uint32_t orig_pagemask = cp0_pagemask_read();
	uint32_t orig_lo0 = cp0_entry_lo0_read();
	uint32_t orig_lo1 = cp0_entry_lo1_read();
	uint32_t orig_hi = cp0_entry_hi_read();
		
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
				
				cp0_index_write(ZERO_PAGE_TLBI);
				cp0_pagemask_write(TLB_PAGE_MASK_1M);
				cp0_entry_lo0_write(lo0.value);
				cp0_entry_lo1_write(lo1.value);
				cp0_entry_hi_write(hi.value);
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
			frame_add_region(start_frame, frame);
			start_frame = frame + 1;
			avail = true;
		}
	}
	
	frame_add_region(start_frame, frame);
	
	/* Cleanup TLB */
	cp0_index_write(ZERO_PAGE_TLBI);
	cp0_pagemask_write(orig_pagemask);
	cp0_entry_lo0_write(orig_lo0);
	cp0_entry_lo1_write(orig_lo1);
	cp0_entry_hi_write(orig_hi);
	tlbwi();
	
	/* Blacklist interrupt vector frame */
	frame_mark_unavailable(0, 1);
}


void physmem_print(void)
{
	printf("Base       Size\n");
	printf("---------- ----------\n");
	
	count_t i;
	for (i = 0; i < phys_regions_count; i++) {
		printf("%#10x %10u\n",
			PFN2ADDR(phys_regions[i].start), PFN2ADDR(phys_regions[i].count));
	}	
}

/** @}
 */
