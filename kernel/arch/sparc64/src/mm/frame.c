/*
 * Copyright (C) 2005 Jakub Jermar
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
#include <config.h>
#include <align.h>

/** Create memory zones according to information stored in bootinfo.
 *
 * Walk the bootinfo memory map and create frame zones according to it.
 * The first frame is not blacklisted here as it is done in generic
 * frame_init().
 */
void frame_arch_init(void)
{
	int i;
	pfn_t confdata;

	for (i = 0; i < bootinfo.memmap.count; i++) {

		/*
		 * The memmap is created by HelenOS boot loader.
		 * It already contains no holes.
		 */
	
		confdata = ADDR2PFN(bootinfo.memmap.zones[i].start);
		if (confdata == 0)
			confdata = 2;
		zone_create(ADDR2PFN(bootinfo.memmap.zones[i].start),
			SIZE2FRAMES(ALIGN_DOWN(bootinfo.memmap.zones[i].size, PAGE_SIZE)),
			confdata, 0);
	}

}

/** @}
 */
