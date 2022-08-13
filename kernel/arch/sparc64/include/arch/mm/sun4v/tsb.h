/*
 * SPDX-FileCopyrightText: 2006 Jakub Jermar
 * SPDX-FileCopyrightText: 2009 Pavel Rimsky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_sparc64_mm
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
