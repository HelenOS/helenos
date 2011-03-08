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
/** @addtogroup usb
 * @{
 */
/** @file
 * @brief UHCI driver
 */
#ifndef DRV_UHCI_TRANSLATOR_H
#define DRV_UHCI_TRANSLATOR_H

#include <assert.h>
#include <malloc.h>
#include <mem.h>
#include <as.h>

#define UHCI_STRCUTURES_ALIGNMENT 16
#define UHCI_REQUIRED_PAGE_SIZE 4096

static inline uintptr_t addr_to_phys(void *addr)
{
	uintptr_t result;
	int ret = as_get_physical_mapping(addr, &result);

	assert(ret == 0);
	return (result | ((uintptr_t)addr & 0xfff));
}

static inline void * malloc32(size_t size)
	{ return memalign(UHCI_STRCUTURES_ALIGNMENT, size); }

static inline void * get_page()
{
	void * free_address = as_get_mappable_page(UHCI_REQUIRED_PAGE_SIZE);
	assert(free_address);
	if (free_address == 0)
		return 0;
	void* ret =
	  as_area_create(free_address, UHCI_REQUIRED_PAGE_SIZE,
		  AS_AREA_READ | AS_AREA_WRITE);
	if (ret != free_address)
		return 0;
	return ret;
}

static inline void free32(void *addr)
	{ if (addr) free(addr); }

#endif
/**
 * @}
 */
