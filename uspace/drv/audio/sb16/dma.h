/*
 * Copyright (c) 2011 Jan Vesely
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
/** @addtogroup drvaudiosb16
 * @{
 */
/** @file
 * @brief DMA memory management
 */
#ifndef DRV_AUDIO_SB16_DMA_H
#define DRV_AUDIO_SB16_DMA_H

#include <assert.h>
#include <errno.h>
#include <malloc.h>
#include <mem.h>
#include <as.h>

#define DMA_ALIGNENT 65536

/** Get physical address translation
 *
 * @param[in] addr Virtual address to translate
 * @return Physical address if exists, NULL otherwise.
 */
static inline uintptr_t addr_to_phys(const void *addr)
{
	if (addr == NULL)
		return 0;

	uintptr_t result;
	const int ret = as_get_physical_mapping(addr, &result);
	if (ret != EOK)
		return 0;
	return (result | ((uintptr_t)addr & 0xfff));
}
/*----------------------------------------------------------------------------*/
/** DMA mallocator simulator
 *
 * @param[in] size Size of the required memory space
 * @return Address of the aligned and big enough memory place, NULL on failure.
 */
static inline void * malloc24(size_t size)
{
	/* This works only reliably when the host has less than 16MB of memory
	 * as physical address needs to fit into 24 bits */

	/* If we need more than one page there is no guarantee that the
	 * memory will be continuous */
	if (size > PAGE_SIZE)
		return NULL;
	/* Calculate alignment to make sure the block won't cross page
	 * boundary */
	return memalign(DMA_ALIGNENT, size);
}
/*----------------------------------------------------------------------------*/
/** DMA mallocator simulator
 *
 * @param[in] addr Address of the place allocated by malloc24
 */
static inline void free24(void *addr)
	{ free(addr); }

#endif
/**
 * @}
 */
