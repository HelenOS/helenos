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
 * Simplifies handling of buffers accessible to hardware. Defines properties of
 * such buffer, which can be communicated through IPC to allow higher layers to
 * allocate a buffer that is ready to be passed to HW right away (after being
 * shared through IPC).
 *
 * Note that although allocated memory is always page-aligned, the buffer itself
 * may be only a part of it, justifying the existence of page-alignment and
 * page-crossing flags.
 *
 * Also, currently the buffers that are allocated are always contiguous and
 * page-aligned, regardless of whether the policy requires it. We blindly
 * believe this fact in dma_buffer_phys, which will yield wrong results if the
 * buffer is not contiguous.
 */
#ifndef LIB_USB_DMA_BUFFER
#define LIB_USB_DMA_BUFFER

#include <stdint.h>
#include <stdlib.h>
#include <usbhc_iface.h>
#include <errno.h>

#define DMA_POLICY_4GiB		(1<<0)	/**< Must use only 32-bit addresses */
#define DMA_POLICY_PAGE_ALIGNED	(1<<1)	/**< The first pointer must be page-aligned */
#define DMA_POLICY_CONTIGUOUS	(1<<2)	/**< Pages must follow each other physically */
#define DMA_POLICY_NOT_CROSSING	(1<<3)	/**< Buffer must not cross page boundary. (Implies buffer is no larger than page).  */

#define DMA_POLICY_STRICT	(-1U)
#define DMA_POLICY_DEFAULT	DMA_POLICY_STRICT

typedef struct dma_buffer {
	void *virt;
	uintptr_t phys;
} dma_buffer_t;

extern errno_t dma_buffer_alloc(dma_buffer_t *db, size_t size);
extern errno_t dma_buffer_alloc_policy(dma_buffer_t *, size_t, dma_policy_t);
extern void dma_buffer_free(dma_buffer_t *);
extern uintptr_t dma_buffer_phys(const dma_buffer_t *, void *);

extern bool dma_buffer_check_policy(const void *, size_t, const dma_policy_t);

extern errno_t dma_buffer_lock(dma_buffer_t *, void *, size_t);
extern void dma_buffer_unlock(dma_buffer_t *, size_t);

static inline int dma_buffer_is_set(dma_buffer_t *db)
{
	return !!db->virt;
}

#endif
/**
 * @}
 */
