/*
 * Copyright (c) 2011 Jakub Jermar
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

/** @addtogroup kernel_generic_mm
 * @{
 */

/**
 * @file
 * @brief Kernel virtual memory setup.
 */

#include <mm/km.h>
#include <arch/mm/km.h>
#include <assert.h>
#include <mm/page.h>
#include <mm/frame.h>
#include <mm/asid.h>
#include <config.h>
#include <typedefs.h>
#include <lib/ra.h>
#include <arch.h>
#include <align.h>
#include <macros.h>
#include <bitops.h>
#include <proc/thread.h>

static ra_arena_t *km_ni_arena;

#define DEFERRED_PAGES_MAX	(PAGE_SIZE / sizeof(uintptr_t))

/** Number of freed pages in the deferred buffer. */
static volatile unsigned deferred_pages;
/** Buffer of deferred freed pages. */
static uintptr_t deferred_page[DEFERRED_PAGES_MAX];

/** Flush the buffer of deferred freed pages.
 *
 * @return		Number of freed pages.
 */
static unsigned km_flush_deferred(void)
{
	unsigned i = 0;
	ipl_t ipl;

	ipl = tlb_shootdown_start(TLB_INVL_ASID, ASID_KERNEL, 0, 0);

	for (i = 0; i < deferred_pages; i++) {
		page_mapping_remove(AS_KERNEL, deferred_page[i]);
		km_page_free(deferred_page[i], PAGE_SIZE);
	}

	tlb_invalidate_asid(ASID_KERNEL);

	as_invalidate_translation_cache(AS_KERNEL, 0, -1);
	tlb_shootdown_finalize(ipl);

	return i;
}

/** Architecture dependent setup of identity-mapped kernel memory. */
void km_identity_init(void)
{
	km_identity_arch_init();
	config.identity_configured = true;
}

/** Architecture dependent setup of non-identity-mapped kernel memory. */
void km_non_identity_init(void)
{
	km_ni_arena = ra_arena_create();
	assert(km_ni_arena != NULL);
	km_non_identity_arch_init();
	config.non_identity_configured = true;
}

bool km_is_non_identity(uintptr_t addr)
{
	return km_is_non_identity_arch(addr);
}

void km_non_identity_span_add(uintptr_t base, size_t size)
{
	bool span_added;

	page_mapping_make_global(base, size);

	span_added = ra_span_add(km_ni_arena, base, size);
	assert(span_added);
	(void) span_added;
}

uintptr_t km_page_alloc(size_t size, size_t align)
{
	uintptr_t base;
	if (ra_alloc(km_ni_arena, size, align, &base))
		return base;
	else
		panic("Kernel ran out of virtual address space.");
}

void km_page_free(uintptr_t page, size_t size)
{
	ra_free(km_ni_arena, page, size);
}

static uintptr_t
km_map_aligned(uintptr_t paddr, size_t size, size_t align, unsigned int flags)
{
	uintptr_t vaddr;
	uintptr_t offs;

	if (align == KM_NATURAL_ALIGNMENT)
		align = ispwr2(size) ? size : (1U << (fnzb(size) + 1));

	assert(ALIGN_DOWN(paddr, FRAME_SIZE) == paddr);
	assert(ALIGN_UP(size, FRAME_SIZE) == size);
	assert(ispwr2(align));

	/* Enforce at least PAGE_SIZE alignment. */
	vaddr = km_page_alloc(size, max(PAGE_SIZE, align));

	page_table_lock(AS_KERNEL, true);
	for (offs = 0; offs < size; offs += PAGE_SIZE) {
		page_mapping_insert(AS_KERNEL, vaddr + offs, paddr + offs,
		    flags);
	}
	page_table_unlock(AS_KERNEL, true);

	return vaddr;
}

static void km_unmap_aligned(uintptr_t vaddr, size_t size)
{
	uintptr_t offs;
	ipl_t ipl;

	assert(ALIGN_DOWN(vaddr, PAGE_SIZE) == vaddr);
	assert(ALIGN_UP(size, PAGE_SIZE) == size);

	page_table_lock(AS_KERNEL, true);

	size_t pages = size >> PAGE_WIDTH;
	ipl = tlb_shootdown_start(TLB_INVL_PAGES, ASID_KERNEL, vaddr, pages);

	for (offs = 0; offs < size; offs += PAGE_SIZE)
		page_mapping_remove(AS_KERNEL, vaddr + offs);

	tlb_invalidate_pages(ASID_KERNEL, vaddr, pages);

	as_invalidate_translation_cache(AS_KERNEL, 0, -1);
	tlb_shootdown_finalize(ipl);
	page_table_unlock(AS_KERNEL, true);

	km_page_free(vaddr, size);
}

/** Map a piece of physical address space into the virtual address space.
 *
 * @param paddr		Physical address to be mapped. May be unaligned.
 * @param size		Size of area starting at paddr to be mapped.
 * @param flags		Protection flags to be used for the mapping.
 *
 * @return New virtual address mapped to paddr.
 */
uintptr_t km_map(uintptr_t paddr, size_t size, size_t align, unsigned int flags)
{
	uintptr_t page;
	size_t offs;

	offs = paddr - ALIGN_DOWN(paddr, FRAME_SIZE);
	page = km_map_aligned(ALIGN_DOWN(paddr, FRAME_SIZE),
	    ALIGN_UP(size + offs, FRAME_SIZE), align, flags);

	return page + offs;
}

/** Unmap a piece of virtual address space.
 *
 * @param vaddr		Virtual address to be unmapped. May be unaligned, but
 *			it must be a value previously returned by km_map().
 * @param size		Size of area starting at vaddr to be unmapped.
 */
void km_unmap(uintptr_t vaddr, size_t size)
{
	size_t offs;

	offs = vaddr - ALIGN_DOWN(vaddr, PAGE_SIZE);
	km_unmap_aligned(ALIGN_DOWN(vaddr, PAGE_SIZE),
	    ALIGN_UP(size + offs, PAGE_SIZE));
}

/** Unmap kernel non-identity page.
 *
 * @param[in] page	Non-identity page to be unmapped.
 */
static void km_unmap_deferred(uintptr_t page)
{
	page_table_lock(AS_KERNEL, true);

	if (deferred_pages == DEFERRED_PAGES_MAX) {
		(void) km_flush_deferred();
		deferred_pages = 0;
	}

	deferred_page[deferred_pages++] = page;

	page_table_unlock(AS_KERNEL, true);
}

/** Create a temporary page.
 *
 * The page is mapped read/write to a newly allocated frame of physical memory.
 * The page must be returned back to the system by a call to
 * km_temporary_page_put().
 *
 * @param[inout] framep	Pointer to a variable which will receive the physical
 *			address of the allocated frame.
 * @param[in] flags	Frame allocation flags. FRAME_NONE, FRAME_NO_RESERVE
 *			and FRAME_ATOMIC bits are allowed.
 * @return		Virtual address of the allocated frame.
 */
uintptr_t km_temporary_page_get(uintptr_t *framep, frame_flags_t flags)
{
	assert(THREAD);
	assert(framep);
	assert(!(flags & ~(FRAME_NO_RESERVE | FRAME_ATOMIC)));

	/*
	 * Allocate a frame, preferably from high memory.
	 */
	uintptr_t page;
	uintptr_t frame;

	frame = frame_alloc(1, FRAME_HIGHMEM | flags, 0);
	if (frame >= config.identity_size) {
		page = km_map(frame, PAGE_SIZE, PAGE_SIZE,
		    PAGE_READ | PAGE_WRITE | PAGE_CACHEABLE);
	} else {
		page = PA2KA(frame);
	}

	*framep = frame;
	return page;
}

/** Destroy a temporary page.
 *
 * This function destroys a temporary page previously created by
 * km_temporary_page_get(). The page destruction may be immediate or deferred.
 * The frame mapped by the destroyed page is not freed.
 *
 * @param[in] page	Temporary page to be destroyed.
 */
void km_temporary_page_put(uintptr_t page)
{
	assert(THREAD);

	if (km_is_non_identity(page))
		km_unmap_deferred(page);
}

/** @}
 */
