/*
 * Copyright (c) 2001-2004 Jakub Jermar
 * Copyright (c) 2012      Adam Hraska
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

/** @addtogroup amd64
 * @{
 */
/** @file
 */

#ifndef KERN_amd64_ATOMIC_H_
#define KERN_amd64_ATOMIC_H_

#include <arch/barrier.h>
#include <preemption.h>
#include <trace.h>

NO_TRACE static inline void atomic_inc(atomic_t *val)
{
#ifdef CONFIG_SMP
	asm volatile (
	    "lock incq %[count]\n"
	    : [count] "+m" (val->count)
	);
#else
	asm volatile (
	    "incq %[count]\n"
	    : [count] "+m" (val->count)
	);
#endif /* CONFIG_SMP */
}

NO_TRACE static inline void atomic_dec(atomic_t *val)
{
#ifdef CONFIG_SMP
	asm volatile (
	    "lock decq %[count]\n"
	    : [count] "+m" (val->count)
	);
#else
	asm volatile (
	    "decq %[count]\n"
	    : [count] "+m" (val->count)
	);
#endif /* CONFIG_SMP */
}

NO_TRACE static inline atomic_count_t atomic_postinc(atomic_t *val)
{
	atomic_count_t r = 1;

	asm volatile (
	    "lock xaddq %[r], %[count]\n"
	    : [count] "+m" (val->count),
	      [r] "+r" (r)
	);

	return r;
}

NO_TRACE static inline atomic_count_t atomic_postdec(atomic_t *val)
{
	atomic_count_t r = -1;

	asm volatile (
	    "lock xaddq %[r], %[count]\n"
	    : [count] "+m" (val->count),
	      [r] "+r" (r)
	);

	return r;
}

#define atomic_preinc(val)  (atomic_postinc(val) + 1)
#define atomic_predec(val)  (atomic_postdec(val) - 1)

NO_TRACE static inline atomic_count_t test_and_set(atomic_t *val)
{
	atomic_count_t v = 1;

	asm volatile (
	    "xchgq %[v], %[count]\n"
	    : [v] "+r" (v),
	      [count] "+m" (val->count)
	);

	return v;
}

/** amd64 specific fast spinlock */
NO_TRACE static inline void atomic_lock_arch(atomic_t *val)
{
	atomic_count_t tmp;

	preemption_disable();
	asm volatile (
	    "0:\n"
	    "	pause\n"
	    "	mov %[count], %[tmp]\n"
	    "	testq %[tmp], %[tmp]\n"
	    "	jnz 0b\n"       /* lightweight looping on locked spinlock */

	    "	incq %[tmp]\n"  /* now use the atomic operation */
	    "	xchgq %[count], %[tmp]\n"
	    "	testq %[tmp], %[tmp]\n"
	    "	jnz 0b\n"
	    : [count] "+m" (val->count),
	      [tmp] "=&r" (tmp)
	);

	/*
	 * Prevent critical section code from bleeding out this way up.
	 */
	CS_ENTER_BARRIER();
}


#define _atomic_cas_impl(pptr, exp_val, new_val, old_val, prefix) \
({ \
	switch (sizeof(typeof(*(pptr)))) { \
	case 1: \
		asm volatile ( \
			prefix " cmpxchgb %[newval], %[ptr]\n" \
			: /* Output operands. */ \
			/* Old/current value is returned in eax. */ \
			[oldval] "=a" (old_val), \
			/* (*ptr) will be read and written to, hence "+" */ \
			[ptr] "+m" (*pptr) \
			: /* Input operands. */ \
			/* Expected value must be in eax. */ \
			[expval] "a" (exp_val), \
			/* The new value may be in any register. */ \
			[newval] "r" (new_val) \
			: "memory" \
		); \
		break; \
	case 2: \
		asm volatile ( \
			prefix " cmpxchgw %[newval], %[ptr]\n" \
			: /* Output operands. */ \
			/* Old/current value is returned in eax. */ \
			[oldval] "=a" (old_val), \
			/* (*ptr) will be read and written to, hence "+" */ \
			[ptr] "+m" (*pptr) \
			: /* Input operands. */ \
			/* Expected value must be in eax. */ \
			[expval] "a" (exp_val), \
			/* The new value may be in any register. */ \
			[newval] "r" (new_val) \
			: "memory" \
		); \
		break; \
	case 4: \
		asm volatile ( \
			prefix " cmpxchgl %[newval], %[ptr]\n" \
			: /* Output operands. */ \
			/* Old/current value is returned in eax. */ \
			[oldval] "=a" (old_val), \
			/* (*ptr) will be read and written to, hence "+" */ \
			[ptr] "+m" (*pptr) \
			: /* Input operands. */ \
			/* Expected value must be in eax. */ \
			[expval] "a" (exp_val), \
			/* The new value may be in any register. */ \
			[newval] "r" (new_val) \
			: "memory" \
		); \
		break; \
	case 8: \
		asm volatile ( \
			prefix " cmpxchgq %[newval], %[ptr]\n" \
			: /* Output operands. */ \
			/* Old/current value is returned in eax. */ \
			[oldval] "=a" (old_val), \
			/* (*ptr) will be read and written to, hence "+" */ \
			[ptr] "+m" (*pptr) \
			: /* Input operands. */ \
			/* Expected value must be in eax. */ \
			[expval] "a" (exp_val), \
			/* The new value may be in any register. */ \
			[newval] "r" (new_val) \
			: "memory" \
		); \
		break; \
	} \
})


#ifndef local_atomic_cas

#define local_atomic_cas(pptr, exp_val, new_val) \
({ \
	/* Use proper types and avoid name clashes */ \
	typeof(*(pptr)) _old_val_cas; \
	typeof(*(pptr)) _exp_val_cas = exp_val; \
	typeof(*(pptr)) _new_val_cas = new_val; \
	_atomic_cas_impl(pptr, _exp_val_cas, _new_val_cas, _old_val_cas, ""); \
	\
	_old_val_cas; \
})

#else
/* Check if arch/atomic.h does not accidentally include /atomic.h .*/
#error Architecture specific cpu local atomics already defined! Check your includes.
#endif


#ifndef local_atomic_exchange
/*
 * Issuing a xchg instruction always implies lock prefix semantics.
 * Therefore, it is cheaper to use a cmpxchg without a lock prefix
 * in a loop.
 */
#define local_atomic_exchange(pptr, new_val) \
({ \
	/* Use proper types and avoid name clashes */ \
	typeof(*(pptr)) _exp_val_x; \
	typeof(*(pptr)) _old_val_x; \
	typeof(*(pptr)) _new_val_x = new_val; \
	\
	do { \
		_exp_val_x = *pptr; \
		_old_val_x = local_atomic_cas(pptr, _exp_val_x, _new_val_x); \
	} while (_old_val_x != _exp_val_x); \
	\
	_old_val_x; \
})

#else
/* Check if arch/atomic.h does not accidentally include /atomic.h .*/
#error Architecture specific cpu local atomics already defined! Check your includes.
#endif


#endif

/** @}
 */
