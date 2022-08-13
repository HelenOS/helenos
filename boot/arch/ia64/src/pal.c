/*
 * SPDX-FileCopyrightText: 2011 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <arch/pal.h>
#include <arch/types.h>

uint64_t pal_proc = 0;

uint64_t pal_proc_freq_ratio(void)
{
	uint64_t proc_ratio;

	pal_static_call_0_1(PAL_FREQ_RATIOS, &proc_ratio);

	return proc_ratio;
}
