/*
 * Copyright (c) 2009 Jakub Jermar
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

/** @addtogroup arm32
 * @{
 */
/** @file
 *  @brief Kernel part of Restartable Atomic Sequences support.
 */

#include <arch/ras.h>
#include <mm/mm.h>
#include <mm/frame.h>
#include <mm/page.h>
#include <mm/km.h>
#include <mm/tlb.h>
#include <mm/asid.h>
#include <interrupt.h>
#include <arch/exception.h>
#include <arch.h>
#include <mem.h>
#include <typedefs.h>

uintptr_t *ras_page = NULL;

void ras_init(void)
{
	uintptr_t frame =
	    frame_alloc(1, FRAME_ATOMIC | FRAME_HIGHMEM, 0);
	if (!frame)
		frame = frame_alloc(1, FRAME_LOWMEM, 0);

	ras_page = (uintptr_t *) km_map(frame,
	    PAGE_SIZE, PAGE_READ | PAGE_WRITE | PAGE_USER | PAGE_CACHEABLE);

	memsetb(ras_page, PAGE_SIZE, 0);
	ras_page[RAS_START] = 0;
	ras_page[RAS_END] = 0xffffffff;
}

void ras_check(unsigned int n, istate_t *istate)
{
	bool restart_needed = false;
	uintptr_t restart_pc = 0;

	if (istate_from_uspace(istate)) {
		if (ras_page[RAS_START]) {
			if ((ras_page[RAS_START] < istate->pc) &&
			    (ras_page[RAS_END] > istate->pc)) {
				restart_needed = true;
				restart_pc = ras_page[RAS_START];
			}
			ras_page[RAS_START] = 0;
			ras_page[RAS_END] = 0xffffffff;
		}
	}

	exc_dispatch(n, istate);
	if (restart_needed)
		istate->pc = restart_pc;
}

/** @}
 */
