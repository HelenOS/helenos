/*
 * Copyright (c) 2012 Adam Hraska
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

/** @addtogroup liburcu
 * @{
 */
/**
 * @file
 * 
 * User space RCU is based on URCU utilizing signals [1]. This 
 * implementation does not however signal each thread of the process 
 * to issue a memory barrier. Instead, we introduced a syscall that
 * issues memory barriers (via IPIs) on cpus that are running threads
 * of the current process. First, it does not require us to schedule
 * and run every thread of the process. Second, IPIs are less intrusive 
 * than switching contexts and entering user space.
 * 
 * This algorithm is further modified to require a single instead of
 * two reader group changes per grace period. Signal-URCU flips
 * the reader group and waits for readers of the previous group 
 * twice in succession in order to wait for new readers that were
 * delayed and mistakenly associated with the previous reader group. 
 * The modified algorithm ensures that the new reader group is
 * always empty (by explicitly waiting for it to become empty).
 * Only then does it flip the reader group and wait for preexisting
 * readers of the old reader group (invariant of SRCU [2, 3]).
 * 
 * 
 * [1] User-level implementations of read-copy update,
 *     2012, appendix
 *     http://www.rdrop.com/users/paulmck/RCU/urcu-supp-accepted.2011.08.30a.pdf
 * 
 * [2] linux/kernel/srcu.c in Linux 3.5-rc2,
 *     2012
 *     http://tomoyo.sourceforge.jp/cgi-bin/lxr/source/kernel/srcu.c?v=linux-3.5-rc2-ccs-1.8.3
 *
 * [3] [RFC PATCH 5/5 single-thread-version] implement 
 *     per-domain single-thread state machine,
 *     2012, Lai
 *     https://lkml.org/lkml/2012/3/6/586
 */

#include "rcu.h"
#include <fibril_synch.h>
#include <fibril.h>
#include <stdio.h>
#include <compiler/barrier.h>
#include <libarch/barrier.h>
#include <adt/list.h>
#include <futex.h>
#include <macros.h>
#include <async.h>
#include <smp_memory_barrier.h>

#define RCU_SLEEP_MS        10

#define RCU_NESTING_SHIFT   1
#define RCU_NESTING_INC     (1 << RCU_NESTING_SHIFT)
#define RCU_GROUP_BIT_MASK  (size_t)(RCU_NESTING_INC - 1)
#define RCU_GROUP_A         (size_t)(0 | RCU_NESTING_INC)
#define RCU_GROUP_B         (size_t)(1 | RCU_NESTING_INC)


typedef struct rcu_fibril_data {
	size_t nesting_cnt;
	link_t link;
} rcu_fibril_data_t;

typedef struct rcu_data {
	fibril_mutex_t mtx;
	size_t reader_group;
	futex_t list_futex;
	list_t fibrils_list;
} rcu_data_t;


static fibril_local rcu_fibril_data_t rcu_fibril = {
	.nesting_cnt = 0	
};

static rcu_data_t rcu = {
	.mtx = FIBRIL_MUTEX_INITIALIZER(rcu.mtx),
	.reader_group = RCU_GROUP_A,
	.list_futex = FUTEX_INITIALIZER,
	.fibrils_list = LIST_INITIALIZER(rcu.fibrils_list),
};


static void wait_for_readers(size_t reader_group);
static void force_mb_in_all_threads(void);
static bool is_preexisting_reader(const rcu_fibril_data_t *fib, size_t group);

static bool is_in_group(size_t nesting_cnt, size_t group);
static bool is_in_reader_section(size_t nesting_cnt);
static size_t get_other_group(size_t group);


/** Registers a fibril so it may start using RCU read sections.
 * 
 * A fibril must be registered with rcu before it can enter RCU critical
 * sections delineated by rcu_read_lock() and rcu_read_unlock().
 */
void rcu_register_fibril(void)
{
	futex_down(&rcu.list_futex);
	list_append(&rcu_fibril.link, &rcu.fibrils_list);
	futex_up(&rcu.list_futex);
}

/** Deregisters a fibril that had been using RCU read sections.
 * 
 * A fibril must be deregistered before it exits if it had
 * been registered with rcu via rcu_register_fibril().
 */
void rcu_deregister_fibril(void)
{
	/* 
	 * Forcefully unlock any reader sections. The fibril is exiting
	 * so it is not holding any references to data protected by the
	 * rcu section. Therefore, it is safe to unlock. Otherwise, 
	 * rcu_synchronize() would wait indefinitely.
	 */
	memory_barrier();
	rcu_fibril.nesting_cnt = 0;
	
	futex_down(&rcu.list_futex);
	list_remove(&rcu_fibril.link);
	futex_up(&rcu.list_futex);
}

/** Delimits the start of an RCU reader critical section. 
 * 
 * RCU reader sections may be nested.  
 */
void rcu_read_lock(void)
{
	size_t nesting_cnt = ACCESS_ONCE(rcu_fibril.nesting_cnt);
	
	if (0 == (nesting_cnt >> RCU_NESTING_SHIFT)) {
		ACCESS_ONCE(rcu_fibril.nesting_cnt) = ACCESS_ONCE(rcu.reader_group);
		/* Required by MB_FORCE_L */
		compiler_barrier(); /* CC_BAR_L */
	} else {
		ACCESS_ONCE(rcu_fibril.nesting_cnt) = nesting_cnt + RCU_NESTING_INC;
	}
}

/** Delimits the start of an RCU reader critical section. */
void rcu_read_unlock(void)
{
	/* Required by MB_FORCE_U */
	compiler_barrier(); /* CC_BAR_U */
	/* todo: ACCESS_ONCE(nesting_cnt) ? */
	rcu_fibril.nesting_cnt -= RCU_NESTING_INC;
}

/** Blocks until all preexisting readers exit their critical sections. */
void rcu_synchronize(void)
{
	/* 
	 * Pairs up with MB_FORCE_L (ie CC_BAR_L). Makes changes prior 
	 * to rcu_synchronize() visible to new readers. 
	 */
	memory_barrier(); /* MB_A */
	
	/* todo: early exit for batched sync()s */
	fibril_mutex_lock(&rcu.mtx);
	
	/* 
	 * Pairs up with MB_A. 
	 * 
	 * If the memory barrier is issued before CC_BAR_L in the target
	 * thread, it pairs up with MB_A and the thread sees all changes
	 * prior to rcu_synchronize(). Ie any reader sections are new
	 * rcu readers.  
	 * 
	 * If the memory barrier is issued after CC_BAR_L, it pairs up
	 * with MB_B and it will make the most recent nesting_cnt visible
	 * in this thread. Since the reader may have already accessed
	 * memory protected by RCU (it ran instructions passed CC_BAR_L),
	 * it is a preexisting reader. Seeing the most recent nesting_cnt 
	 * ensures the thread will be identified as a preexisting reader
	 * and we will wait for it in wait_for_readers(old_reader_group).
	 */
	force_mb_in_all_threads(); /* MB_FORCE_L */
	
	/* 
	 * Pairs with MB_FORCE_L (ie CC_BAR_L, CC_BAR_U) and makes the most
	 * current fibril.nesting_cnt visible to this cpu.
	 */
	read_barrier(); /* MB_B */
	
	size_t new_reader_group = get_other_group(rcu.reader_group);
	wait_for_readers(new_reader_group);
	
	/* Separates waiting for readers in new_reader_group from group flip. */
	memory_barrier();
	
	/* Flip the group new readers should associate with. */
	size_t old_reader_group = rcu.reader_group;
	rcu.reader_group = new_reader_group;

	/* Flip the group before waiting for preexisting readers in the old group.*/
	memory_barrier();
	
	wait_for_readers(old_reader_group);
	
	/* MB_FORCE_U  */
	force_mb_in_all_threads(); /* MB_FORCE_U */
	
	fibril_mutex_unlock(&rcu.mtx);
}

/** Issues a memory barrier in each thread of this process. */
static void force_mb_in_all_threads(void)
{
	/* 
	 * Only issue barriers in running threads. The scheduler will 
	 * execute additional memory barriers when switching to threads
	 * of the process that are currently not running.
	 */
	smp_memory_barrier();
}

/** Waits for readers of reader_group to exit their readers sections. */
static void wait_for_readers(size_t reader_group)
{
	futex_down(&rcu.list_futex);
	
	list_t quiescent_fibrils;
	list_initialize(&quiescent_fibrils);
	
	while (!list_empty(&rcu.fibrils_list)) {
		list_foreach_safe(rcu.fibrils_list, fibril_it, next_fibril) {
			rcu_fibril_data_t *fib = member_to_inst(fibril_it, 
				rcu_fibril_data_t, link);
			
			if (is_preexisting_reader(fib, reader_group)) {
				futex_up(&rcu.list_futex);
				async_usleep(RCU_SLEEP_MS * 1000);
				futex_down(&rcu.list_futex);
				break;
			} else {
				list_remove(fibril_it);
				list_append(fibril_it, &quiescent_fibrils);
			}
		}
	}
	
	list_concat(&rcu.fibrils_list, &quiescent_fibrils);
	futex_up(&rcu.list_futex);
}

static bool is_preexisting_reader(const rcu_fibril_data_t *fib, size_t group)
{
	size_t nesting_cnt = ACCESS_ONCE(fib->nesting_cnt);
	
	return is_in_group(nesting_cnt, group) && is_in_reader_section(nesting_cnt);
}

static size_t get_other_group(size_t group)
{
	if (group == RCU_GROUP_A) 
		return RCU_GROUP_B;
	else
		return RCU_GROUP_A;
}

static bool is_in_reader_section(size_t nesting_cnt)
{
	return RCU_NESTING_INC <= nesting_cnt;
}

static bool is_in_group(size_t nesting_cnt, size_t group)
{
	return (nesting_cnt & RCU_GROUP_BIT_MASK) == (group & RCU_GROUP_BIT_MASK);
}



/** @}
 */
