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

dma_policy_t dma_policy_default = {
	.use64 = false,
	.alignment = PAGE_SIZE,
};

/**
 * Allocate a DMA buffer.
 *
 * @param[in] db dma_buffer_t structure to fill
 * @param[in] policy dma_policy_t structure to guide
 * @param[in] size Size of the required memory space
 * @return Error code.
 */
int dma_buffer_alloc_policy(dma_buffer_t *db, size_t size, dma_policy_t policy)
{
	assert(db);

	if (policy.alignment > PAGE_SIZE)
		return EINVAL;

	const size_t aligned_size = ALIGN_UP(size, policy.alignment);
	const size_t real_size = ALIGN_UP(aligned_size, PAGE_SIZE);
	const uintptr_t flags = policy.use64 ? 0 : DMAMEM_4GiB;

	uintptr_t phys;
	void *address = AS_AREA_ANY;

	const int ret = dmamem_map_anonymous(real_size,
	    flags, AS_AREA_READ | AS_AREA_WRITE, 0,
	    &phys, &address);

	if (ret == EOK) {
		/* Poison, accessing it should be enough to make sure
		 * the location is mapped, but poison works better */
		memset(address, 0x5, real_size);
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
int dma_buffer_alloc(dma_buffer_t *db, size_t size)
{
	return dma_buffer_alloc_policy(db, size, dma_policy_default);
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
 * @}
 */
