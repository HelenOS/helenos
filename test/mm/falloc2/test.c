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
#include <arch/mm/page.h>
#include <arch/types.h>
#include <arch/atomic.h>
#include <debug.h>
#include <proc/thread.h>
#include <memstr.h>

#define MAX_FRAMES 128
#define MAX_ORDER 2

#define THREAD_RUNS 1
#define THREADS 6

static void thread(void * arg);
static void failed(void);

static atomic_t thread_count;

void thread(void * arg) {
	int status, order, run, allocated,i;
	
	__u8 val = *((__u8 *) arg);
	index_t k;
	
	__address frames[MAX_FRAMES];

	for (run=0;run<THREAD_RUNS;run++) {
	
		for (order=0;order<=MAX_ORDER;order++) {
			printf("Thread #%d: Allocating %d frames blocks ... \n",val, 1<<order);
			allocated = 0;
			for (i=0;i<MAX_FRAMES>>order;i++) {
				frames[allocated] = frame_alloc(FRAME_NON_BLOCKING | FRAME_KA,order, &status);
				if (status == 0) {
					memsetb(frames[allocated], (1 << order) * FRAME_SIZE, val);
					allocated++;
				} else {
					break;
				}
			}
		
			printf("Thread #%d: %d blocks alocated.\n",val, allocated);

			printf("Thread #%d: Deallocating ... \n", val);
			for (i=0;i<allocated;i++) {
				for (k=0;k<=((FRAME_SIZE << order) - 1);k++) {
					if ( ((char *) frames[i])[k] != val ) {
						printf("Thread #%d: Unexpected data in block %P offset %X\n",val, frames[i], k);
						failed();
					}
				
				}
			
				frame_free(frames[i]);
			}
			printf("Thread #%d: Finished run.\n", val);
		}
	}
	
	
	atomic_dec(&thread_count);

}


void failed(void) {
	panic("Test failed.\n");
}


void test(void) {
	int i;

	atomic_set(&thread_count, THREADS);
		
	for (i=1;i<=THREADS;i++) {
		thread_t * thrd;
		thrd = thread_create(thread, &i, TASK, 0);
		if (thrd) thread_ready(thrd); else failed();
	}
	
	while (thread_count.count);

	printf("Test passed\n");
}

