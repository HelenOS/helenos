/*
 * Copyright (c) 2018 CZ.NIC, z.s.p.o.
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

#include <atomic.h>

#ifdef PLATFORM_arm32

/*
 * Older ARMs don't have atomic instructions, so we need to define a bunch
 * of symbols for GCC to use.
 */

void __sync_synchronize(void)
{
	// FIXME: Full memory barrier. We need a syscall for this.
	// Should we implement this or is empty definition ok here?
}

unsigned __sync_add_and_fetch_4(volatile void *vptr, unsigned val)
{
	return atomic_add((atomic_t *)vptr, val);
}

unsigned __sync_sub_and_fetch_4(volatile void *vptr, unsigned val)
{
	return atomic_add((atomic_t *)vptr, -(atomic_signed_t)val);
}

bool __sync_bool_compare_and_swap_4(volatile void *ptr, unsigned old_val, unsigned new_val)
{
	return cas((atomic_t *)ptr, old_val, new_val);
}

unsigned __sync_val_compare_and_swap_4(volatile void *ptr, unsigned old_val, unsigned new_val)
{
	while (true) {
		if (__sync_bool_compare_and_swap_4(ptr, old_val, new_val)) {
			return old_val;
		}

		unsigned current = *(volatile unsigned *)ptr;
		if (current != old_val)
			return current;

		/* If the current value is the same as old_val, retry. */
	}
}

unsigned __atomic_fetch_add_4(unsigned *mem, unsigned val, int model)
{
	// TODO
	(void) model;

	return __sync_add_and_fetch_4(mem, val) - val;
}

unsigned __atomic_fetch_sub_4(unsigned *mem, unsigned val, int model)
{
	// TODO
	(void) model;

	return __sync_sub_and_fetch_4(mem, val) + val;
}

bool __atomic_compare_exchange_4(unsigned *mem, unsigned *expected, unsigned desired, bool weak, int success, int failure)
{
	// TODO
	(void) success;
	(void) failure;
	(void) weak;

	unsigned old = __sync_val_compare_and_swap_4(mem, *expected, desired);
	if (old == *expected)
		return true;

	*expected = old;
	return false;
}

#endif
