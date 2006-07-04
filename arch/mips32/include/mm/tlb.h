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

/** @addtogroup mips32mm	
 * @{
 */
/** @file
 */

#ifndef __mips32_TLB_H__
#define __mips32_TLB_H__

#include <arch/exception.h>
#include <typedefs.h>

#ifdef TLBCNT
#	define TLB_ENTRY_COUNT		TLBCNT
#else
#	define TLB_ENTRY_COUNT		48
#endif

#define TLB_WIRED		1
#define TLB_KSTACK_WIRED_INDEX	0

#define TLB_PAGE_MASK_16K	(0x3<<13)

#define PAGE_UNCACHED			2
#define PAGE_CACHEABLE_EXC_WRITE	5

typedef union entry_lo entry_lo_t;
typedef union entry_hi entry_hi_t;
typedef union page_mask page_mask_t;
typedef union index tlb_index_t;

union entry_lo {
	struct {
#ifdef BIG_ENDIAN
		unsigned : 2;		/* zero */
		unsigned pfn : 24;	/* frame number */
		unsigned c : 3; 	/* cache coherency attribute */
		unsigned d : 1; 	/* dirty/write-protect bit */
		unsigned v : 1; 	/* valid bit */
		unsigned g : 1; 	/* global bit */
#else
		unsigned g : 1; 	/* global bit */
		unsigned v : 1; 	/* valid bit */
		unsigned d : 1; 	/* dirty/write-protect bit */
		unsigned c : 3; 	/* cache coherency attribute */
		unsigned pfn : 24;	/* frame number */
		unsigned : 2;		/* zero */
#endif
	} __attribute__ ((packed));
	uint32_t value;
};

/** Page Table Entry. */
struct pte {
	unsigned g : 1;			/**< Global bit. */
	unsigned p : 1;			/**< Present bit. */
	unsigned d : 1;			/**< Dirty bit. */
	unsigned cacheable : 1;		/**< Cacheable bit. */
	unsigned : 1;			/**< Unused. */
	unsigned soft_valid : 1;	/**< Valid content even if not present. */
	unsigned pfn : 24;		/**< Physical frame number. */
	unsigned w : 1;			/**< Page writable bit. */
	unsigned a : 1;			/**< Accessed bit. */
};

union entry_hi {
	struct {
#ifdef BIG_ENDIAN
		unsigned vpn2 : 19;
		unsigned : 5;
		unsigned asid : 8;
#else
		unsigned asid : 8;
		unsigned : 5;
		unsigned vpn2 : 19;
#endif
	} __attribute__ ((packed));
	uint32_t value;
};

union page_mask {
	struct {
#ifdef BIG_ENDIAN
		unsigned : 7;
		unsigned mask : 12;
		unsigned : 13;
#else
		unsigned : 13;
		unsigned mask : 12;
		unsigned : 7;
#endif
	} __attribute__ ((packed));
	uint32_t value;
};

union index {
	struct {
#ifdef BIG_ENDIAN
		unsigned p : 1;
		unsigned : 27;
		unsigned index : 4;
#else
		unsigned index : 4;
		unsigned : 27;
		unsigned p : 1;
#endif
	} __attribute__ ((packed));
	uint32_t value;
};

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

#define tlb_invalidate(asid)	tlb_invalidate_asid(asid)

extern void tlb_invalid(istate_t *istate);
extern void tlb_refill(istate_t *istate);
extern void tlb_modified(istate_t *istate);

#endif

/** @}
 */
