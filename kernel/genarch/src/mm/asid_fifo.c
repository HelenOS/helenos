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

/** @addtogroup genarchmm
 * @{
 */

/**
 * @file
 * @brief	FIFO queue ASID management.
 *
 * Architectures that link with this file keep the unallocated ASIDs
 * in FIFO queue. The queue can be statically (e.g. mips32) or
 * dynamically allocated (e.g ia64 and sparc64).
 */

#include <genarch/mm/asid_fifo.h>
#include <arch/mm/asid.h>
#include <mm/asid.h>
#include <adt/fifo.h>

#define FIFO_STATIC_LIMIT	1024
#define FIFO_STATIC		(ASIDS_ALLOCABLE<FIFO_STATIC_LIMIT)

/**
 * FIFO queue containing unassigned ASIDs.
 * Can be only accessed when asidlock is held.
 */
#if FIFO_STATIC
FIFO_INITIALIZE_STATIC(free_asids, asid_t, ASIDS_ALLOCABLE);
#else
FIFO_INITIALIZE_DYNAMIC(free_asids, asid_t, ASIDS_ALLOCABLE);
#endif

/** Initialize data structures for O(1) ASID allocation and deallocation. */
void asid_fifo_init(void)
{
	int i;

#if (!FIFO_STATIC)
	fifo_create(free_asids);
#endif

	for (i = 0; i < ASIDS_ALLOCABLE; i++) {
		fifo_push(free_asids, ASID_START + i);
	}
}

/** Allocate free ASID.
 *
 * Allocation runs in O(1).
 *
 * @return Free ASID.
 */
asid_t asid_find_free(void)
{
	return fifo_pop(free_asids);
}

/** Return ASID among free ASIDs.
 *
 * This operation runs in O(1).
 *
 * @param asid ASID being freed.
 */
void asid_put_arch(asid_t asid)
{
	fifo_push(free_asids, asid);
}

/** @}
 */
