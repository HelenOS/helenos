/*
 * Copyright (C) 2006 Jakub Jermar
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

/**
 * @file	backend_elf.c
 * @brief	Backend for address space areas backed by an ELF image.
 */

#include <elf.h>
#include <debug.h>
#include <arch/types.h>
#include <typedefs.h>
#include <mm/as.h>
#include <mm/frame.h>
#include <mm/slab.h>
#include <align.h>
#include <memstr.h>
#include <macros.h>
#include <arch.h>

static int elf_page_fault(as_area_t *area, __address addr, pf_access_t access);
static void elf_frame_free(as_area_t *area, __address page, __address frame);

mem_backend_t elf_backend = {
	.page_fault = elf_page_fault,
	.frame_free = elf_frame_free,
	.share = NULL
};

/** Service a page fault in the ELF backend address space area.
 *
 * The address space area and page tables must be already locked.
 *
 * @param area Pointer to the address space area.
 * @param addr Faulting virtual address.
 * @param access Access mode that caused the fault (i.e. read/write/exec).
 *
 * @return AS_PF_FAULT on failure (i.e. page fault) or AS_PF_OK on success (i.e. serviced).
 */
int elf_page_fault(as_area_t *area, __address addr, pf_access_t access)
{
	elf_header_t *elf = area->backend_data.elf;
	elf_segment_header_t *entry = area->backend_data.segment;
	__address base, frame;
	index_t i;

	if (!as_area_check_access(area, access))
		return AS_PF_FAULT;

	ASSERT((addr >= entry->p_vaddr) && (addr < entry->p_vaddr + entry->p_memsz));
	i = (addr - entry->p_vaddr) >> PAGE_WIDTH;
	base = (__address) (((void *) elf) + entry->p_offset);
	ASSERT(ALIGN_UP(base, FRAME_SIZE) == base);
	
	if (ALIGN_DOWN(addr, PAGE_SIZE) + PAGE_SIZE < entry->p_vaddr + entry->p_filesz) {
		/*
		 * Initialized portion of the segment. The memory is backed
		 * directly by the content of the ELF image. Pages are
		 * only copied if the segment is writable so that there
		 * can be more instantions of the same memory ELF image
		 * used at a time. Note that this could be later done
		 * as COW.
		 */
		if (entry->p_flags & PF_W) {
			frame = PFN2ADDR(frame_alloc(ONE_FRAME, 0));
			memcpy((void *) PA2KA(frame), (void *) (base + i*FRAME_SIZE), FRAME_SIZE);
		} else {
			frame = KA2PA(base + i*FRAME_SIZE);
		}	
	} else if (ALIGN_DOWN(addr, PAGE_SIZE) >= ALIGN_UP(entry->p_vaddr + entry->p_filesz, PAGE_SIZE)) {
		/*
		 * This is the uninitialized portion of the segment.
		 * It is not physically present in the ELF image.
		 * To resolve the situation, a frame must be allocated
		 * and cleared.
		 */
		frame = PFN2ADDR(frame_alloc(ONE_FRAME, 0));
		memsetb(PA2KA(frame), FRAME_SIZE, 0);
	} else {
		size_t size;
		/*
		 * The mixed case.
		 * The lower part is backed by the ELF image and
		 * the upper part is anonymous memory.
		 */
		size = entry->p_filesz - (i<<PAGE_WIDTH);
		frame = PFN2ADDR(frame_alloc(ONE_FRAME, 0));
		memsetb(PA2KA(frame) + size, FRAME_SIZE - size, 0);
		memcpy((void *) PA2KA(frame), (void *) (base + i*FRAME_SIZE), size);
	}
	
	page_mapping_insert(AS, addr, frame, as_area_get_flags(area));
        if (!used_space_insert(area, ALIGN_DOWN(addr, PAGE_SIZE), 1))
                panic("Could not insert used space.\n");

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
void elf_frame_free(as_area_t *area, __address page, __address frame)
{
	elf_header_t *elf = area->backend_data.elf;
	elf_segment_header_t *entry = area->backend_data.segment;
	__address base;
	index_t i;
	
	ASSERT((page >= entry->p_vaddr) && (page < entry->p_vaddr + entry->p_memsz));
	i = (page - entry->p_vaddr) >> PAGE_WIDTH;
	base = (__address) (((void *) elf) + entry->p_offset);
	ASSERT(ALIGN_UP(base, FRAME_SIZE) == base);
	
	if (page + PAGE_SIZE < ALIGN_UP(entry->p_vaddr + entry->p_filesz, PAGE_SIZE)) {
		if (entry->p_flags & PF_W) {
			/*
			 * Free the frame with the copy of writable segment data.
			 */
			frame_free(ADDR2PFN(frame));
		}
	} else {
		/*
		 * The frame is either anonymous memory or the mixed case (i.e. lower
		 * part is backed by the ELF image and the upper is anonymous).
		 * In any case, a frame needs to be freed.
		 */
		frame_free(ADDR2PFN(frame)); 
	}
}
