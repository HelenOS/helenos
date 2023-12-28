/*
 * Copyright (c) 2007 Michal Kebrt
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

/*
 * Older ARMs don't have atomic instructions, so we need to define a bunch
 * of symbols for GCC to use.
 */

#include <stdbool.h>
#include "ras_page.h"

volatile unsigned *ras_page;

bool __atomic_compare_exchange_4(volatile void *mem0, void *expected0,
    unsigned desired, bool weak, int success, int failure)
{
	volatile unsigned *mem = mem0;
	unsigned *expected = expected0;

	(void) success;
	(void) failure;
	(void) weak;

	unsigned ov = *expected;
	unsigned ret;

	/*
	 * The following instructions between labels 1 and 2 constitute a
	 * Restartable Atomic Sequence. Should the sequence be non-atomic,
	 * the kernel will restart it.
	 */
	asm volatile (
	    "1:\n"
	    "	adr %[ret], 1b\n"
	    "	str %[ret], %[rp0]\n"
	    "	adr %[ret], 2f\n"
	    "	str %[ret], %[rp1]\n"

	    "	ldr %[ret], %[addr]\n"
	    "	cmp %[ret], %[ov]\n"
	    "	streq %[nv], %[addr]\n"
	    "2:\n"
	    : [ret] "=&r" (ret),
	      [rp0] "=m" (ras_page[0]),
	      [rp1] "=m" (ras_page[1]),
	      [addr] "+m" (*mem)
	    : [ov] "r" (ov),
	      [nv] "r" (desired)
	    : "memory"
	);

	ras_page[0] = 0;
	ras_page[1] = 0xffffffff;

	if (ret == ov)
		return true;

	*expected = ret;
	return false;
}

unsigned short __atomic_fetch_add_2(volatile void *mem0, unsigned short val,
    int model)
{
	volatile unsigned short *mem = mem0;

	(void) model;

	unsigned short ret;

	/*
	 * The following instructions between labels 1 and 2 constitute a
	 * Restartable Atomic Seqeunce. Should the sequence be non-atomic,
	 * the kernel will restart it.
	 */
	asm volatile (
	    "1:\n"
	    "	adr %[ret], 1b\n"
	    "	str %[ret], %[rp0]\n"
	    "	adr %[ret], 2f\n"
	    "	str %[ret], %[rp1]\n"
	    "	ldrh %[ret], %[addr]\n"
	    "	add %[ret], %[ret], %[imm]\n"
	    "	strh %[ret], %[addr]\n"
	    "2:\n"
	    : [ret] "=&r" (ret),
	      [rp0] "=m" (ras_page[0]),
	      [rp1] "=m" (ras_page[1]),
	      [addr] "+m" (*mem)
	    : [imm] "r" (val)
	);

	ras_page[0] = 0;
	ras_page[1] = 0xffffffff;

	return ret - val;
}

unsigned __atomic_fetch_add_4(volatile void *mem0, unsigned val, int model)
{
	volatile unsigned *mem = mem0;

	(void) model;

	unsigned ret;

	/*
	 * The following instructions between labels 1 and 2 constitute a
	 * Restartable Atomic Seqeunce. Should the sequence be non-atomic,
	 * the kernel will restart it.
	 */
	asm volatile (
	    "1:\n"
	    "	adr %[ret], 1b\n"
	    "	str %[ret], %[rp0]\n"
	    "	adr %[ret], 2f\n"
	    "	str %[ret], %[rp1]\n"
	    "	ldr %[ret], %[addr]\n"
	    "	add %[ret], %[ret], %[imm]\n"
	    "	str %[ret], %[addr]\n"
	    "2:\n"
	    : [ret] "=&r" (ret),
	      [rp0] "=m" (ras_page[0]),
	      [rp1] "=m" (ras_page[1]),
	      [addr] "+m" (*mem)
	    : [imm] "r" (val)
	);

	ras_page[0] = 0;
	ras_page[1] = 0xffffffff;

	return ret - val;
}

unsigned __atomic_fetch_sub_4(volatile void *mem, unsigned val, int model)
{
	return __atomic_fetch_add_4(mem, -val, model);
}

void __sync_synchronize(void)
{
	// FIXME: Full memory barrier. We might need a syscall for this.
}

unsigned __sync_add_and_fetch_4(volatile void *vptr, unsigned val)
{
	return __atomic_fetch_add_4(vptr, val, __ATOMIC_SEQ_CST) + val;
}

unsigned __sync_sub_and_fetch_4(volatile void *vptr, unsigned val)
{
	return __atomic_fetch_sub_4(vptr, val, __ATOMIC_SEQ_CST) - val;
}

bool __sync_bool_compare_and_swap_4(volatile void *ptr, unsigned old_val, unsigned new_val)
{
	return __atomic_compare_exchange_4(ptr, &old_val, new_val, false,
	    __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
}

unsigned __sync_val_compare_and_swap_4(volatile void *ptr, unsigned old_val, unsigned new_val)
{
	__atomic_compare_exchange_4(ptr, &old_val, new_val, false,
	    __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
	return old_val;
}
