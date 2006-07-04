/*
 * Copyright (C) 2006 Sergey Bondari
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
#include <arch/types.h>
#include <debug.h>
#include <align.h>

#define MAX_FRAMES 1024
#define MAX_ORDER 8
#define TEST_RUNS 2

void test(void) {
	uintptr_t * frames = (uintptr_t *) malloc(MAX_FRAMES*sizeof(uintptr_t), 0);
	int results[MAX_ORDER+1];
	
	int i, order, run;
	int allocated;

	ASSERT(TEST_RUNS > 1);
	ASSERT(frames != NULL)

	for (run = 0; run < TEST_RUNS; run++) {
		for (order = 0; order <= MAX_ORDER; order++) {
			printf("Allocating %d frames blocks ... ", 1 << order);
			allocated = 0;
			for (i = 0; i < MAX_FRAMES >> order; i++) {
				frames[allocated] = (uintptr_t) frame_alloc(order, FRAME_ATOMIC | FRAME_KA);
				
				if (ALIGN_UP(frames[allocated], FRAME_SIZE << order) != frames[allocated]) {
					panic("Test failed. Block at address %p (size %dK) is not aligned\n", frames[allocated], (FRAME_SIZE << order) >> 10);
				}
				
				if (frames[allocated]) {
					allocated++;
				} else {
					printf("done. ");
					break;
				}
			}
		
			printf("%d blocks allocated.\n", allocated);
		
			if (run) {
				if (results[order] != allocated) {
					panic("Test failed. Frame leak possible.\n");
				}
			} else
				results[order] = allocated;
			
			printf("Deallocating ... ");
			for (i = 0; i < allocated; i++) {
				frame_free(KA2PA(frames[i]));
			}
			printf("done.\n");
		}
	}

	free(frames);
	
	printf("Test passed.\n");
}

