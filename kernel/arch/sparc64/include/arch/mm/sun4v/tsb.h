/*
 * Copyright (c) 2006 Jakub Jermar
 * Copyright (c) 2009 Pavel Rimsky
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

#ifndef KERN_sparc64_sun4v_TSB_H_
#define KERN_sparc64_sun4v_TSB_H_

/*
 * TSB will claim 64K of memory, which
 * is a nice number considered that it is one of
 * the page sizes supported by hardware, which,
 * again, is nice because TSBs need to be locked
 * in TLBs - only one TLB entry will do.
 */
#define TSB_ENTRY_COUNT			4096
#define TSB_ENTRY_MASK			(TSB_ENTRY_COUNT - 1)
#define TSB_SIZE			(TSB_ENTRY_COUNT * sizeof(tsb_entry_t))
#define TSB_FRAMES			SIZE2FRAMES(TSB_SIZE)

#ifndef __ASSEMBLER__

#include <stdbool.h>
#include <typedefs.h>
#include <arch/mm/tte.h>
#include <arch/mm/mmu.h>

/** TSB description, used in hypercalls */
typedef struct tsb_descr {
	uint16_t page_size;	/**< Page size (0 = 8K, 1 = 64K,...). */
	uint16_t associativity;	/**< TSB associativity (will be 1). */
	uint32_t num_ttes;	/**< Number of TTEs. */
	uint32_t context;	/**< Context number. */
	uint32_t pgsize_mask;	/**< Equals "1 << page_size". */
	uint64_t tsb_base;	/**< Real address of TSB base. */
	uint64_t reserved;
} __attribute__((packed)) tsb_descr_t;


/* Forward declarations. */
struct as;
struct pte;

extern void tsb_invalidate(struct as *as, uintptr_t page, size_t pages);
extern void itsb_pte_copy(struct pte *t);
extern void dtsb_pte_copy(struct pte *t, bool ro);

#endif /* !def __ASSEMBLER__ */

#endif

/** @}
 */
