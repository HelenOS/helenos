/*
 * SPDX-FileCopyrightText: 2015 Petr Pavlu
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_arm64
 * @{
 */
/** @file
 */

#ifdef CONFIG_SMP

#include <smp/ipi.h>
#include <panic.h>

/** Deliver IPI to all processors except the current one.
 *
 * @param ipi IPI number.
 */
void ipi_broadcast_arch(int ipi)
{
	panic("broadcast IPI not implemented.");
}

#endif /* CONFIG_SMP */

/** @}
 */
