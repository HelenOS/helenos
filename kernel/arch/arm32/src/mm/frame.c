/*
 * Copyright (c) 2007 Pavel Jancik, Michal Kebrt
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

/** @addtogroup arm32mm
 * @{
 */
/** @file
 *  @brief Frame related functions.
 */

#include <mm/frame.h>
#include <arch/mm/frame.h>
#include <arch/machine_func.h>
#include <config.h>
#include <align.h>
#include <macros.h>

/** Address of the last frame in the memory. */
uintptr_t last_frame = 0;

/** Creates memory zones. */
void frame_low_arch_init(void)
{
	uintptr_t mem_start, mem_size;
	uintptr_t first_frame;
	uintptr_t num_frames;

	machine_get_memory_extents(&mem_start, &mem_size);
	first_frame = ALIGN_UP(mem_start, FRAME_SIZE);
	last_frame = ALIGN_DOWN(mem_start + mem_size, FRAME_SIZE);
	num_frames = (last_frame - first_frame) >> FRAME_WIDTH;
	
	/* All memory as one zone */
	zone_create(first_frame >> FRAME_WIDTH, num_frames,
	    BOOT_PAGE_TABLE_START_FRAME + BOOT_PAGE_TABLE_SIZE_IN_FRAMES, 0);
	
	/* blacklist boot page table */
	frame_mark_unavailable(BOOT_PAGE_TABLE_START_FRAME,
	    BOOT_PAGE_TABLE_SIZE_IN_FRAMES);

	machine_frame_init();
}

void frame_high_arch_init(void)
{
}

/** Frees the boot page table. */
void boot_page_table_free(void)
{
	unsigned int i;
	for (i = 0; i < BOOT_PAGE_TABLE_SIZE_IN_FRAMES; i++)
		frame_free(i * FRAME_SIZE + BOOT_PAGE_TABLE_ADDRESS);
}

/** @}
 */
