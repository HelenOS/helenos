/*
 * SPDX-FileCopyrightText: 2001-2004 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <test.h>
#include <arch.h>
#include <atomic.h>
#include <proc/thread.h>
#include <synch/waitq.h>
#include <synch/semaphore.h>

#define AT_ONCE    3
#define PRODUCERS  50
#define CONSUMERS  50

static semaphore_t sem;

static waitq_t can_start;
static atomic_size_t items_produced;
static atomic_size_t items_consumed;

static void producer(void *arg)
{
	thread_detach(THREAD);

	waitq_sleep(&can_start);

	semaphore_down(&sem);
	atomic_inc(&items_produced);
	thread_usleep(250);
	semaphore_up(&sem);
}

static void consumer(void *arg)
{
	thread_detach(THREAD);

	waitq_sleep(&can_start);

	semaphore_down(&sem);
	atomic_inc(&items_consumed);
	thread_usleep(500);
	semaphore_up(&sem);
}

const char *test_semaphore1(void)
{
	int i, j, k;
	size_t consumers;
	size_t producers;

	waitq_initialize(&can_start);
	semaphore_initialize(&sem, AT_ONCE);

	for (i = 1; i <= 3; i++) {
		thread_t *thrd;

		atomic_store(&items_produced, 0);
		atomic_store(&items_consumed, 0);

		consumers = i * CONSUMERS;
		producers = (4 - i) * PRODUCERS;

		TPRINTF("Creating %zu consumers and %zu producers...",
		    consumers, producers);

		for (j = 0; j < (CONSUMERS + PRODUCERS) / 2; j++) {
			for (k = 0; k < i; k++) {
				thrd = thread_create(consumer, NULL, TASK,
				    THREAD_FLAG_NONE, "consumer");
				if (thrd)
					thread_ready(thrd);
				else
					TPRINTF("could not create consumer %d\n", i);
			}
			for (k = 0; k < (4 - i); k++) {
				thrd = thread_create(producer, NULL, TASK,
				    THREAD_FLAG_NONE, "producer");
				if (thrd)
					thread_ready(thrd);
				else
					TPRINTF("could not create producer %d\n", i);
			}
		}

		TPRINTF("ok\n");

		thread_sleep(1);
		waitq_wakeup(&can_start, WAKEUP_ALL);

		while ((items_consumed != consumers) || (items_produced != producers)) {
			TPRINTF("%zu consumers remaining, %zu producers remaining\n",
			    consumers - items_consumed, producers - items_produced);
			thread_sleep(1);
		}
	}

	return NULL;
}
