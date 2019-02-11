/*
 * Copyright (c) 2006 Jakub Jermar
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

/** @addtogroup abi_generic
 * @{
 */
/** @file
 */

#ifndef _ABI_DDI_ARG_H_
#define _ABI_DDI_ARG_H_

#include <stdint.h>

enum {
	DMAMEM_FLAGS_ANONYMOUS = 0x01,
};

/** Structure encapsulating arguments for SYS_PHYSMEM_MAP syscall. */
typedef struct {
	/** ID of the destination task. */
	uint64_t task_id;
	/** Physical address of starting frame. */
	void *phys_base;
	/** Virtual address of starting page. */
	void *virt_base;
	/** Number of pages to map. */
	size_t pages;
	/** Address space area flags for the mapping. */
	unsigned int flags;
} ddi_memarg_t;

/** Structure encapsulating arguments for SYS_ENABLE_IOSPACE syscall. */
typedef struct {
	uint64_t task_id;  /**< ID of the destination task. */
	void *ioaddr;      /**< Starting I/O space address. */
	size_t size;       /**< Number of bytes. */
} ddi_ioarg_t;

#endif

/** @}
 */
