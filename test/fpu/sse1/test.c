/*
 * Copyright (C) 2005 Ondrej Palkovsky
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
#include <debug.h>
#include <panic.h>

#include <test.h>
#include <atomic.h>
#include <proc/thread.h>
#include <time/delay.h>

#include <arch.h>

#define THREADS		50
#define DELAY   	10000L
#define ATTEMPTS        5

static atomic_t threads_ok;
static waitq_t can_start;

static void testit1(void *data)
{
	int i;
	int arg __attribute__((aligned(16))) = (int)((__native) data);
	int after_arg __attribute__((aligned(16)));
	
	waitq_sleep(&can_start);

	for (i = 0; i<ATTEMPTS; i++) {
		__asm__ volatile (
			"movlpd	%0, %%xmm2"
			:"=m"(arg)
			);

		delay(DELAY);
		__asm__ volatile (
			"movlpd %%xmm2, %0"
			:"=m"(after_arg)
			);
		
		if(arg != after_arg)
			panic("tid%d: arg(%d) != %d\n", 
			      THREAD->tid, arg, after_arg);
	}

	atomic_inc(&threads_ok);
}

static void testit2(void *data)
{
	int i;
	int arg __attribute__((aligned(16))) = (int)((__native) data);
	int after_arg __attribute__((aligned(16)));
	
	waitq_sleep(&can_start);

	for (i = 0; i<ATTEMPTS; i++) {
		__asm__ volatile (
			"movlpd	%0, %%xmm2"
			:"=m"(arg)
			);

		scheduler();
		__asm__ volatile (
			"movlpd %%xmm2, %0"
			:"=m"(after_arg)
			);
		
		if(arg != after_arg)
			panic("tid%d: arg(%d) != %d\n", 
			      THREAD->tid, arg, after_arg);
	}

	atomic_inc(&threads_ok);
}


void test(void)
{
	thread_t *t;
	int i;

	waitq_initialize(&can_start);

	printf("SSE test #1\n");
	printf("Creating %d threads... ", THREADS);

	for (i=0; i<THREADS/2; i++) {  
		if (!(t = thread_create(testit1, (void *)((__native)i*2), TASK, 0, "testit1")))
			panic("could not create thread\n");
		thread_ready(t);
		if (!(t = thread_create(testit2, (void *)((__native)i*2+1), TASK, 0, "testit2")))
			panic("could not create thread\n");
		thread_ready(t);
	}

	printf("ok\n");
	
	thread_sleep(1);
	waitq_wakeup(&can_start, WAKEUP_ALL);

	while (atomic_get(&threads_ok) != THREADS)
		;
		
	printf("Test passed.\n");
}
