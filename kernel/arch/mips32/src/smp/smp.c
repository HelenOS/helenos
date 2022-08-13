/*
 * SPDX-FileCopyrightText: 2009 Martin Decky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_mips32
 * @{
 */
/** @file
 */

#include <config.h>
#include <smp/smp.h>
#include <arch/arch.h>

#ifdef CONFIG_SMP

void smp_init(void)
{
	config.cpu_count = cpu_count;
}

void kmp(void *arg __attribute__((unused)))
{
}

#endif /* CONFIG_SMP */

/** @}
 */
