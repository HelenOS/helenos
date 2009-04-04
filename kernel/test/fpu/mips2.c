/*
 * Copyright (c) 2005 Ondrej Palkovsky
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

#include <test.h>
#include <atomic.h>
#include <proc/thread.h>
#include <time/delay.h>

#include <arch.h>

#define THREADS   50
#define DELAY     10000L
#define ATTEMPTS  5

static atomic_t threads_ok;
static atomic_t threads_fault;
static waitq_t can_start;

static void testit1(void *data)
{
	int i;
	int arg __attribute__((aligned(16))) = (int) ((unative_t) data);
	int after_arg __attribute__((aligned(16)));
	
	thread_detach(THREAD);
	
	waitq_sleep(&can_start);
	
	for (i = 0; i < ATTEMPTS; i++) {
		asm volatile (
			"mtc1 %0,$1"
			: "=r" (arg)
		);
		
		delay(DELAY);
		
		asm volatile (
			"mfc1 %0, $1"
			: "=r" (after_arg)
		);
		
		if (arg != after_arg) {
			TPRINTF("General reg tid%" PRIu64 ": arg(%d) != %d\n", THREAD->tid, arg, after_arg);
			atomic_inc(&threads_fault);
			break;
		}
	}
	atomic_inc(&threads_ok);
}

static void testit2(void *data)
{
	int i;
	int arg __attribute__((aligned(16))) = (int) ((unative_t) data);
	int after_arg __attribute__((aligned(16)));
	
	thread_detach(THREAD);
	
	waitq_sleep(&can_start);
	
	for (i = 0; i < ATTEMPTS; i++) {
		asm volatile (
			"mtc1 %0,$1"
			: "=r" (arg)
		);
		
		scheduler();
		asm volatile (
			"mfc1 %0,$1"
			: "=r" (after_arg)
		);
		
		if (arg != after_arg) {
			TPRINTF("General reg tid%" PRIu64 ": arg(%d) != %d\n", THREAD->tid, arg, after_arg);
			atomic_inc(&threads_fault);
			break;
		}
	}
	atomic_inc(&threads_ok);
}


char *test_mips2(void)
{
	unsigned int i, total = 0;
	
	waitq_initialize(&can_start);
	atomic_set(&threads_ok, 0);
	atomic_set(&threads_fault, 0);
	
	TPRINTF("Creating %u threads... ", 2 * THREADS);
	
	for (i = 0; i < THREADS; i++) {
		thread_t *t;
		
		if (!(t = thread_create(testit1, (void *) ((unative_t) 2 * i), TASK, 0, "testit1", false))) {
			TPRINTF("could not create thread %u\n", 2 * i);
			break;
		}
		thread_ready(t);
		total++;
		
		if (!(t = thread_create(testit2, (void *) ((unative_t) 2 * i + 1), TASK, 0, "testit2", false))) {
			TPRINTF("could not create thread %u\n", 2 * i + 1);
			break;
		}
		thread_ready(t);
		total++;
	}
	
	TPRINTF("ok\n");
		
	thread_sleep(1);
	waitq_wakeup(&can_start, WAKEUP_ALL);
	
	while (atomic_get(&threads_ok) != (long) total) {
		TPRINTF("Threads left: %d\n", total - atomic_get(&threads_ok));
		thread_sleep(1);
	}
	
	if (atomic_get(&threads_fault) == 0)
		return NULL;
	
	return "Test failed";
}
