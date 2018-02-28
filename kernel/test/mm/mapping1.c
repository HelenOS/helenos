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

#include <print.h>
#include <test.h>
#include <mm/page.h>
#include <mm/frame.h>
#include <arch/mm/page.h>
#include <mm/km.h>
#include <typedefs.h>
#include <debug.h>
#include <arch.h>

#define TEST_MAGIC  UINT32_C(0x01234567)

const char *test_mapping1(void)
{
	uintptr_t frame = frame_alloc(1, FRAME_NONE, 0);

	uintptr_t page0 = km_map(frame, FRAME_SIZE,
	    PAGE_READ | PAGE_WRITE | PAGE_CACHEABLE);
	TPRINTF("Virtual address %p mapped to physical address %p.\n",
	    (void *) page0, (void *) frame);

	uintptr_t page1 = km_map(frame, FRAME_SIZE,
	    PAGE_READ | PAGE_WRITE | PAGE_CACHEABLE);
	TPRINTF("Virtual address %p mapped to physical address %p.\n",
	    (void *) page1, (void *) frame);

	for (unsigned int i = 0; i < 2; i++) {
		TPRINTF("Writing magic using the first virtual address.\n");

		*((uint32_t *) page0) = TEST_MAGIC;

		TPRINTF("Reading magic using the second virtual address.\n");

		uint32_t v = *((uint32_t *) page1);

		if (v != TEST_MAGIC) {
			km_unmap(page0, PAGE_SIZE);
			km_unmap(page1, PAGE_SIZE);
			frame_free(frame, 1);
			return "Criss-cross read does not match the value written.";
		}

		TPRINTF("Writing zero using the second virtual address.\n");

		*((uint32_t *) page1) = 0;

		TPRINTF("Reading zero using the first virtual address.\n");

		v = *((uint32_t *) page0);

		if (v != 0) {
			km_unmap(page0, PAGE_SIZE);
			km_unmap(page1, PAGE_SIZE);
			frame_free(frame, 1);
			return "Criss-cross read does not match the value written.";
		}
	}

	km_unmap(page0, PAGE_SIZE);
	km_unmap(page1, PAGE_SIZE);
	frame_free(frame, 1);

	return NULL;
}
