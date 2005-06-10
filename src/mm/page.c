/*
 * Copyright (C) 2001-2004 Jakub Jermar
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

#include <mm/page.h>
#include <arch/mm/page.h>
#include <arch/types.h>
#include <typedefs.h>

void page_init(void)
{	
	page_arch_init();
	map_page_to_frame(0x0, 0x0, PAGE_NOT_PRESENT, 0);
}

/** Map memory structure
 *
 * Identity-map memory structure
 * considering possible crossings
 * of page boundaries.
 *
 * @param s Address of the structure.
 * @param size Size of the structure.
 */
void map_structure(__address s, size_t size)
{
	int i, cnt, length;

	/* TODO: implement portable way of computing page address from address */
        length = size + (s - (s & 0xfffff000));
        cnt = length/PAGE_SIZE + (length%PAGE_SIZE>0);

        for (i = 0; i < cnt; i++)
                map_page_to_frame(s + i*PAGE_SIZE, s + i*PAGE_SIZE, PAGE_NOT_CACHEABLE, 0);

}
