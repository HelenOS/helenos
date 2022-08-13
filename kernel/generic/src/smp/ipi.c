/*
 * SPDX-FileCopyrightText: 2005 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_generic
 * @{
 */

/**
 * @file
 * @brief Generic IPI interface.
 */

#ifdef CONFIG_SMP

#include <smp/ipi.h>
#include <config.h>

/** Broadcast IPI message
 *
 * Broadcast IPI message to all CPUs.
 *
 * @param ipi Message to broadcast.
 *
 */
void ipi_broadcast(int ipi)
{
	/*
	 * Provisions must be made to avoid sending IPI:
	 * - before all CPU's were configured to accept the IPI
	 * - if there is only one CPU but the kernel was compiled with CONFIG_SMP
	 */

	if (config.cpu_count > 1)
		ipi_broadcast_arch(ipi);
}

#endif /* CONFIG_SMP */

/** @}
 */
