/*
 * Copyright (c) 2018 Ondrej Hlavaty
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

dma_policy_t dma_policy_create(unsigned flags, size_t chunk_size)
{
	assert((chunk_size & (chunk_size - 1)) == 0); /* Check if power of 2 */
	assert(chunk_size >= PAGE_SIZE || chunk_size == 0);

	return ((chunk_size - 1) & DMA_POLICY_CHUNK_SIZE_MASK) |
	    (flags & DMA_POLICY_FLAGS_MASK);
}

/**
 * As the driver is typically using only a few buffers at once, we cache the
 * physical mapping to avoid calling the kernel unnecessarily often. This cache
 * is global for a task.
 *
 * TODO: "few" is currently limited to one.
 */
static struct {
	const void *last;
	uintptr_t phys;
} phys_mapping_cache = { 0 };

static void cache_insert(const void *v, uintptr_t p)
{
	phys_mapping_cache.last = v;
	phys_mapping_cache.phys = p;
}

static void cache_evict(const void *v)
{
	if (phys_mapping_cache.last == v)
		phys_mapping_cache.last = NULL;
}

static bool cache_find(const void *v, uintptr_t *p)
{
	*p = phys_mapping_cache.phys;
	return phys_mapping_cache.last == v;
}

/**
 * Allocate a DMA buffer.
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

	const int err = dmamem_map_anonymous(real_size,
	    flags, AS_AREA_READ | AS_AREA_WRITE, 0,
	    &phys, &address);
	if (err)
		return err;

	/* Access the pages to force mapping */
	volatile char *buf = address;
	for (size_t i = 0; i < size; i += PAGE_SIZE)
		buf[i] = 0xff;

	db->virt = address;
	db->policy = dma_policy_create(policy, 0);
	cache_insert(db->virt, phys);

	return EOK;
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
		db->policy = 0;
	}
}

/**
 * Convert a pointer inside a buffer to physical address.
 *
 * @param[in] db Buffer at which virt is pointing
 * @param[in] virt Pointer somewhere inside db
 */
uintptr_t dma_buffer_phys(const dma_buffer_t *db, const void *virt)
{
	const size_t chunk_mask = dma_policy_chunk_mask(db->policy);
	const uintptr_t offset = (virt - db->virt) & chunk_mask;
	const void *chunk_base = virt - offset;

	uintptr_t phys;

	if (!cache_find(chunk_base, &phys)) {
		if (as_get_physical_mapping(chunk_base, &phys))
			return 0;
		cache_insert(chunk_base, phys);
	}

	return phys + offset;
}

static bool dma_buffer_is_4gib(dma_buffer_t *db, size_t size)
{
	if (sizeof(uintptr_t) <= 32)
		return true;

	const size_t chunk_size = dma_policy_chunk_mask(db->policy) + 1;
	const size_t chunks = chunk_size ? 1 : size / chunk_size;

	for (size_t c = 0; c < chunks; c++) {
		const void *addr = db->virt + (c * chunk_size);
		const uintptr_t phys = dma_buffer_phys(db, addr);

		if ((phys & DMAMEM_4GiB) != 0)
			return false;
	}

	return true;
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
	assert(virt);

	uintptr_t phys;

	const errno_t err = dmamem_map(virt, size, 0, 0, &phys);
	if (err)
		return err;

	db->virt = virt;
	db->policy = dma_policy_create(0, PAGE_SIZE);
	cache_insert(virt, phys);

	unsigned flags = -1U;
	if (!dma_buffer_is_4gib(db, size))
		flags &= ~DMA_POLICY_4GiB;
	db->policy = dma_policy_create(flags, PAGE_SIZE);

	return EOK;
}

/**
 * Unlock a buffer for DMA operations.
 */
void dma_buffer_unlock(dma_buffer_t *db, size_t size)
{
	if (db->virt) {
		dmamem_unmap(db->virt, size);
		db->virt = NULL;
		db->policy = 0;
	}
}

/**
 * Must be called when the buffer is received over IPC. Clears potentially
 * leftover value from different buffer mapped to the same virtual address.
 */
void dma_buffer_acquire(dma_buffer_t *db)
{
	cache_evict(db->virt);
}

/**
 * Counterpart of acquire.
 */
void dma_buffer_release(dma_buffer_t *db)
{
	cache_evict(db->virt);
}

/**
 * @}
 */
