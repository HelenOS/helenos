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
#include <print.h>
#include <test.h>
#include <mm/page.h>
#include <mm/frame.h>
#include <mm/as.h>
#include <arch/mm/page.h>
#include <arch/types.h>
#include <debug.h>

#define PAGE0	0x10000000
#define PAGE1	(PAGE0+PAGE_SIZE)

#define VALUE0	0x01234567
#define VALUE1	0x89abcdef

void test(void)
{
	__address frame0, frame1;
	__u32 v0, v1;

	printf("Memory management test mapping #1\n");

	frame0 = PA2KA(PFN2ADDR(frame_alloc(ONE_FRAME, FRAME_KA)));
	frame1 = PA2KA(PFN2ADDR(frame_alloc(ONE_FRAME, FRAME_KA)));

	printf("Writing %#x to physical address %p.\n", VALUE0, KA2PA(frame0));
	*((__u32 *) frame0) = VALUE0;
	printf("Writing %#x to physical address %p.\n", VALUE1, KA2PA(frame1));
	*((__u32 *) frame1) = VALUE1;
	
	printf("Mapping virtual address %p to physical address %p.\n", PAGE0, KA2PA(frame0));
	page_mapping_insert(AS_KERNEL, PAGE0, KA2PA(frame0), PAGE_PRESENT | PAGE_WRITE);
	printf("Mapping virtual address %p to physical address %p.\n", PAGE1, KA2PA(frame1));	
	page_mapping_insert(AS_KERNEL, PAGE1, KA2PA(frame1), PAGE_PRESENT | PAGE_WRITE);
	
	printf("Value at virtual address %p is %#x.\n", PAGE0, v0 = *((__u32 *) PAGE0));
	printf("Value at virtual address %p is %#x.\n", PAGE1, v1 = *((__u32 *) PAGE1));
	
	ASSERT(v0 == VALUE0);
	ASSERT(v1 == VALUE1);

	printf("Writing %#x to virtual address %p.\n", 0, PAGE0);
	*((__u32 *) PAGE0) = 0;
	printf("Writing %#x to virtual address %p.\n", 0, PAGE1);
	*((__u32 *) PAGE1) = 0;	

	v0 = *((__u32 *) PAGE0);
	v1 = *((__u32 *) PAGE1);
	
	printf("Value at virtual address %p is %#x.\n", PAGE0, *((__u32 *) PAGE0));	
	printf("Value at virtual address %p is %#x.\n", PAGE1, *((__u32 *) PAGE1));

	ASSERT(v0 == 0);
	ASSERT(v1 == 0);
	
	printf("Test passed.\n");
	
}
