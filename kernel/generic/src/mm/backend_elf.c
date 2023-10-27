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

/** @addtogroup kernel_generic_mm
 * @{
 */

/**
 * @file
 * @brief	Backend for address space areas backed by an ELF image.
 */

#include <lib/elf.h>
#include <assert.h>
#include <typedefs.h>
#include <mm/as.h>
#include <mm/frame.h>
#include <mm/slab.h>
#include <mm/page.h>
#include <mm/reserve.h>
#include <mm/km.h>
#include <genarch/mm/page_pt.h>
#include <genarch/mm/page_ht.h>
#include <align.h>
#include <memw.h>
#include <macros.h>
#include <arch.h>
#include <barrier.h>

static bool elf_create(as_area_t *);
static bool elf_resize(as_area_t *, size_t);
static void elf_share(as_area_t *);
static void elf_destroy(as_area_t *);

static bool elf_is_resizable(as_area_t *);
static bool elf_is_shareable(as_area_t *);

static int elf_page_fault(as_area_t *, uintptr_t, pf_access_t);
static void elf_frame_free(as_area_t *, uintptr_t, uintptr_t);

mem_backend_t elf_backend = {
	.create = elf_create,
	.resize = elf_resize,
	.share = elf_share,
	.destroy = elf_destroy,

	.is_resizable = elf_is_resizable,
	.is_shareable = elf_is_shareable,

	.page_fault = elf_page_fault,
	.frame_free = elf_frame_free,

	.create_shared_data = NULL,
	.destroy_shared_data = NULL
};

static size_t elf_nonanon_pages_get(as_area_t *area)
{
	elf_segment_header_t *entry = area->backend_data.segment;
	uintptr_t first = ALIGN_UP(entry->p_vaddr, PAGE_SIZE);
	uintptr_t last = ALIGN_DOWN(entry->p_vaddr + entry->p_filesz,
	    PAGE_SIZE);

	if (entry->p_flags & PF_W)
		return 0;

	if (last < first)
		return 0;

	return last - first;
}

/** Get page number in the task where the ELF page originates from.
 *
 * The ELF page can be shared to a different address than it originated from,
 * but we need the originating address since that corresponds to the ELF's
 * virtual addesses.
 *
 * @param area Area in which the page resides
 * @param page Virtual address of the page in @a area
 * @return Virtual address of the page in the origin address space
 */
static uintptr_t elf_orig_page(as_area_t *area, uintptr_t page)
{
	return page - area->base + area->backend_data.elf_base;
}

bool elf_create(as_area_t *area)
{
	size_t nonanon_pages = elf_nonanon_pages_get(area);

	if (area->pages <= nonanon_pages)
		return true;

	return reserve_try_alloc(area->pages - nonanon_pages);
}

bool elf_resize(as_area_t *area, size_t new_pages)
{
	size_t nonanon_pages = elf_nonanon_pages_get(area);

	if (new_pages > area->pages) {
		/* The area is growing. */
		if (area->pages >= nonanon_pages)
			return reserve_try_alloc(new_pages - area->pages);
		else if (new_pages > nonanon_pages)
			return reserve_try_alloc(new_pages - nonanon_pages);
	} else if (new_pages < area->pages) {
		/* The area is shrinking. */
		if (new_pages >= nonanon_pages)
			reserve_free(area->pages - new_pages);
		else if (area->pages > nonanon_pages)
			reserve_free(nonanon_pages - new_pages);
	}

	return true;
}

/** Share ELF image backed address space area.
 *
 * If the area is writable, then all mapped pages are duplicated in the pagemap.
 * Otherwise only portions of the area that are not backed by the ELF image
 * are put into the pagemap.
 *
 * @param area		Address space area.
 */
void elf_share(as_area_t *area)
{
	elf_segment_header_t *entry = area->backend_data.segment;
	used_space_ival_t *start;
	used_space_ival_t *cur;
	uintptr_t start_anon = entry->p_vaddr + entry->p_filesz;

	assert(mutex_locked(&area->as->lock));
	assert(mutex_locked(&area->lock));

	/*
	 * Find the node in which to start linear search.
	 */
	if (area->flags & AS_AREA_WRITE) {
		start = used_space_first(&area->used_space);
	} else {
		/* Find first interval containing addresses >= start_anon */
		start = used_space_find_gteq(&area->used_space, start_anon);
	}

	/*
	 * Copy used anonymous portions of the area to sh_info's page map.
	 */
	mutex_lock(&area->sh_info->lock);
	cur = start;
	while (cur != NULL) {
		uintptr_t base = cur->page;
		size_t count = cur->count;
		unsigned int i;

		/*
		 * Skip read-only areas of used space that are backed
		 * by the ELF image.
		 */
		if (!(area->flags & AS_AREA_WRITE))
			if (base >= entry->p_vaddr &&
			    base + P2SZ(count) <= start_anon)
				continue;

		for (i = 0; i < count; i++) {
			pte_t pte;
			bool found;

			/*
			 * Skip read-only pages that are backed by the
			 * ELF image.
			 */
			if (!(area->flags & AS_AREA_WRITE))
				if (base >= entry->p_vaddr &&
				    base + P2SZ(i + 1) <= start_anon)
					continue;

			page_table_lock(area->as, false);
			found = page_mapping_find(area->as,
			    base + P2SZ(i), false, &pte);

			(void) found;
			assert(found);
			assert(PTE_VALID(&pte));
			assert(PTE_PRESENT(&pte));

			as_pagemap_insert(&area->sh_info->pagemap,
			    (base + P2SZ(i)) - area->base,
			    PTE_GET_FRAME(&pte));
			page_table_unlock(area->as, false);

			pfn_t pfn = ADDR2PFN(PTE_GET_FRAME(&pte));
			frame_reference_add(pfn);
		}

		cur = used_space_next(cur);
	}

	mutex_unlock(&area->sh_info->lock);
}

void elf_destroy(as_area_t *area)
{
	size_t nonanon_pages = elf_nonanon_pages_get(area);

	if (area->pages > nonanon_pages)
		reserve_free(area->pages - nonanon_pages);
}

bool elf_is_resizable(as_area_t *area)
{
	return true;
}

bool elf_is_shareable(as_area_t *area)
{
	return true;
}

/** Service a page fault in the ELF backend address space area.
 *
 * The address space area and page tables must be already locked.
 *
 * @param area		Pointer to the address space area.
 * @param upage		Faulting virtual page.
 * @param access	Access mode that caused the fault (i.e.
 * 			read/write/exec).
 *
 * @return		AS_PF_FAULT on failure (i.e. page fault) or AS_PF_OK
 * 			on success (i.e. serviced).
 */
int elf_page_fault(as_area_t *area, uintptr_t upage, pf_access_t access)
{
	elf_header_t *elf = area->backend_data.elf;
	elf_segment_header_t *entry = area->backend_data.segment;
	uintptr_t base;
	uintptr_t frame;
	uintptr_t kpage;
	uintptr_t start_anon;
	uintptr_t elfpage;
	size_t i;
	bool dirty = false;

	assert(page_table_locked(AS));
	assert(mutex_locked(&area->lock));
	assert(IS_ALIGNED(upage, PAGE_SIZE));

	elfpage = elf_orig_page(area, upage);

	if (!as_area_check_access(area, access))
		return AS_PF_FAULT;

	if (elfpage < ALIGN_DOWN(entry->p_vaddr, PAGE_SIZE))
		return AS_PF_FAULT;

	if (elfpage >= entry->p_vaddr + entry->p_memsz)
		return AS_PF_FAULT;

	i = (elfpage - ALIGN_DOWN(entry->p_vaddr, PAGE_SIZE)) >>
	    PAGE_WIDTH;
	base = (uintptr_t)
	    (((void *) elf) + ALIGN_DOWN(entry->p_offset, PAGE_SIZE));

	/* Virtual address of the end of initialized part of segment */
	start_anon = entry->p_vaddr + entry->p_filesz;

	mutex_lock(&area->sh_info->lock);
	if (area->sh_info->shared) {
		/*
		 * The address space area is shared.
		 */

		errno_t rc = as_pagemap_find(&area->sh_info->pagemap,
		    upage - area->base, &frame);
		if (rc == EOK) {
			frame_reference_add(ADDR2PFN(frame));
			page_mapping_insert(AS, upage, frame,
			    as_area_get_flags(area));
			if (!used_space_insert(&area->used_space, upage, 1))
				panic("Cannot insert used space.");
			mutex_unlock(&area->sh_info->lock);
			return AS_PF_OK;
		}
	}

	/*
	 * The area is either not shared or the pagemap does not contain the
	 * mapping.
	 */
	if (elfpage >= entry->p_vaddr && elfpage + PAGE_SIZE <= start_anon) {
		/*
		 * Initialized portion of the segment. The memory is backed
		 * directly by the content of the ELF image. Pages are
		 * only copied if the segment is writable so that there
		 * can be more instances of the same memory ELF image
		 * used at a time. Note that this could be later done
		 * as COW.
		 */
		if (entry->p_flags & PF_W) {
			kpage = km_temporary_page_get(&frame, FRAME_NO_RESERVE);
			memcpy((void *) kpage, (void *) (base + i * PAGE_SIZE),
			    PAGE_SIZE);
			if (entry->p_flags & PF_X) {
				smc_coherence((void *) kpage, PAGE_SIZE);
			}
			km_temporary_page_put(kpage);
			dirty = true;
		} else {
			pte_t pte;
			bool found;

			found = page_mapping_find(AS_KERNEL,
			    base + i * FRAME_SIZE, true, &pte);

			(void) found;
			assert(found);
			assert(PTE_PRESENT(&pte));

			frame = PTE_GET_FRAME(&pte);
		}
	} else if (elfpage >= start_anon) {
		/*
		 * This is the uninitialized portion of the segment.
		 * It is not physically present in the ELF image.
		 * To resolve the situation, a frame must be allocated
		 * and cleared.
		 */
		kpage = km_temporary_page_get(&frame, FRAME_NO_RESERVE);
		memsetb((void *) kpage, PAGE_SIZE, 0);
		km_temporary_page_put(kpage);
		dirty = true;
	} else {
		size_t pad_lo, pad_hi;
		/*
		 * The mixed case.
		 *
		 * The middle part is backed by the ELF image and
		 * the lower and upper parts are anonymous memory.
		 * (The segment can be and often is shorter than 1 page).
		 */
		if (upage < entry->p_vaddr)
			pad_lo = entry->p_vaddr - upage;
		else
			pad_lo = 0;

		if (start_anon < upage + PAGE_SIZE)
			pad_hi = upage + PAGE_SIZE - start_anon;
		else
			pad_hi = 0;

		kpage = km_temporary_page_get(&frame, FRAME_NO_RESERVE);
		memcpy((void *) (kpage + pad_lo),
		    (void *) (base + i * PAGE_SIZE + pad_lo),
		    PAGE_SIZE - pad_lo - pad_hi);
		if (entry->p_flags & PF_X) {
			smc_coherence((void *) (kpage + pad_lo),
			    PAGE_SIZE - pad_lo - pad_hi);
		}
		memsetb((void *) kpage, pad_lo, 0);
		memsetb((void *) (kpage + PAGE_SIZE - pad_hi), pad_hi, 0);
		km_temporary_page_put(kpage);
		dirty = true;
	}

	if (dirty && area->sh_info->shared) {
		frame_reference_add(ADDR2PFN(frame));
		as_pagemap_insert(&area->sh_info->pagemap, upage - area->base,
		    frame);
	}

	mutex_unlock(&area->sh_info->lock);

	page_mapping_insert(AS, upage, frame, as_area_get_flags(area));
	if (!used_space_insert(&area->used_space, upage, 1))
		panic("Cannot insert used space.");

	return AS_PF_OK;
}

/** Free a frame that is backed by the ELF backend.
 *
 * The address space area and page tables must be already locked.
 *
 * @param area		Pointer to the address space area.
 * @param page		Page that is mapped to frame. Must be aligned to
 * 			PAGE_SIZE.
 * @param frame		Frame to be released.
 *
 */
void elf_frame_free(as_area_t *area, uintptr_t page, uintptr_t frame)
{
	elf_segment_header_t *entry = area->backend_data.segment;
	uintptr_t start_anon;
	uintptr_t elfpage;

	assert(page_table_locked(area->as));
	assert(mutex_locked(&area->lock));

	elfpage = elf_orig_page(area, page);

	assert(elfpage >= ALIGN_DOWN(entry->p_vaddr, PAGE_SIZE));
	assert(elfpage < entry->p_vaddr + entry->p_memsz);

	start_anon = entry->p_vaddr + entry->p_filesz;

	if (elfpage >= entry->p_vaddr && elfpage + PAGE_SIZE <= start_anon) {
		if (entry->p_flags & PF_W) {
			/*
			 * Free the frame with the copy of writable segment
			 * data.
			 */
			frame_free_noreserve(frame, 1);
		}
	} else {
		/*
		 * The frame is either anonymous memory or the mixed case (i.e.
		 * lower part is backed by the ELF image and the upper is
		 * anonymous). In any case, a frame needs to be freed.
		 */
		frame_free_noreserve(frame, 1);
	}
}

/** @}
 */
