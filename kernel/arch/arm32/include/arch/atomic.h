/*
 * Copyright (c) 2007 Michal Kebrt
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
 *  @brief Atomic operations.
 */

#ifndef KERN_arm32_ATOMIC_H_
#define KERN_arm32_ATOMIC_H_

#include <arch/asm.h>
#include <trace.h>

/** Atomic addition.
 *
 * @param val Where to add.
 * @param i   Value to be added.
 *
 * @return Value after addition.
 *
 */
NO_TRACE static inline atomic_count_t atomic_add(atomic_t *val,
    atomic_count_t i)
{
	/*
	 * This implementation is for UP pre-ARMv6 systems where we do not have
	 * the LDREX and STREX instructions.
	 */
	ipl_t ipl = interrupts_disable();
	val->count += i;
	atomic_count_t ret = val->count;
	interrupts_restore(ipl);

	return ret;
}

/** Atomic increment.
 *
 * @param val Variable to be incremented.
 *
 */
NO_TRACE static inline void atomic_inc(atomic_t *val)
{
	atomic_add(val, 1);
}

/** Atomic decrement.
 *
 * @param val Variable to be decremented.
 *
 */
NO_TRACE static inline void atomic_dec(atomic_t *val)
{
	atomic_add(val, -1);
}

/** Atomic pre-increment.
 *
 * @param val Variable to be incremented.
 * @return    Value after incrementation.
 *
 */
NO_TRACE static inline atomic_count_t atomic_preinc(atomic_t *val)
{
	return atomic_add(val, 1);
}

/** Atomic pre-decrement.
 *
 * @param val Variable to be decremented.
 * @return    Value after decrementation.
 *
 */
NO_TRACE static inline atomic_count_t atomic_predec(atomic_t *val)
{
	return atomic_add(val, -1);
}

/** Atomic post-increment.
 *
 * @param val Variable to be incremented.
 * @return    Value before incrementation.
 *
 */
NO_TRACE static inline atomic_count_t atomic_postinc(atomic_t *val)
{
	return atomic_add(val, 1) - 1;
}

/** Atomic post-decrement.
 *
 * @param val Variable to be decremented.
 * @return    Value before decrementation.
 *
 */
NO_TRACE static inline atomic_count_t atomic_postdec(atomic_t *val)
{
	return atomic_add(val, -1) + 1;
}

#endif

/** @}
 */
