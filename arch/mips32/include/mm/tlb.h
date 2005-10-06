/*
 * Copyright (C) 2003-2004 Jakub Jermar
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

#ifndef __mips32_TLB_H__
#define __mips32_TLB_H__

#include <arch/exception.h>

#define TLB_SIZE	48

#define TLB_WIRED		1
#define TLB_KSTACK_WIRED_INDEX	0

#define TLB_PAGE_MASK_16K	(0x3<<13)

#define PAGE_UNCACHED			2
#define PAGE_CACHEABLE_EXC_WRITE	5

union entry_lo {
	struct {
		unsigned g : 1; 	/* global bit */
		unsigned v : 1; 	/* valid bit */
		unsigned d : 1; 	/* dirty/write-protect bit */
		unsigned c : 3; 	/* cache coherency attribute */
		unsigned pfn : 24;	/* frame number */
		unsigned zero: 2;	/* zero */
	} __attribute__ ((packed));
	__u32 value;
};

struct pte {
	unsigned g : 1; 	/* global bit */
	unsigned v : 1; 	/* valid bit */
	unsigned d : 1; 	/* dirty/write-protect bit */
	unsigned c : 3; 	/* cache coherency attribute */
	unsigned pfn : 24;	/* frame number */
	unsigned w : 1;		/* writable */
	unsigned a : 1;		/* accessed */
} __attribute__ ((packed));

union entry_hi {
	struct {
		unsigned asid : 8;
		unsigned : 5;
		unsigned vpn2 : 19;
	} __attribute__ ((packed));
	__u32 value;
};

union page_mask {
	struct {
		unsigned : 13;
		unsigned mask : 12;
		unsigned : 7;
	} __attribute__ ((packed));
	__u32 value;
};

union index {
	struct {
		unsigned index : 4;
		unsigned : 27;
		unsigned p : 1;
	} __attribute__ ((packed));
	__u32 value;
};

typedef union entry_lo entry_lo_t;
typedef union entry_hi entry_hi_t;
typedef union page_mask page_mask_t;
typedef union index tlb_index_t;

/** Probe TLB for Matching Entry
 *
 * Probe TLB for Matching Entry.
 */
static inline void tlbp(void)
{
	__asm__ volatile ("tlbp\n\t");
}


/** Read Indexed TLB Entry
 *
 * Read Indexed TLB Entry.
 */
static inline void tlbr(void)
{
	__asm__ volatile ("tlbr\n\t");
}

/** Write Indexed TLB Entry
 *
 * Write Indexed TLB Entry.
 */
static inline void tlbwi(void)
{
	__asm__ volatile ("tlbwi\n\t");
}

/** Write Random TLB Entry
 *
 * Write Random TLB Entry.
 */
static inline void tlbwr(void)
{
	__asm__ volatile ("tlbwr\n\t");
}

extern void tlb_invalid(struct exception_regdump *pstate);
extern void tlb_refill(struct exception_regdump *pstate);
extern void tlb_modified(struct exception_regdump *pstate);

#endif
