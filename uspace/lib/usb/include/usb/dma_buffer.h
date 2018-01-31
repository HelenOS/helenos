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
/** @addtogroup libusb
 * @{
 */
/** @file
 * @brief USB host controller library: DMA buffer helpers
 *
 * Simplifies usage of bounce buffers.
 *
 * Currently the minimum size allocated is a page, which is wasteful. Could be
 * extended to support memory pools, which will enable smaller units of
 * allocation.
 */
#ifndef LIB_USB_DMA_BUFFER
#define LIB_USB_DMA_BUFFER

#include <stdint.h>
#include <stdlib.h>
#include <errno.h>

typedef const struct dma_policy {
	bool use64;		/**< Whether to use more than initial 4GiB of memory */
	size_t alignment;	/**< What alignment is needed. At most PAGE_SIZE. */
} dma_policy_t;

typedef struct dma_buffer {
	void *virt;
	uintptr_t phys;
} dma_buffer_t;

extern dma_policy_t dma_policy_default;

extern int dma_buffer_alloc(dma_buffer_t *db, size_t size);
extern int dma_buffer_alloc_policy(dma_buffer_t *, size_t, dma_policy_t);
extern void dma_buffer_free(dma_buffer_t *);
extern uintptr_t dma_buffer_phys(const dma_buffer_t *, void *);

static inline int dma_buffer_is_set(dma_buffer_t *db)
{
	return !!db->virt;
}

#endif
/**
 * @}
 */
