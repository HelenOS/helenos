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
 * @brief	Backend for address space areas backed by an ELF image.
 */

#include <lib/elf.h>
#include <debug.h>
#include <arch/types.h>
#include <mm/as.h>
#include <mm/frame.h>
#include <mm/slab.h>
#include <mm/page.h>
#include <genarch/mm/page_pt.h>
#include <genarch/mm/page_ht.h>
#include <align.h>
#include <memstr.h>
#include <macros.h>
#include <arch.h>

#ifdef CONFIG_VIRT_IDX_DCACHE
#include <arch/mm/cache.h>
#endif

static int elf_page_fault(as_area_t *area, uintptr_t addr, pf_access_t access);
static void elf_frame_free(as_area_t *area, uintptr_t page, uintptr_t frame);
static void elf_share(as_area_t *area);

mem_backend_t elf_backend = {
	.page_fault = elf_page_fault,
	.frame_free = elf_frame_free,
	.share = elf_share
};

/** Service a page fault in the ELF backend address space area.
 *
 * The address space area and page tables must be already locked.
 *
 * @param area Pointer to the address space area.
 * @param addr Faulting virtual address.
 * @param access Access mode that caused the fault (i.e. read/write/exec).
 *
 * @return AS_PF_FAULT on failure (i.e. page fault) or AS_PF_OK on success (i.e.
 *     serviced).
 */
int elf_page_fault(as_area_t *area, uintptr_t addr, pf_access_t access)
{
	elf_header_t *elf = area->backend_data.elf;
	elf_segment_header_t *entry = area->backend_data.segment;
	btree_node_t *leaf;
	uintptr_t base, frame;
	index_t i;
	bool dirty = false;

	if (!as_area_check_access(area, access))
		return AS_PF_FAULT;

	ASSERT((addr >= entry->p_vaddr) &&
	    (addr < entry->p_vaddr + entry->p_memsz));
	i = (addr - entry->p_vaddr) >> PAGE_WIDTH;
	base = (uintptr_t) (((void *) elf) + entry->p_offset);
	ASSERT(ALIGN_UP(base, FRAME_SIZE) == base);

	if (area->sh_info) {
		bool found = false;

		/*
		 * The address space area is shared.
		 */
		 
		mutex_lock(&area->sh_info->lock);
		frame = (uintptr_t) btree_search(&area->sh_info->pagemap,
			ALIGN_DOWN(addr, PAGE_SIZE) - area->base, &leaf);
		if (!frame) {
			int i;

			/*
			 * Workaround for valid NULL address.
			 */

			for (i = 0; i < leaf->keys; i++) {
				if (leaf->key[i] ==
				    ALIGN_DOWN(addr, PAGE_SIZE)) {
					found = true;
					break;
				}
			}
		}
		if (frame || found) {
			frame_reference_add(ADDR2PFN(frame));
			page_mapping_insert(AS, addr, frame,
			    as_area_get_flags(area));
			if (!used_space_insert(area,
			    ALIGN_DOWN(addr, PAGE_SIZE), 1))
				panic("Could not insert used space.\n");
			mutex_unlock(&area->sh_info->lock);
			return AS_PF_OK;
		}
	}
	
	/*
	 * The area is either not shared or the pagemap does not contain the
	 * mapping.
	 */
	
	if (ALIGN_DOWN(addr, PAGE_SIZE) + PAGE_SIZE <
	    entry->p_vaddr + entry->p_filesz) {
		/*
		 * Initialized portion of the segment. The memory is backed
		 * directly by the content of the ELF image. Pages are
		 * only copied if the segment is writable so that there
		 * can be more instantions of the same memory ELF image
		 * used at a time. Note that this could be later done
		 * as COW.
		 */
		if (entry->p_flags & PF_W) {
			frame = (uintptr_t)frame_alloc(ONE_FRAME, 0);
			memcpy((void *) PA2KA(frame),
			    (void *) (base + i * FRAME_SIZE), FRAME_SIZE);
			dirty = true;

			if (area->sh_info) {
				frame_reference_add(ADDR2PFN(frame));
				btree_insert(&area->sh_info->pagemap,
				    ALIGN_DOWN(addr, PAGE_SIZE) - area->base,
				    (void *) frame, leaf);
			}

		} else {
			frame = KA2PA(base + i*FRAME_SIZE);
		}	
	} else if (ALIGN_DOWN(addr, PAGE_SIZE) >=
	    ALIGN_UP(entry->p_vaddr + entry->p_filesz, PAGE_SIZE)) {
		/*
		 * This is the uninitialized portion of the segment.
		 * It is not physically present in the ELF image.
		 * To resolve the situation, a frame must be allocated
		 * and cleared.
		 */
		frame = (uintptr_t)frame_alloc(ONE_FRAME, 0);
		memsetb(PA2KA(frame), FRAME_SIZE, 0);
		dirty = true;

		if (area->sh_info) {
			frame_reference_add(ADDR2PFN(frame));
			btree_insert(&area->sh_info->pagemap,
			    ALIGN_DOWN(addr, PAGE_SIZE) - area->base,
			    (void *) frame, leaf);
		}

	} else {
		size_t size;
		/*
		 * The mixed case.
		 * The lower part is backed by the ELF image and
		 * the upper part is anonymous memory.
		 */
		size = entry->p_filesz - (i<<PAGE_WIDTH);
		frame = (uintptr_t)frame_alloc(ONE_FRAME, 0);
		memsetb(PA2KA(frame) + size, FRAME_SIZE - size, 0);
		memcpy((void *) PA2KA(frame), (void *) (base + i * FRAME_SIZE),
		    size);
		dirty = true;

		if (area->sh_info) {
			frame_reference_add(ADDR2PFN(frame));
			btree_insert(&area->sh_info->pagemap,
			    ALIGN_DOWN(addr, PAGE_SIZE) - area->base,
			    (void *) frame, leaf);
		}

	}
	
	if (area->sh_info)
		mutex_unlock(&area->sh_info->lock);
	
	page_mapping_insert(AS, addr, frame, as_area_get_flags(area));
	if (!used_space_insert(area, ALIGN_DOWN(addr, PAGE_SIZE), 1))
		panic("Could not insert used space.\n");

#ifdef CONFIG_VIRT_IDX_DCACHE
	if (dirty && PAGE_COLOR(PA2KA(frame)) != PAGE_COLOR(addr)) {
		/*
		 * By writing to the frame using kernel virtual address,
		 * we have created an illegal virtual alias. We now have to
		 * invalidate cachelines belonging to addr on all processors
		 * so that they will be reloaded with the new content on next
		 * read.
		 */
		dcache_flush_frame(addr, frame);
		dcache_shootdown_start(DCACHE_INVL_FRAME, PAGE_COLOR(addr), frame);
		dcache_shootdown_finalize();
	}
#endif

	return AS_PF_OK;
}

/** Free a frame that is backed by the ELF backend.
 *
 * The address space area and page tables must be already locked.
 *
 * @param area Pointer to the address space area.
 * @param page Page that is mapped to frame. Must be aligned to PAGE_SIZE.
 * @param frame Frame to be released.
 *
 */
void elf_frame_free(as_area_t *area, uintptr_t page, uintptr_t frame)
{
	elf_header_t *elf = area->backend_data.elf;
	elf_segment_header_t *entry = area->backend_data.segment;
	uintptr_t base;
	index_t i;
	
	ASSERT((page >= entry->p_vaddr) &&
	    (page < entry->p_vaddr + entry->p_memsz));
	i = (page - entry->p_vaddr) >> PAGE_WIDTH;
	base = (uintptr_t) (((void *) elf) + entry->p_offset);
	ASSERT(ALIGN_UP(base, FRAME_SIZE) == base);
	
	if (page + PAGE_SIZE <
	    ALIGN_UP(entry->p_vaddr + entry->p_filesz, PAGE_SIZE)) {
		if (entry->p_flags & PF_W) {
			/*
			 * Free the frame with the copy of writable segment
			 * data.
			 */
			frame_free(frame);
		}
	} else {
		/*
		 * The frame is either anonymous memory or the mixed case (i.e.
		 * lower part is backed by the ELF image and the upper is
		 * anonymous). In any case, a frame needs to be freed.
		 */ 
		frame_free(frame);
	}
}

/** Share ELF image backed address space area.
 *
 * If the area is writable, then all mapped pages are duplicated in the pagemap.
 * Otherwise only portions of the area that are not backed by the ELF image
 * are put into the pagemap.
 *
 * The address space and address space area must be locked prior to the call.
 *
 * @param area Address space area.
 */
void elf_share(as_area_t *area)
{
	elf_segment_header_t *entry = area->backend_data.segment;
	link_t *cur;
	btree_node_t *leaf, *node;
	uintptr_t start_anon = entry->p_vaddr + entry->p_filesz;

	/*
	 * Find the node in which to start linear search.
	 */
	if (area->flags & AS_AREA_WRITE) {
		node = list_get_instance(area->used_space.leaf_head.next,
		    btree_node_t, leaf_link);
	} else {
		(void) btree_search(&area->sh_info->pagemap, start_anon, &leaf);
		node = btree_leaf_node_left_neighbour(&area->sh_info->pagemap,
		    leaf);
		if (!node)
			node = leaf;
	}

	/*
	 * Copy used anonymous portions of the area to sh_info's page map.
	 */
	mutex_lock(&area->sh_info->lock);
	for (cur = &node->leaf_link; cur != &area->used_space.leaf_head;
	    cur = cur->next) {
		int i;
		
		node = list_get_instance(cur, btree_node_t, leaf_link);
		
		for (i = 0; i < node->keys; i++) {
			uintptr_t base = node->key[i];
			count_t count = (count_t) node->value[i];
			int j;
			
			/*
			 * Skip read-only areas of used space that are backed
			 * by the ELF image.
			 */
			if (!(area->flags & AS_AREA_WRITE))
				if (base + count*PAGE_SIZE <= start_anon)
					continue;
			
			for (j = 0; j < count; j++) {
				pte_t *pte;
			
				/*
				 * Skip read-only pages that are backed by the
				 * ELF image.
				 */
				if (!(area->flags & AS_AREA_WRITE))
					if (base + (j + 1) * PAGE_SIZE <=
					    start_anon)
						continue;
				
				page_table_lock(area->as, false);
				pte = page_mapping_find(area->as,
				    base + j * PAGE_SIZE);
				ASSERT(pte && PTE_VALID(pte) &&
				    PTE_PRESENT(pte));
				btree_insert(&area->sh_info->pagemap,
				    (base + j * PAGE_SIZE) - area->base,
					(void *) PTE_GET_FRAME(pte), NULL);
				page_table_unlock(area->as, false);

				pfn_t pfn = ADDR2PFN(PTE_GET_FRAME(pte));
				frame_reference_add(pfn);
			}
				
		}
	}
	mutex_unlock(&area->sh_info->lock);
}

/** @}
 */

