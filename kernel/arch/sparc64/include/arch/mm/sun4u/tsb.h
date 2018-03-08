/*
 * Copyright (c) 2006 Jakub Jermar
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

#ifndef KERN_sparc64_sun4u_TSB_H_
#define KERN_sparc64_sun4u_TSB_H_

/*
 * ITSB abd DTSB will claim 64K of memory, which
 * is a nice number considered that it is one of
 * the page sizes supported by hardware, which,
 * again, is nice because TSBs need to be locked
 * in TLBs - only one TLB entry will do.
 */
#define TSB_BASE_REG_SIZE	2	/* keep in sync with as.c */
#define ITSB_ENTRY_COUNT	(512 * (1 << TSB_BASE_REG_SIZE))
#define DTSB_ENTRY_COUNT	(512 * (1 << TSB_BASE_REG_SIZE))

#define ITSB_ENTRY_MASK		(ITSB_ENTRY_COUNT - 1)
#define DTSB_ENTRY_MASK		(DTSB_ENTRY_COUNT - 1)

#define TSB_ENTRY_COUNT		(ITSB_ENTRY_COUNT + DTSB_ENTRY_COUNT)
#define TSB_SIZE		(TSB_ENTRY_COUNT * sizeof(tsb_entry_t))
#define TSB_FRAMES		SIZE2FRAMES(TSB_SIZE)

#define TSB_TAG_TARGET_CONTEXT_SHIFT	48

#ifndef __ASSEMBLER__

#include <arch/mm/tte.h>
#include <arch/mm/mmu.h>
#include <typedefs.h>

/** TSB Base register. */
typedef union tsb_base_reg {
	uint64_t value;
	struct {
		uint64_t base : 51;	/**< TSB base address, bits 63:13. */
		unsigned split : 1;	/**< Split vs. common TSB for 8K and 64K
					 * pages. HelenOS uses only 8K pages
					 * for user mappings, so we always set
					 * this to 0.
					 */
		unsigned : 9;
		unsigned size : 3;	/**< TSB size. Number of entries is
					 * 512 * 2^size. */
	} __attribute__ ((packed));
} tsb_base_reg_t;

/** Read ITSB Base register.
 *
 * @return Content of the ITSB Base register.
 */
static inline uint64_t itsb_base_read(void)
{
	return asi_u64_read(ASI_IMMU, VA_IMMU_TSB_BASE);
}

/** Read DTSB Base register.
 *
 * @return Content of the DTSB Base register.
 */
static inline uint64_t dtsb_base_read(void)
{
	return asi_u64_read(ASI_DMMU, VA_DMMU_TSB_BASE);
}

/** Write ITSB Base register.
 *
 * @param v New content of the ITSB Base register.
 */
static inline void itsb_base_write(uint64_t v)
{
	asi_u64_write(ASI_IMMU, VA_IMMU_TSB_BASE, v);
}

/** Write DTSB Base register.
 *
 * @param v New content of the DTSB Base register.
 */
static inline void dtsb_base_write(uint64_t v)
{
	asi_u64_write(ASI_DMMU, VA_DMMU_TSB_BASE, v);
}

#if defined (US3)

/** Write DTSB Primary Extension register.
 *
 * @param v New content of the DTSB Primary Extension register.
 */
static inline void dtsb_primary_extension_write(uint64_t v)
{
	asi_u64_write(ASI_DMMU, VA_DMMU_PRIMARY_EXTENSION, v);
}

/** Write DTSB Secondary Extension register.
 *
 * @param v New content of the DTSB Secondary Extension register.
 */
static inline void dtsb_secondary_extension_write(uint64_t v)
{
	asi_u64_write(ASI_DMMU, VA_DMMU_SECONDARY_EXTENSION, v);
}

/** Write DTSB Nucleus Extension register.
 *
 * @param v New content of the DTSB Nucleus Extension register.
 */
static inline void dtsb_nucleus_extension_write(uint64_t v)
{
	asi_u64_write(ASI_DMMU, VA_DMMU_NUCLEUS_EXTENSION, v);
}

/** Write ITSB Primary Extension register.
 *
 * @param v New content of the ITSB Primary Extension register.
 */
static inline void itsb_primary_extension_write(uint64_t v)
{
	asi_u64_write(ASI_IMMU, VA_IMMU_PRIMARY_EXTENSION, v);
}

/** Write ITSB Nucleus Extension register.
 *
 * @param v New content of the ITSB Nucleus Extension register.
 */
static inline void itsb_nucleus_extension_write(uint64_t v)
{
	asi_u64_write(ASI_IMMU, VA_IMMU_NUCLEUS_EXTENSION, v);
}

#endif

/* Forward declarations. */
struct as;
struct pte;

extern void tsb_invalidate(struct as *as, uintptr_t page, size_t pages);
extern void itsb_pte_copy(struct pte *t, size_t index);
extern void dtsb_pte_copy(struct pte *t, size_t index, bool ro);

#endif /* !def __ASSEMBLER__ */

#endif

/** @}
 */
