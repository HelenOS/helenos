/*
 * SPDX-FileCopyrightText: 2005 Jakub Vana
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_ia64
 * @{
 */
/** @file
 */

#include <smp/smp.h>
#include <smp/ipi.h>

#ifdef CONFIG_SMP

void ipi_broadcast_arch(int ipi)
{
}

void smp_init(void)
{
}

void kmp(void *arg __attribute__((unused)))
{
}

#endif

/** @}
 */
