/*
 * Copyright (c) 2006 Jakub Jermar
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

/** @addtogroup kernel_generic
 * @{
 */
/** @file
 */

#ifndef KERN_ATOMIC_H_
#define KERN_ATOMIC_H_

#include <stdbool.h>
#include <typedefs.h>
#include <stdatomic.h>

/*
 * Shorthand for relaxed atomic read/write, something that's needed to formally
 * avoid undefined behavior in cases where we need to read a variable in
 * different threads and we don't particularly care about ordering
 * (e.g. statistic printouts). This is most likely translated into the same
 * assembly instructions as regular read/writes.
 */
#define atomic_set_unordered(var, val) atomic_store_explicit((var), (val), memory_order_relaxed)
#define atomic_get_unordered(var) atomic_load_explicit((var), memory_order_relaxed)

#define atomic_predec(val) \
	(atomic_fetch_sub((val), 1) - 1)

#define atomic_preinc(val) \
	(atomic_fetch_add((val), 1) + 1)

#define atomic_postdec(val) \
	atomic_fetch_sub((val), 1)

#define atomic_postinc(val) \
	atomic_fetch_add((val), 1)

#define atomic_dec(val) \
	((void) atomic_fetch_sub(val, 1))

#define atomic_inc(val) \
	((void) atomic_fetch_add(val, 1))

#define local_atomic_exchange(var_addr, new_val) \
	atomic_exchange_explicit( \
	    (_Atomic typeof(*(var_addr)) *) (var_addr), \
	    (new_val), memory_order_relaxed)

#if __64_BITS__

typedef struct {
	atomic_uint_fast64_t value;
} atomic_time_stat_t;

#define ATOMIC_TIME_INITIALIZER() (atomic_time_stat_t) {}

static inline void atomic_time_increment(atomic_time_stat_t *time, int a)
{
	/*
	 * We require increments to be synchronized with each other, so we
	 * can use ordinary reads and writes instead of a more expensive atomic
	 * read-modify-write operations.
	 */
	uint64_t v = atomic_load_explicit(&time->value, memory_order_relaxed);
	atomic_store_explicit(&time->value, v + a, memory_order_relaxed);
}

static inline uint64_t atomic_time_read(atomic_time_stat_t *time)
{
	return atomic_load_explicit(&time->value, memory_order_relaxed);
}

#else

/**
 * A monotonically increasing 64b time statistic.
 * Increments must be synchronized with each other (or limited to a single
 * thread/CPU), but reads can be performed from any thread.
 *
 */
typedef struct {
	uint64_t true_value;
	atomic_uint_fast32_t high1;
	atomic_uint_fast32_t high2;
	atomic_uint_fast32_t low;
} atomic_time_stat_t;

#define ATOMIC_TIME_INITIALIZER() (atomic_time_stat_t) {}

static inline void atomic_time_increment(atomic_time_stat_t *time, int a)
{
	/*
	 * On 32b architectures, we can't rely on 64b memory reads/writes being
	 * architecturally atomic, but we also don't want to pay the cost of
	 * emulating atomic reads/writes, so instead we split value in half
	 * and perform some ordering magic to make sure readers always get
	 * consistent value.
	 */

	/* true_value is only used by the writer, so this need not be atomic. */
	uint64_t val = time->true_value;
	uint32_t old_high = val >> 32;
	val += a;
	uint32_t new_high = val >> 32;
	time->true_value = val;

	/* Tell GCC that the first branch is far more likely than the second. */
	if (__builtin_expect(old_high == new_high, 1)) {
		/* If the high half didn't change, we need not bother with barriers. */
		atomic_store_explicit(&time->low, (uint32_t) val, memory_order_relaxed);
	} else {
		/*
		 * If both halves changed, extra ordering is necessary.
		 * The idea is that if reader reads high1 and high2 with the same value,
		 * it is guaranteed that they read the correct low half for that value.
		 *
		 * This is the same sequence that is used by userspace to read clock.
		 */
		atomic_store_explicit(&time->high1, new_high, memory_order_relaxed);
		atomic_store_explicit(&time->low, (uint32_t) val, memory_order_release);
		atomic_store_explicit(&time->high2, new_high, memory_order_release);
	}
}

static inline uint64_t atomic_time_read(atomic_time_stat_t *time)
{
	uint32_t high2 = atomic_load_explicit(&time->high2, memory_order_acquire);
	uint32_t low = atomic_load_explicit(&time->low, memory_order_acquire);
	uint32_t high1 = atomic_load_explicit(&time->high1, memory_order_relaxed);

	if (high1 != high2)
		low = 0;

	/* If the values differ, high1 is always the newer value. */
	return (uint64_t) high1 << 32 | (uint64_t) low;
}

#endif /* __64_BITS__ */

#endif

/** @}
 */
