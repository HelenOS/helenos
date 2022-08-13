/*
 * SPDX-FileCopyrightText: 2012 Adam Hraska
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_arm32
 * @{
 */
/** @file
 *  @brief Atomic operations emulation.
 */

#include <synch/spinlock.h>
#include <arch/barrier.h>
#include <arch/asm.h>

unsigned __atomic_fetch_add_4(volatile unsigned *mem, unsigned val, int model)
{
	/*
	 * This implementation is for UP pre-ARMv6 systems where we do not have
	 * the LDREX and STREX instructions.
	 */
	ipl_t ipl = interrupts_disable();
	unsigned ret = *mem;
	*mem += val;
	interrupts_restore(ipl);
	return ret;
}

unsigned __atomic_fetch_sub_4(volatile unsigned *mem, unsigned val, int model)
{
	ipl_t ipl = interrupts_disable();
	unsigned ret = *mem;
	*mem -= val;
	interrupts_restore(ipl);
	return ret;
}

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

void __sync_synchronize(void)
{
	dsb();
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
