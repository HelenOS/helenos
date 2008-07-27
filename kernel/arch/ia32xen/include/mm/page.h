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

/** @addtogroup ia32xen_mm	
 * @{
 */
/** @file
 */

#ifndef KERN_ia32xen_PAGE_H_
#define KERN_ia32xen_PAGE_H_

#include <arch/mm/frame.h>

#define PAGE_WIDTH	FRAME_WIDTH
#define PAGE_SIZE	FRAME_SIZE

#ifdef KERNEL

#ifndef __ASM__
#	define KA2PA(x)	(((uintptr_t) (x)) - 0x80000000)
#	define PA2KA(x)	(((uintptr_t) (x)) + 0x80000000)
#else
#	define KA2PA(x)	((x) - 0x80000000)
#	define PA2KA(x)	((x) + 0x80000000)
#endif

/*
 * Implementation of generic 4-level page table interface.
 * IA-32 has 2-level page tables, so PTL1 and PTL2 are left out.
 */

/* Number of entries in each level. */
#define PTL0_ENTRIES_ARCH	1024
#define PTL1_ENTRIES_ARCH	0
#define PTL2_ENTRIES_ARCH	0
#define PTL3_ENTRIES_ARCH	1024

/* Page table size for each level. */
#define PTL0_SIZE_ARCH		ONE_FRAME
#define PTL1_SIZE_ARCH		0
#define PTL2_SIZE_ARCH		0
#define PTL3_SIZE_ARCH		ONE_FRAME

/* Macros calculating indices into page tables in each level. */
#define PTL0_INDEX_ARCH(vaddr)	(((vaddr) >> 22) & 0x3ff)
#define PTL1_INDEX_ARCH(vaddr)	0
#define PTL2_INDEX_ARCH(vaddr)	0
#define PTL3_INDEX_ARCH(vaddr)	(((vaddr) >> 12) & 0x3ff)

/* Get PTE address accessors for each level. */
#define GET_PTL1_ADDRESS_ARCH(ptl0, i) \
	((pte_t *) MA2PA((((pte_t *) (ptl0))[(i)].frame_address) << 12))
#define GET_PTL2_ADDRESS_ARCH(ptl1, i) \
	(ptl1)
#define GET_PTL3_ADDRESS_ARCH(ptl2, i) \
	(ptl2)
#define GET_FRAME_ADDRESS_ARCH(ptl3, i) \
	((uintptr_t) MA2PA((((pte_t *) (ptl3))[(i)].frame_address) << 12))

/* Set PTE address accessors for each level. */
#define SET_PTL0_ADDRESS_ARCH(ptl0) \
{ \
	mmuext_op_t mmu_ext; \
	\
	mmu_ext.cmd = MMUEXT_NEW_BASEPTR; \
	mmu_ext.mfn = ADDR2PFN(PA2MA(ptl0)); \
	ASSERT(xen_mmuext_op(&mmu_ext, 1, NULL, DOMID_SELF) == 0); \
}

#define SET_PTL1_ADDRESS_ARCH(ptl0, i, a) \
{ \
	mmuext_op_t mmu_ext; \
	\
	mmu_ext.cmd = MMUEXT_PIN_L1_TABLE; \
	mmu_ext.mfn = ADDR2PFN(PA2MA(a)); \
	ASSERT(xen_mmuext_op(&mmu_ext, 1, NULL, DOMID_SELF) == 0); \
	\
	mmu_update_t update; \
	\
	update.ptr = PA2MA(KA2PA(&((pte_t *) (ptl0))[(i)])); \
	update.val = PA2MA(a); \
	ASSERT(xen_mmu_update(&update, 1, NULL, DOMID_SELF) == 0); \
}

#define SET_PTL2_ADDRESS_ARCH(ptl1, i, a)
#define SET_PTL3_ADDRESS_ARCH(ptl2, i, a)
#define SET_FRAME_ADDRESS_ARCH(ptl3, i, a) \
{ \
	mmu_update_t update; \
	\
	update.ptr = PA2MA(KA2PA(&((pte_t *) (ptl3))[(i)])); \
	update.val = PA2MA(a); \
	ASSERT(xen_mmu_update(&update, 1, NULL, DOMID_SELF) == 0); \
}

/* Get PTE flags accessors for each level. */
#define GET_PTL1_FLAGS_ARCH(ptl0, i) \
	get_pt_flags((pte_t *) (ptl0), (index_t) (i))
#define GET_PTL2_FLAGS_ARCH(ptl1, i) \
	PAGE_PRESENT
#define GET_PTL3_FLAGS_ARCH(ptl2, i) \
	PAGE_PRESENT
#define GET_FRAME_FLAGS_ARCH(ptl3, i) \
	get_pt_flags((pte_t *) (ptl3), (index_t) (i))

/* Set PTE flags accessors for each level. */
#define SET_PTL1_FLAGS_ARCH(ptl0, i, x) \
	set_pt_flags((pte_t *) (ptl0), (index_t) (i), (x))
#define SET_PTL2_FLAGS_ARCH(ptl1, i, x)
#define SET_PTL3_FLAGS_ARCH(ptl2, i, x)
#define SET_FRAME_FLAGS_ARCH(ptl3, i, x) \
	set_pt_flags((pte_t *) (ptl3), (index_t) (i), (x))

/* Query macros for the last level. */
#define PTE_VALID_ARCH(p) \
	(*((uint32_t *) (p)) != 0)
#define PTE_PRESENT_ARCH(p) \
	((p)->present != 0)
#define PTE_GET_FRAME_ARCH(p) \
	((p)->frame_address << FRAME_WIDTH)
#define PTE_WRITABLE_ARCH(p) \
	((p)->writeable != 0)
#define PTE_EXECUTABLE_ARCH(p) \
	1

#ifndef __ASM__

#include <mm/mm.h>
#include <arch/hypercall.h>
#include <arch/interrupt.h>

/* Page fault error codes. */

/** When bit on this position is 0, the page fault was caused by a not-present
 * page.
 */
#define PFERR_CODE_P		(1 << 0)

/** When bit on this position is 1, the page fault was caused by a write. */
#define PFERR_CODE_RW		(1 << 1)

/** When bit on this position is 1, the page fault was caused in user mode. */
#define PFERR_CODE_US		(1 << 2)

/** When bit on this position is 1, a reserved bit was set in page directory. */ 
#define PFERR_CODE_RSVD		(1 << 3)

typedef struct {
	uint64_t ptr;      /**< Machine address of PTE */
	union {            /**< New contents of PTE */
		uint64_t val;
		pte_t pte;
	};
} mmu_update_t;

typedef struct {
	unsigned int cmd;
	union {
		unsigned long mfn;
		unsigned long linear_addr;
	};
	union {
		unsigned int nr_ents;
		void *vcpumask;
	};
} mmuext_op_t;

static inline int xen_update_va_mapping(const void *va, const pte_t pte,
    const unsigned int flags)
{
	return hypercall4(XEN_UPDATE_VA_MAPPING, va, pte, 0, flags);
}

static inline int xen_mmu_update(const mmu_update_t *req,
    const unsigned int count, unsigned int *success_count, domid_t domid)
{
	return hypercall4(XEN_MMU_UPDATE, req, count, success_count, domid);
}

static inline int xen_mmuext_op(const mmuext_op_t *op, const unsigned int count,
    unsigned int *success_count, domid_t domid)
{
	return hypercall4(XEN_MMUEXT_OP, op, count, success_count, domid);
}

static inline int get_pt_flags(pte_t *pt, index_t i)
{
	pte_t *p = &pt[i];
	
	return ((!p->page_cache_disable) << PAGE_CACHEABLE_SHIFT |
	    (!p->present) << PAGE_PRESENT_SHIFT |
	    p->uaccessible << PAGE_USER_SHIFT |
	    1 << PAGE_READ_SHIFT |
	    p->writeable << PAGE_WRITE_SHIFT |
	    1 << PAGE_EXEC_SHIFT |
	    p->global << PAGE_GLOBAL_SHIFT);
}

static inline void set_pt_flags(pte_t *pt, index_t i, int flags)
{
	pte_t p = pt[i];
	
	p.page_cache_disable = !(flags & PAGE_CACHEABLE);
	p.present = !(flags & PAGE_NOT_PRESENT);
	p.uaccessible = (flags & PAGE_USER) != 0;
	p.writeable = (flags & PAGE_WRITE) != 0;
	p.global = (flags & PAGE_GLOBAL) != 0;
	
	/*
	 * Ensure that there is at least one bit set even if the present bit is cleared.
	 */
	p.soft_valid = true;
	
	mmu_update_t update;
	
	update.ptr = PA2MA(KA2PA(&(pt[i])));
	update.pte = p;
	xen_mmu_update(&update, 1, NULL, DOMID_SELF);
}

extern void page_arch_init(void);
extern void page_fault(int n, istate_t *istate);

#endif /* __ASM__ */

#endif /* KERNEL */

#endif

/** @}
 */
