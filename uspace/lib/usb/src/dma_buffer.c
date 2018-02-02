/*
 * Copyright (c) 2017 Ondrej Hlavaty <aearsis@eideo.cz>
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
/**  @addtogroup libusb
 * @{
 */
/** @file
 */

#include <align.h>
#include <as.h>
#include <ddi.h>
#include <stddef.h>

#include "usb/dma_buffer.h"

/**
 * Allocate a DMA buffer.
 *
 * XXX: Currently cannot make much use of missing constraints, as it always
 * allocates page-aligned contiguous buffer. We rely on it in dma_buffer_phys.
 *
 * @param[in] db dma_buffer_t structure to fill
 * @param[in] size Size of the required memory space
 * @param[in] policy dma_policy_t flags to guide the allocation
 * @return Error code.
 */
errno_t dma_buffer_alloc_policy(dma_buffer_t *db, size_t size, dma_policy_t policy)
{
	assert(db);

	const size_t real_size = ALIGN_UP(size, PAGE_SIZE);
	const bool need_4gib = !!(policy & DMA_POLICY_4GiB);

	const uintptr_t flags = need_4gib ? DMAMEM_4GiB : 0;

	uintptr_t phys;
	void *address = AS_AREA_ANY;

	const int ret = dmamem_map_anonymous(real_size,
	    flags, AS_AREA_READ | AS_AREA_WRITE, 0,
	    &phys, &address);

	if (ret == EOK) {
		/* Access the pages to force mapping */
		volatile char *buf = address;
		for (size_t i = 0; i < size; i += PAGE_SIZE)
			buf[i] = 0xff;

		db->virt = address;
		db->phys = phys;
	}
	return ret;
}


/**
 * Allocate a DMA buffer using the default policy.
 *
 * @param[in] db dma_buffer_t structure to fill
 * @param[in] size Size of the required memory space
 * @return Error code.
 */
errno_t dma_buffer_alloc(dma_buffer_t *db, size_t size)
{
	return dma_buffer_alloc_policy(db, size, DMA_POLICY_DEFAULT);
}


/**
 * Free a DMA buffer.
 *
 * @param[in] db dma_buffer_t structure buffer of which will be freed
 */
void dma_buffer_free(dma_buffer_t *db)
{
	if (db->virt) {
		dmamem_unmap_anonymous(db->virt);
		db->virt = NULL;
		db->phys = 0;
	}
}

/**
 * Convert a pointer inside a buffer to physical address.
 *
 * @param[in] db Buffer at which virt is pointing
 * @param[in] virt Pointer somewhere inside db
 */
uintptr_t dma_buffer_phys(const dma_buffer_t *db, void *virt)
{
	return db->phys + (virt - db->virt);
}

/**
 * Check whether a memory area is compatible with a policy.
 *
 * Useful to skip copying when the buffer is already ready to be given to
 * hardware as is.
 *
 * Note that the "as_get_physical_mapping" fails when the page is not mapped
 * yet, and that the caller is responsible for forcing the mapping.
 */
bool dma_buffer_check_policy(const void *buffer, size_t size, const dma_policy_t policy)
{
	uintptr_t addr = (uintptr_t) buffer;

	const bool check_4gib       = !!(policy & DMA_POLICY_4GiB);
	const bool check_crossing   = !!(policy & DMA_POLICY_NOT_CROSSING);
	const bool check_alignment  = !!(policy & DMA_POLICY_PAGE_ALIGNED);
	const bool check_contiguous = !!(policy & DMA_POLICY_CONTIGUOUS);

	/* Check the two conditions that are easy */
	if (check_crossing && (addr + size - 1) / PAGE_SIZE != addr / PAGE_SIZE)
		goto violated;

	if (check_alignment && ((uintptr_t) buffer) % PAGE_SIZE)
		goto violated;

	/*
	 * For these conditions, we need to walk through pages and check
	 * physical address of each one
	 */
	if (check_contiguous || check_4gib) {
		const void *virt = buffer;
		uintptr_t phys;

		/* Get the mapping of the first page */
		if (as_get_physical_mapping(virt, &phys))
			goto error;

		/* First page can already break 4GiB condition */
		if (check_4gib && (phys & DMAMEM_4GiB) != 0)
			goto violated;

		while (size >= PAGE_SIZE) {
			/* Move to the next page */
			virt += PAGE_SIZE;
			size -= PAGE_SIZE;

			uintptr_t last_phys = phys;
			if (as_get_physical_mapping(virt, &phys))
				goto error;

			if (check_contiguous && (phys - last_phys) != PAGE_SIZE)
				goto violated;

			if (check_4gib && (phys & DMAMEM_4GiB) != 0)
				goto violated;
		}
	}

	/* All checks passed */
	return true;

violated:
error:
	return false;
}

/**
 * Lock an arbitrary buffer for DMA operations, creating a DMA buffer.
 *
 * FIXME: To handle page-unaligned buffers, we need to calculate the base
 *        address and lock the whole first page. But as the operation is not yet
 *        implemented in the kernel, it doesn't matter.
 */
errno_t dma_buffer_lock(dma_buffer_t *db, void *virt, size_t size)
{
	db->virt = virt;
	return dmamem_map(db->virt, size, 0, 0, &db->phys);
}

/**
 * Unlock a buffer for DMA operations.
 */
void dma_buffer_unlock(dma_buffer_t *db, size_t size)
{
	if (db->virt) {
		dmamem_unmap(db->virt, size);
		db->virt = NULL;
		db->phys = 0;
	}
}

/**
 * @}
 */
