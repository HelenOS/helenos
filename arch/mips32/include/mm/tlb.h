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

#define PAGE_UNCACHED			2
#define PAGE_CACHEABLE_EXC_WRITE	5

struct entry_lo {
	unsigned g : 1; 	/* global bit */
	unsigned v : 1; 	/* valid bit */
	unsigned d : 1; 	/* dirty/write-protect bit */
	unsigned c : 3; 	/* cache coherency attribute */
	unsigned pfn : 24;	/* frame number */
	unsigned : 2;
} __attribute__ ((packed));

struct entry_hi {
	unsigned asid : 8;
	unsigned : 4;
	unsigned g : 1;
	unsigned vpn2 : 19;
} __attribute__ ((packed));

struct page_mask {
	unsigned : 13;
	unsigned mask : 12;
	unsigned : 7;
} __attribute__ ((packed));

struct tlb_entry {
	struct entry_lo lo0;
	struct entry_lo lo1;
	struct entry_hi hi;
	struct page_mask mask;
} __attribute__ ((packed));

typedef struct entry_lo pte_t;

extern void tlb_invalid(struct exception_regdump *pstate);
extern void tlb_refill(struct exception_regdump *pstate);

#endif
