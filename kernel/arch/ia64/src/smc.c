/*
 * SPDX-FileCopyrightText: 2005 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <barrier.h>
#include <arch/barrier.h>

#define FC_INVAL_MIN		32

void smc_coherence(void *a, size_t l)
{
	unsigned long i;
	for (i = 0; i < (l); i += FC_INVAL_MIN)
		fc_i(a + i);
	sync_i();
	srlz_i();
}
