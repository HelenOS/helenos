/*
 * SPDX-FileCopyrightText: 2005 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <barrier.h>

void smc_coherence(void *a, size_t l)
{
	// TODO
	compiler_barrier();
}
