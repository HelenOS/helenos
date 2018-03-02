/*
 * Copyright (c) 2007 Pavel Jancik, Michal Kebrt
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

/** @addtogroup arm32boot
 * @{
 */
/** @file
 * @brief Memory management used while booting the kernel.
 */

#include <stdint.h>
#include <arch/asm.h>
#include <arch/mm.h>
#include <arch/cp15.h>

#ifdef PROCESSOR_ARCH_armv7_a
static unsigned log2(unsigned val)
{
	unsigned log = 0;
	while (val >> log++);
	return log - 2;
}

static void dcache_invalidate_level(unsigned level)
{
	CSSELR_write(level << 1);
	const uint32_t ccsidr = CCSIDR_read();
	const unsigned sets = CCSIDR_SETS(ccsidr);
	const unsigned ways = CCSIDR_WAYS(ccsidr);
	const unsigned line_log = CCSIDR_LINESIZE_LOG(ccsidr);
	const unsigned set_shift = line_log;
	const unsigned way_shift = 32 - log2(ways);

	for (unsigned k = 0; k < ways; ++k)
		for (unsigned j = 0; j < sets; ++j) {
			const uint32_t val = (level << 1) |
			    (j << set_shift) | (k << way_shift);
			DCISW_write(val);
		}
}

/** invalidate all dcaches -- armv7 */
static void cache_invalidate(void)
{
	const uint32_t cinfo = CLIDR_read();
	for (unsigned i = 0; i < 7; ++i) {
		switch (CLIDR_CACHE(i, cinfo))
		{
		case CLIDR_DCACHE_ONLY:
		case CLIDR_SEP_CACHE:
		case CLIDR_UNI_CACHE:
			dcache_invalidate_level(i);
		}
	}
	asm volatile ( "dsb\n" );
	ICIALLU_write(0);
	asm volatile ( "isb\n" );
}
#endif

/** Disable the MMU */
static void disable_paging(void)
{
	asm volatile (
		"mrc p15, 0, r0, c1, c0, 0\n"
		"bic r0, r0, #1\n"
		"mcr p15, 0, r0, c1, c0, 0\n"
		::: "r0"
	);
}

/** Check if caching can be enabled for a given memory section.
 *
 * Memory areas used for I/O are excluded from caching.
 * At the moment caching is enabled only on GTA02.
 *
 * @param section	The section number.
 *
 * @return	1 if the given section can be mapped as cacheable, 0 otherwise.
*/
static inline int section_cacheable(pfn_t section)
{
	const unsigned long address = section << PTE_SECTION_SHIFT;
#ifdef MACHINE_gta02
	if (address < GTA02_IOMEM_START || address >= GTA02_IOMEM_END)
		return 1;
#elif defined MACHINE_beagleboardxm
	if (address >= BBXM_RAM_START && address < BBXM_RAM_END)
		return 1;
#elif defined MACHINE_beaglebone
	if (address >= AM335x_RAM_START && address < AM335x_RAM_END)
		return 1;
#elif defined MACHINE_raspberrypi
	if (address < BCM2835_RAM_END)
		return 1;
#endif
	return address * 0;
}

/** Initialize "section" page table entry.
 *
 * Will be readable/writable by kernel with no access from user mode.
 * Will belong to domain 0. No cache or buffering is enabled.
 *
 * @param pte   Section entry to initialize.
 * @param frame First frame in the section (frame number).
 *
 * @note If frame is not 1 MB aligned, first lower 1 MB aligned frame will be
 *       used.
 *
 */
static void init_ptl0_section(pte_level0_section_t* pte,
    pfn_t frame)
{
	pte->descriptor_type = PTE_DESCRIPTOR_SECTION;
	pte->xn = 0;
	pte->domain = 0;
	pte->should_be_zero_1 = 0;
	pte->access_permission_0 = PTE_AP_USER_NO_KERNEL_RW;
#if defined(PROCESSOR_ARCH_armv6) || defined(PROCESSOR_ARCH_armv7_a)
	/*
	 * Keeps this setting in sync with memory type attributes in:
	 * init_boot_pt (boot/arch/arm32/src/mm.c)
	 * set_pt_level1_flags (kernel/arch/arm32/include/arch/mm/page_armv6.h)
	 * set_ptl0_addr (kernel/arch/arm32/include/arch/mm/page.h)
	 */
	pte->tex = section_cacheable(frame) ? 5 : 0;
	pte->cacheable = section_cacheable(frame) ? 0 : 0;
	pte->bufferable = section_cacheable(frame) ? 1 : 1;
#else
	pte->bufferable = section_cacheable(frame);
	pte->cacheable = section_cacheable(frame);
	pte->tex = 0;
#endif
	pte->access_permission_1 = 0;
	pte->shareable = 0;
	pte->non_global = 0;
	pte->should_be_zero_2 = 0;
	pte->non_secure = 0;
	pte->section_base_addr = frame;
}

/** Initialize page table used while booting the kernel. */
static void init_boot_pt(void)
{
	/*
	 * Create 1:1 virtual-physical mapping.
	 * Physical memory on BBxM a BBone starts at 2GB
	 * boundary, icp has a memory mirror at 2GB.
	 * (ARM Integrator Core Module User guide ch. 6.3,  p. 6-7)
	 * gta02 somehow works (probably due to limited address size),
	 * s3c2442b manual ch. 5, p.5-1:
	 * "Address space: 128Mbytes per bank (total 1GB/8 banks)"
	 */
	for (pfn_t page = 0; page < PTL0_ENTRIES; ++page)
		init_ptl0_section(&boot_pt[page], page);

	/*
	 * Tell MMU page might be cached. Keeps this setting in sync
	 * with memory type attributes in:
	 * init_ptl0_section (boot/arch/arm32/src/mm.c)
	 * set_pt_level1_flags (kernel/arch/arm32/include/arch/mm/page_armv6.h)
	 * set_ptl0_addr (kernel/arch/arm32/include/arch/mm/page.h)
	 */
	uint32_t val = (uint32_t)boot_pt & TTBR_ADDR_MASK;
#if defined(PROCESSOR_ARCH_armv6) || defined(PROCESSOR_ARCH_armv7_a)
	// FIXME: TTBR_RGN_WBWA_CACHE is unpredictable on ARMv6
	val |= TTBR_RGN_WBWA_CACHE | TTBR_C_FLAG;
#endif
	TTBR0_write(val);
}

static void enable_paging(void)
{
	/*
	 * c3   - each two bits controls access to the one of domains (16)
	 * 0b01 - behave as a client (user) of a domain
	 */
	asm volatile (
		/* Behave as a client of domains */
		"ldr r0, =0x55555555\n"
		"mcr p15, 0, r0, c3, c0, 0\n"

		/* Current settings */
		"mrc p15, 0, r0, c1, c0, 0\n"

		/* Enable ICache, DCache, BPredictors and MMU,
		 * we disable caches before jumping to kernel
		 * so this is safe for all archs.
		 * Enable VMSAv6 the bit (23) is only writable on ARMv6.
		 * (and QEMU)
		 */
#ifdef PROCESSOR_ARCH_armv6
		"ldr r1, =0x00801805\n"
#else
		"ldr r1, =0x00001805\n"
#endif

		"orr r0, r0, r1\n"

		/* Invalidate the TLB content before turning on the MMU.
		 * ARMv7-A Reference manual, B3.10.3
		 */
		"mcr p15, 0, r0, c8, c7, 0\n"

		/* Store settings, enable the MMU */
		"mcr p15, 0, r0, c1, c0, 0\n"
		::: "r0", "r1"
	);
}

/** Start the MMU - initialize page table and enable paging. */
void mmu_start(void)
{
	disable_paging();
#ifdef PROCESSOR_ARCH_armv7_a
	/* Make sure we run in memory code when caches are enabled,
	 * make sure we read memory data too. This part is ARMv7 specific as
	 * ARMv7 no longer invalidates caches on restart.
	 * See chapter B2.2.2 of ARM Architecture Reference Manual p. B2-1263*/
	cache_invalidate();
#endif
	init_boot_pt();
	enable_paging();
}

/** @}
 */
