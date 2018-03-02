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

/** @addtogroup genericmm
 * @{
 */

/**
 * @file
 * @brief	Backend for anonymous memory address space areas.
 *
 */

#include <assert.h>
#include <mm/as.h>
#include <mm/page.h>
#include <mm/reserve.h>
#include <genarch/mm/page_pt.h>
#include <genarch/mm/page_ht.h>
#include <mm/frame.h>
#include <mm/slab.h>
#include <mm/km.h>
#include <synch/mutex.h>
#include <adt/list.h>
#include <adt/btree.h>
#include <errno.h>
#include <typedefs.h>
#include <align.h>
#include <mem.h>
#include <arch.h>

static bool anon_create(as_area_t *);
static bool anon_resize(as_area_t *, size_t);
static void anon_share(as_area_t *);
static void anon_destroy(as_area_t *);

static bool anon_is_resizable(as_area_t *);
static bool anon_is_shareable(as_area_t *);

static int anon_page_fault(as_area_t *, uintptr_t, pf_access_t);
static void anon_frame_free(as_area_t *, uintptr_t, uintptr_t);

mem_backend_t anon_backend = {
	.create = anon_create,
	.resize = anon_resize,
	.share = anon_share,
	.destroy = anon_destroy,

	.is_resizable = anon_is_resizable,
	.is_shareable = anon_is_shareable,

	.page_fault = anon_page_fault,
	.frame_free = anon_frame_free,

	.create_shared_data = NULL,
	.destroy_shared_data = NULL
};

bool anon_create(as_area_t *area)
{
	if (area->flags & AS_AREA_LATE_RESERVE)
		return true;

	return reserve_try_alloc(area->pages);
}

bool anon_resize(as_area_t *area, size_t new_pages)
{
	if (area->flags & AS_AREA_LATE_RESERVE)
		return true;

	if (new_pages > area->pages)
		return reserve_try_alloc(new_pages - area->pages);
	else if (new_pages < area->pages)
		reserve_free(area->pages - new_pages);

	return true;
}

/** Share the anonymous address space area.
 *
 * Sharing of anonymous area is done by duplicating its entire mapping
 * to the pagemap. Page faults will primarily search for frames there.
 *
 * The address space and address space area must be already locked.
 *
 * @param area Address space area to be shared.
 */
void anon_share(as_area_t *area)
{
	assert(mutex_locked(&area->as->lock));
	assert(mutex_locked(&area->lock));
	assert(!(area->flags & AS_AREA_LATE_RESERVE));

	/*
	 * Copy used portions of the area to sh_info's page map.
	 */
	mutex_lock(&area->sh_info->lock);
	list_foreach(area->used_space.leaf_list, leaf_link, btree_node_t,
	    node) {
		unsigned int i;

		for (i = 0; i < node->keys; i++) {
			uintptr_t base = node->key[i];
			size_t count = (size_t) node->value[i];
			unsigned int j;

			for (j = 0; j < count; j++) {
				pte_t pte;
				bool found;

				page_table_lock(area->as, false);
				found = page_mapping_find(area->as,
				    base + P2SZ(j), false, &pte);

				assert(found);
				assert(PTE_VALID(&pte));
				assert(PTE_PRESENT(&pte));

				btree_insert(&area->sh_info->pagemap,
				    (base + P2SZ(j)) - area->base,
				    (void *) PTE_GET_FRAME(&pte), NULL);
				page_table_unlock(area->as, false);

				pfn_t pfn = ADDR2PFN(PTE_GET_FRAME(&pte));
				frame_reference_add(pfn);
			}

		}
	}
	mutex_unlock(&area->sh_info->lock);
}

void anon_destroy(as_area_t *area)
{
	if (area->flags & AS_AREA_LATE_RESERVE)
		return;

	reserve_free(area->pages);
}

bool anon_is_resizable(as_area_t *area)
{
	return true;
}

bool anon_is_shareable(as_area_t *area)
{
	return !(area->flags & AS_AREA_LATE_RESERVE);
}

/** Service a page fault in the anonymous memory address space area.
 *
 * The address space area and page tables must be already locked.
 *
 * @param area Pointer to the address space area.
 * @param upage Faulting virtual page.
 * @param access Access mode that caused the fault (i.e. read/write/exec).
 *
 * @return AS_PF_FAULT on failure (i.e. page fault) or AS_PF_OK on success (i.e.
 *     serviced).
 */
int anon_page_fault(as_area_t *area, uintptr_t upage, pf_access_t access)
{
	uintptr_t kpage;
	uintptr_t frame;

	assert(page_table_locked(AS));
	assert(mutex_locked(&area->lock));
	assert(IS_ALIGNED(upage, PAGE_SIZE));

	if (!as_area_check_access(area, access))
		return AS_PF_FAULT;

	mutex_lock(&area->sh_info->lock);
	if (area->sh_info->shared) {
		btree_node_t *leaf;

		/*
		 * The area is shared, chances are that the mapping can be found
		 * in the pagemap of the address space area share info
		 * structure.
		 * In the case that the pagemap does not contain the respective
		 * mapping, a new frame is allocated and the mapping is created.
		 */
		frame = (uintptr_t) btree_search(&area->sh_info->pagemap,
		    upage - area->base, &leaf);
		if (!frame) {
			bool allocate = true;
			unsigned int i;

			/*
			 * Zero can be returned as a valid frame address.
			 * Just a small workaround.
			 */
			for (i = 0; i < leaf->keys; i++) {
				if (leaf->key[i] == upage - area->base) {
					allocate = false;
					break;
				}
			}
			if (allocate) {
				kpage = km_temporary_page_get(&frame,
				    FRAME_NO_RESERVE);
				memsetb((void *) kpage, PAGE_SIZE, 0);
				km_temporary_page_put(kpage);

				/*
				 * Insert the address of the newly allocated
				 * frame to the pagemap.
				 */
				btree_insert(&area->sh_info->pagemap,
				    upage - area->base, (void *) frame, leaf);
			}
		}
		frame_reference_add(ADDR2PFN(frame));
	} else {

		/*
		 * In general, there can be several reasons that
		 * can have caused this fault.
		 *
		 * - non-existent mapping: the area is an anonymous
		 *   area (e.g. heap or stack) and so far has not been
		 *   allocated a frame for the faulting page
		 *
		 * - non-present mapping: another possibility,
		 *   currently not implemented, would be frame
		 *   reuse; when this becomes a possibility,
		 *   do not forget to distinguish between
		 *   the different causes
		 */

		if (area->flags & AS_AREA_LATE_RESERVE) {
			/*
			 * Reserve the memory for this page now.
			 */
			if (!reserve_try_alloc(1)) {
				mutex_unlock(&area->sh_info->lock);
				return AS_PF_SILENT;
			}
		}

		kpage = km_temporary_page_get(&frame, FRAME_NO_RESERVE);
		memsetb((void *) kpage, PAGE_SIZE, 0);
		km_temporary_page_put(kpage);
	}
	mutex_unlock(&area->sh_info->lock);

	/*
	 * Map 'upage' to 'frame'.
	 * Note that TLB shootdown is not attempted as only new information is
	 * being inserted into page tables.
	 */
	page_mapping_insert(AS, upage, frame, as_area_get_flags(area));
	if (!used_space_insert(area, upage, 1))
		panic("Cannot insert used space.");

	return AS_PF_OK;
}

/** Free a frame that is backed by the anonymous memory backend.
 *
 * The address space area and page tables must be already locked.
 *
 * @param area Ignored.
 * @param page Virtual address of the page corresponding to the frame.
 * @param frame Frame to be released.
 */
void anon_frame_free(as_area_t *area, uintptr_t page, uintptr_t frame)
{
	assert(page_table_locked(area->as));
	assert(mutex_locked(&area->lock));

	if (area->flags & AS_AREA_LATE_RESERVE) {
		/*
		 * In case of the late reserve areas, physical memory will not
		 * be unreserved when the area is destroyed so we need to use
		 * the normal unreserving frame_free().
		 */
		frame_free(frame, 1);
	} else {
		/*
		 * The reserve will be given back when the area is destroyed or
		 * resized, so use the frame_free_noreserve() which does not
		 * manipulate the reserve or it would be given back twice.
		 */
		frame_free_noreserve(frame, 1);
	}
}

/** @}
 */
