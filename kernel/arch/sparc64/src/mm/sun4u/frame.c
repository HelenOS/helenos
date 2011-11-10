/*
 * Copyright (c) 2005 Jakub Jermar
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

/** @addtogroup sparc64mm
 * @{
 */
/** @file
 */

#include <arch/mm/frame.h>
#include <mm/frame.h>
#include <arch/boot/boot.h>
#include <typedefs.h>
#include <config.h>
#include <align.h>
#include <macros.h>

uintptr_t last_frame = (uintptr_t) NULL;

/** Create memory zones according to information stored in memmap.
 *
 * Walk the memory map and create frame zones according to it.
 */
void frame_low_arch_init(void)
{
	if (config.cpu_active == 1) {
		unsigned int i;
		
		for (i = 0; i < memmap.cnt; i++) {
			/* To be safe, make the available zone possibly smaller */
			uintptr_t new_start = ALIGN_UP((uintptr_t) memmap.zones[i].start,
			    FRAME_SIZE);
			size_t new_size = ALIGN_DOWN(memmap.zones[i].size -
			    (new_start - ((uintptr_t) memmap.zones[i].start)), FRAME_SIZE);
			
			/*
			 * The memmap is created by HelenOS boot loader.
			 * It already contains no holes.
			 */
			
			pfn_t confdata = ADDR2PFN(new_start);
			
			if (confdata == ADDR2PFN(KA2PA(PFN2ADDR(0))))
				confdata = ADDR2PFN(KA2PA(PFN2ADDR(2)));
			
			zone_create(ADDR2PFN(new_start), SIZE2FRAMES(new_size),
			    confdata, 0);
			
			last_frame = max(last_frame, new_start + new_size);
		}
		
		/*
		 * On sparc64, physical memory can start on a non-zero address.
		 * The generic frame_init() only marks PFN 0 as not free, so we
		 * must mark the physically first frame not free explicitly
		 * here, no matter what is its address.
		 */
		frame_mark_unavailable(ADDR2PFN(KA2PA(PFN2ADDR(0))), 1);
	}
	
	end_of_identity = PA2KA(last_frame);
}

void frame_high_arch_init(void)
{
}

/** @}
 */
