/*
 * Copyright (c) 2013 Jan Vesely
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
/** @addtogroup drvusbehci
 * @{
 */
/** @file
 * @brief EHCI driver
 */
#ifndef DRV_EHCI_UTILS_MALLOC32_H
#define DRV_EHCI_UTILS_MALLOC32_H

#include <as.h>
#include <ddi.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/types.h>

/* Generic TDs and EDs require 16byte alignment,
 * Isochronous TD require 32byte alignment,
 * buffers do not have to be aligned.
 */
#define EHCI_ALIGN   32

#define EHCI_REQUIRED_PAGE_SIZE   4096

/** Get physical address translation
 *
 * @param[in] addr Virtual address to translate
 * @return Physical address if exists, NULL otherwise.
 */
static inline uintptr_t addr_to_phys(const void *addr)
{
	uintptr_t result;
	int ret = as_get_physical_mapping(addr, &result);
	
	if (ret != EOK)
		return 0;
	
	return result;
}

/** Physical mallocator simulator
 *
 * @param[in] size Size of the required memory space
 * @return Address of the aligned and big enough memory place, NULL on failure.
 */
static inline void * malloc32(size_t size)
	{ return memalign(EHCI_ALIGN, size); }

/** Physical mallocator simulator
 *
 * @param[in] addr Address of the place allocated by malloc32
 */
static inline void free32(void *addr)
	{ free(addr); }

/** Create 4KB page mapping
 *
 * @return Address of the mapped page, NULL on failure.
 */
static inline void *get_page(void)
{
	uintptr_t phys;
	void *address;

	const int ret = dmamem_map_anonymous(EHCI_REQUIRED_PAGE_SIZE,
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
