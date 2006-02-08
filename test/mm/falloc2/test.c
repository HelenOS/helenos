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
#include <arch/atomic.h>
#include <debug.h>
#include <proc/thread.h>
#include <memstr.h>
#include <arch.h>

#define MAX_FRAMES 256
#define MAX_ORDER 8

#define THREAD_RUNS 1
#define THREADS 8

static void thread(void * arg);
static void failed(void);

static atomic_t thread_count;

void thread(void * arg)
{
	int status, order, run, allocated, i;
	__u8 val = THREAD->tid % THREADS;
	index_t k;
	
	__address * frames =  (__address *) malloc(MAX_FRAMES * sizeof(__address), FRAME_ATOMIC);
	ASSERT(frames != NULL);

	for (run = 0; run < THREAD_RUNS; run++) {
		for (order = 0; order <= MAX_ORDER; order++) {
			printf("Thread #%d (cpu%d): Allocating %d frames blocks ... \n", THREAD->tid, CPU->id, 1 << order);
			allocated = 0;
			for (i = 0; i < (MAX_FRAMES >> order); i++) {
				frames[allocated] = PA2KA(PFN2ADDR(frame_alloc_rc(order, FRAME_ATOMIC | FRAME_KA, &status)));
				if (status == 0) {
					memsetb(frames[allocated], FRAME_SIZE << order, val);
					allocated++;
				} else {
					break;
				}
			}
			printf("Thread #%d (cpu%d): %d blocks allocated.\n", THREAD->tid, CPU->id, allocated);

			printf("Thread #%d (cpu%d): Deallocating ... \n", THREAD->tid, CPU->id);
			for (i = 0; i < allocated; i++) {
				for (k = 0; k <= ((FRAME_SIZE << order) - 1); k++) {
					if (((__u8 *) frames[i])[k] != val) {
						printf("Thread #%d (cpu%d): Unexpected data (%d) in block %P offset %X\n", THREAD->tid, CPU->id, ((char *) frames[i])[k], frames[i], k);
						failed();
					}
				}
				frame_free(ADDR2PFN(KA2PA(frames[i])));
			}
			printf("Thread #%d (cpu%d): Finished run.\n", THREAD->tid, CPU->id);
		}
	}
	
	free(frames);
	printf("Thread #%d (cpu%d): Exiting\n", THREAD->tid, CPU->id);
	atomic_dec(&thread_count);
}


void failed(void)
{
	panic("Test failed.\n");
}


void test(void)
{
	int i;

	atomic_set(&thread_count, THREADS);
		
	for (i = 0; i < THREADS; i++) {
		thread_t * thrd;
		thrd = thread_create(thread, NULL, TASK, 0);
		if (thrd)
			thread_ready(thrd);
		else
			failed();
	}
	
	while (thread_count.count)
		;

	printf("Test passed.\n");
}
