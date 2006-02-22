/*
 * Copyright (C) 2005 Martin Decky
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

#include <genarch/ofw/memory_init.h>
#include <genarch/ofw/ofw.h>
#include <panic.h>
#include <mm/frame.h>
#include <align.h>
#include <arch/types.h>
#include <typedefs.h>

#define MEMMAP_MAX_RECORDS 32

typedef struct {
	__address start;
	size_t size;
} memmap_t;

static memmap_t memmap[MEMMAP_MAX_RECORDS];
size_t total_mem = 0;

void ofw_init_memmap(void)
{
	int i;
	int ret;

	phandle handle = ofw_find_device("/memory");
	if (handle == -1)
		panic("No RAM\n");
	
	ret = ofw_get_property(handle, "reg", &memmap, sizeof(memmap));
	if (ret == -1)
		panic("Device /memory has no reg property\n");
	
	
	for (i = 0; i < MEMMAP_MAX_RECORDS; i++) {
		if (memmap[i].size == 0)
			break;
		total_mem += memmap[i].size;
	}
}

size_t ofw_get_memory_size(void) 
{
	return total_mem;
}

void ofw_init_zones(void)
{
	int i;
	pfn_t confdata;

	for (i = 0; i < MEMMAP_MAX_RECORDS; i++) {
		if (memmap[i].size == 0)
			break;
		confdata = ADDR2PFN(memmap[i].start);
		if (confdata == 0)
			confdata = 2;
		zone_create(ADDR2PFN(memmap[i].start),
			    SIZE2FRAMES(ALIGN_DOWN(memmap[i].size,PAGE_SIZE)),
			    confdata, 0);
	}
}
