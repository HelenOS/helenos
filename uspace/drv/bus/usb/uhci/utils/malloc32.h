/*
 * Copyright (c) 2010 Jan Vesely
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
/** @addtogroup drvusbuhci
 * @{
 */
/** @file
 * @brief UHCI driver
 */
#ifndef DRV_UHCI_UTILS_MALLOC32_H
#define DRV_UHCI_UTILS_MALLOC32_H

#include <as.h>
#include <assert.h>
#include <ddi.h>
#include <errno.h>
#include <malloc.h>
#include <mem.h>
#include <unistd.h>

#define UHCI_STRCUTURES_ALIGNMENT 16
#define UHCI_REQUIRED_PAGE_SIZE 4096


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
	
	return result;
}

/** DMA malloc simulator
 *
 * @param[in] size Size of the required memory space
 * @return Address of the aligned and big enough memory place, NULL on failure.
 */
static inline void * malloc32(size_t size)
{
	/* This works only when the host has less than 4GB of memory as
	 * physical address needs to fit into 32 bits */

	/* If we need more than one page there is no guarantee that the
	 * memory will be continuous */
	if (size > PAGE_SIZE)
		return NULL;
	/* Calculate alignment to make sure the block won't cross page
	 * boundary */
	size_t alignment = UHCI_STRCUTURES_ALIGNMENT;
	while (alignment < size)
		alignment *= 2;
	return memalign(alignment, size);
}

/** DMA malloc simulator
 *
 * @param[in] addr Address of the place allocated by malloc32
 */
static inline void free32(void *addr)
{
	free(addr);
}

/** Create 4KB page mapping
 *
 * @return Address of the mapped page, NULL on failure.
 */
static inline void *get_page(void)
{
	uintptr_t phys;
	void *address = AS_AREA_ANY;
	
	const int ret = dmamem_map_anonymous(UHCI_REQUIRED_PAGE_SIZE,
	    DMAMEM_4GiB, AS_AREA_READ | AS_AREA_WRITE, 0, &phys,
	    &address);
	
	return ((ret == EOK) ? address : NULL);
}

static inline void return_page(void *page)
{
	dmamem_unmap_anonymous(page);
}

#endif
/**
 * @}
 */
