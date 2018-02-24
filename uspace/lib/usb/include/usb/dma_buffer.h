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
/** @addtogroup libusb
 * @{
 */
/** @file
 * @brief USB host controller library: DMA buffer helpers
 *
 * Simplifies handling of buffers accessible to hardware. Defines properties of
 * such buffer, which can be communicated through IPC to allow higher layers to
 * allocate a buffer that is ready to be passed to HW right away (after being
 * shared through IPC).
 *
 * Currently, it is possible to allocate either completely contiguous buffers
 * (with dma_map_anonymous) or arbitrary memory (with as_area_create). Shall the
 * kernel be updated, this is a subject of major optimization of memory usage.
 * The other way to do it without the kernel is building an userspace IO vector
 * in a similar way how QEMU does it.
 *
 * The structures themselves are defined in usbhc_iface, because they need to be
 * passed through IPC.
 */
#ifndef LIB_USB_DMA_BUFFER
#define LIB_USB_DMA_BUFFER

#include <as.h>
#include <bitops.h>
#include <errno.h>
#include <stdint.h>
#include <stdlib.h>
#include <usbhc_iface.h>

/**
 * The DMA policy describes properties of the buffer. It is used in two
 * different contexts. Either it represents requirements, which shall be
 * satisfied to avoid copying the buffer to a more strict one. Or, it is the
 * actual property of the buffer, which can be more strict than requested. It
 * always holds that more bits set means more restrictive policy, and that by
 * computing a bitwise OR one gets the restriction that holds for both.
 *
 * The high bits of a DMA policy represent a physical contiguity. If bit i is
 * set, it means that chunks of a size 2^(i+1) are contiguous in memory. It
 * shall never happen that bit i > j is set when j is not.
 *
 * The previous applies for i >= PAGE_WIDTH. Lower bits are used as bit flags.
 */
#define DMA_POLICY_FLAGS_MASK		(PAGE_SIZE - 1)
#define DMA_POLICY_CHUNK_SIZE_MASK	(~DMA_POLICY_FLAGS_MASK)

#define DMA_POLICY_4GiB	(1<<0)		/**< Must use only 32-bit addresses */

#define DMA_POLICY_STRICT		(-1UL)
#define DMA_POLICY_DEFAULT		DMA_POLICY_STRICT

extern dma_policy_t dma_policy_create(unsigned, size_t);

/**
 * Get mask which defines bits of offset in chunk.
 */
static inline size_t dma_policy_chunk_mask(const dma_policy_t policy)
{
	return policy | DMA_POLICY_FLAGS_MASK;
}

extern errno_t dma_buffer_alloc(dma_buffer_t *db, size_t size);
extern errno_t dma_buffer_alloc_policy(dma_buffer_t *, size_t, dma_policy_t);
extern void dma_buffer_free(dma_buffer_t *);

extern uintptr_t dma_buffer_phys(const dma_buffer_t *, const void *);

static inline uintptr_t dma_buffer_phys_base(const dma_buffer_t *db)
{
	return dma_buffer_phys(db, db->virt);
}

extern errno_t dma_buffer_lock(dma_buffer_t *, void *, size_t);
extern void dma_buffer_unlock(dma_buffer_t *, size_t);

extern void dma_buffer_acquire(dma_buffer_t *);
extern void dma_buffer_release(dma_buffer_t *);

static inline bool dma_buffer_is_set(const dma_buffer_t *db)
{
	return !!db->virt;
}

#endif
/**
 * @}
 */
