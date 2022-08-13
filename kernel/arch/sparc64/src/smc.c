/*
 * SPDX-FileCopyrightText: 2005 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <barrier.h>
#include <arch/barrier.h>

#if defined(US)

#define FLUSH_INVAL_MIN  4

void smc_coherence(void *a, size_t l)
{
	asm volatile ("membar #StoreStore\n" ::: "memory");

	for (size_t i = 0; i < l; i += FLUSH_INVAL_MIN) {
		asm volatile (
		    "flush %[reg]\n"
		    :: [reg] "r" (a + i)
		    : "memory"
		);
	}
}

#elif defined (US3)

void smc_coherence(void *a, size_t l)
{
	asm volatile ("membar #StoreStore\n" ::: "memory");

	flush_pipeline();
}

#endif  /* defined(US3) */
