/*
 * SPDX-FileCopyrightText: 2005 Martin Decky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <barrier.h>

#define COHERENCE_INVAL_MIN  4

/*
 * The IMB sequence used here is valid for all possible cache models
 * on uniprocessor. SMP might require a different sequence.
 * See PowerPC Programming Environment for 32-Bit Microprocessors,
 * chapter 5.1.5.2
 */

void smc_coherence(void *addr, size_t len)
{
	unsigned int i;

	for (i = 0; i < len; i += COHERENCE_INVAL_MIN)
		asm volatile (
		    "dcbst 0, %[addr]\n"
		    :: [addr] "r" (addr + i)
		);

	asm volatile ("sync" ::: "memory");

	for (i = 0; i < len; i += COHERENCE_INVAL_MIN)
		asm volatile (
		    "icbi 0, %[addr]\n"
		    :: [addr] "r" (addr + i)
		);

	asm volatile ("sync" ::: "memory");
	asm volatile ("isync" ::: "memory");
}
