/*
 * Copyright (c) 2006 Martin Decky
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

/** @addtogroup ppc32mm
 * @{
 */
/** @file
 */

#include <arch/mm/tlb.h>
#include <interrupt.h>
#include <typedefs.h>

void tlb_refill(unsigned int n, istate_t *istate)
{
	uint32_t tlbmiss;
	ptehi_t ptehi;
	ptelo_t ptelo;

	asm volatile (
	    "mfspr %[tlbmiss], 980\n"
	    "mfspr %[ptehi], 981\n"
	    "mfspr %[ptelo], 982\n"
	    : [tlbmiss] "=r" (tlbmiss),
	      [ptehi] "=r" (ptehi),
	      [ptelo] "=r" (ptelo)
	);

	uint32_t badvaddr = tlbmiss & 0xfffffffc;
	uint32_t physmem = physmem_top();

	if ((badvaddr < PA2KA(0)) || (badvaddr >= PA2KA(physmem)))
		return; // FIXME

	ptelo.rpn = KA2PA(badvaddr) >> 12;
	ptelo.wimg = 0;
	ptelo.pp = 2; // FIXME

	uint32_t index = 0;
	asm volatile (
	    "mtspr 981, %[ptehi]\n"
	    "mtspr 982, %[ptelo]\n"
	    "tlbld %[index]\n"
	    "tlbli %[index]\n"
	    : [index] "=r" (index)
	    : [ptehi] "r" (ptehi),
	      [ptelo] "r" (ptelo)
	);
}

void tlb_arch_init(void)
{
	tlb_invalidate_all();
}

void tlb_invalidate_all(void)
{
	asm volatile (
	    "sync\n"
	);

	for (unsigned int i = 0; i < 0x00040000; i += 0x00001000) {
		asm volatile (
		    "tlbie %[i]\n"
		    :: [i] "r" (i)
		);
	}

	asm volatile (
	    "eieio\n"
	    "tlbsync\n"
	    "sync\n"
	);
}

void tlb_invalidate_asid(asid_t asid)
{
	tlb_invalidate_all();
}

void tlb_invalidate_pages(asid_t asid, uintptr_t page, size_t cnt)
{
	tlb_invalidate_all();
}

#define PRINT_BAT(name, ureg, lreg) \
	asm volatile ( \
		"mfspr %[upper], " #ureg "\n" \
		"mfspr %[lower], " #lreg "\n" \
		: [upper] "=r" (upper), \
		  [lower] "=r" (lower) \
	); \
	\
	mask = (upper & 0x1ffc) >> 2; \
	if (upper & 3) { \
		uint32_t tmp = mask; \
		length = 128; \
		\
		while (tmp) { \
			if ((tmp & 1) == 0) { \
				printf("ibat[0]: error in mask\n"); \
				break; \
			} \
			length <<= 1; \
			tmp >>= 1; \
		} \
	} else \
		length = 0; \
	\
	printf(name ": page=%#0" PRIx32 " frame=%#0" PRIx32 \
	    " length=%#0" PRIx32 " KB (mask=%#0" PRIx32 ")%s%s\n", \
	    upper & UINT32_C(0xffff0000), lower & UINT32_C(0xffff0000), \
	    length, mask, \
	    ((upper >> 1) & 1) ? " supervisor" : "", \
	    (upper & 1) ? " user" : "");

void tlb_print(void)
{
	uint32_t sr;

	for (sr = 0; sr < 16; sr++) {
		uint32_t vsid = sr_get(sr << 28);

		printf("sr[%02" PRIu32 "]: vsid=%#0" PRIx32 " (asid=%" PRIu32 ")"
		    "%s%s\n", sr, vsid & UINT32_C(0x00ffffff),
		    (vsid & UINT32_C(0x00ffffff)) >> 4,
		    ((vsid >> 30) & 1) ? " supervisor" : "",
		    ((vsid >> 29) & 1) ? " user" : "");
	}

	uint32_t upper;
	uint32_t lower;
	uint32_t mask;
	uint32_t length;

	PRINT_BAT("ibat[0]", 528, 529);
	PRINT_BAT("ibat[1]", 530, 531);
	PRINT_BAT("ibat[2]", 532, 533);
	PRINT_BAT("ibat[3]", 534, 535);

	PRINT_BAT("dbat[0]", 536, 537);
	PRINT_BAT("dbat[1]", 538, 539);
	PRINT_BAT("dbat[2]", 540, 541);
	PRINT_BAT("dbat[3]", 542, 543);
}

/** @}
 */
