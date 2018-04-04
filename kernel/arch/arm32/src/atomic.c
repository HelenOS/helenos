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

/** @addtogroup arm32
 * @{
 */
/** @file
 *  @brief Atomic operations emulation.
 */

#include <synch/spinlock.h>


IRQ_SPINLOCK_STATIC_INITIALIZE_NAME(cas_lock, "arm-cas-lock");

/** Implements GCC's missing compare-and-swap intrinsic for ARM.
 *
 * Sets \a *ptr to \a new_val if it is equal to \a expected. In any case,
 * returns the previous value of \a *ptr.
 */
void *__sync_val_compare_and_swap_4(void **ptr, void *expected, void *new_val)
{
	/*
	 * Using an interrupt disabling spinlock might still lead to deadlock
	 * if CAS() is used in an exception handler. Eg. if a CAS() results
	 * in a page fault exception and the exception handler again tries
	 * to invoke CAS() (even for a different memory location), the spinlock
	 * would deadlock.
	 */
	irq_spinlock_lock(&cas_lock, true);

	void *cur_val = *ptr;

	if (cur_val == expected) {
		*ptr = new_val;
	}

	irq_spinlock_unlock(&cas_lock, true);

	return cur_val;
}

/* Naive implementations of the newer intrinsics. */

_Bool __atomic_compare_exchange_4(void **mem, void **expected, void *desired, _Bool weak, int success, int failure)
{
	(void) weak;
	(void) success;
	(void) failure;

	void *old = *expected;
	void *new = __sync_val_compare_and_swap_4(mem, old, desired);
	if (old == new) {
		return 1;
	} else {
		*expected = new;
		return 0;
	}
}

void *__atomic_exchange_4(void **mem, void *val, int model)
{
	(void) model;

	irq_spinlock_lock(&cas_lock, true);
	void *old = *mem;
	*mem = val;
	irq_spinlock_unlock(&cas_lock, true);

	return old;
}

/** @}
 */
