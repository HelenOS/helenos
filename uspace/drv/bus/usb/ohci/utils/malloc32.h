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
/** @addtogroup drvusbohci
 * @{
 */
/** @file
 * @brief OHCI driver
 */
#ifndef DRV_OHCI_UTILS_MALLOC32_H
#define DRV_OHCI_UTILS_MALLOC32_H

#include <assert.h>
#include <malloc.h>
#include <unistd.h>
#include <errno.h>
#include <mem.h>
#include <as.h>

/* Generic TDs and EDs require 16byte alignment,
 * Isochronous TD require 32byte alignment,
 * buffers do not have to be aligned.
 */
#define OHCI_ALIGN 32

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
	{ return memalign(OHCI_ALIGN, size); }

/** Physical mallocator simulator
 *
 * @param[in] addr Address of the place allocated by malloc32
 */
static inline void free32(void *addr)
	{ free(addr); }
#endif
/**
 * @}
 */
