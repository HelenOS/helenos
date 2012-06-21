/*
 * Copyright (c) 2006 Sergey Bondari
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
#include <mm/slab.h>
#include <arch/mm/page.h>
#include <typedefs.h>
#include <debug.h>
#include <align.h>

#define MAX_FRAMES  1024U
#define MAX_ORDER   8
#define TEST_RUNS   2

const char *test_falloc1(void)
{
	if (TEST_RUNS < 2)
		return "Test is compiled with TEST_RUNS < 2";
	
	uintptr_t *frames = (uintptr_t *)
	    malloc(MAX_FRAMES * sizeof(uintptr_t), 0);
	if (frames == NULL)
		return "Unable to allocate frames";
	
	unsigned int results[MAX_ORDER + 1];
	for (unsigned int run = 0; run < TEST_RUNS; run++) {
		for (unsigned int order = 0; order <= MAX_ORDER; order++) {
			TPRINTF("Allocating %u frames blocks ... ", 1 << order);
			
			unsigned int allocated = 0;
			for (unsigned int i = 0; i < (MAX_FRAMES >> order); i++) {
				frames[allocated] = (uintptr_t)
				    frame_alloc(order, FRAME_ATOMIC | FRAME_KA);
				
				if (ALIGN_UP(frames[allocated], FRAME_SIZE << order) !=
				    frames[allocated]) {
					TPRINTF("Block at address %p (size %u) is not aligned\n",
					    (void *) frames[allocated], (FRAME_SIZE << order) >> 10);
					return "Test failed";
				}
				
				if (frames[allocated])
					allocated++;
				else {
					TPRINTF("done. ");
					break;
				}
			}
			
			TPRINTF("%d blocks allocated.\n", allocated);
			
			if (run) {
				if (results[order] != allocated)
					return "Possible frame leak";
			} else
				results[order] = allocated;
			
			TPRINTF("Deallocating ... ");
			
			for (unsigned int i = 0; i < allocated; i++)
				frame_free(KA2PA(frames[i]));
			
			TPRINTF("done.\n");
		}
	}
	
	free(frames);
	
	return NULL;
}
