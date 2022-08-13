/*
 * SPDX-FileCopyrightText: 2005 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_ia32
 * @{
 */
/** @file
 */

#ifdef CONFIG_SMP

#include <smp/ipi.h>
#include <arch/smp/apic.h>

void ipi_broadcast_arch(int ipi)
{
	(void) l_apic_broadcast_custom_ipi((uint8_t) ipi);
}

#endif /* CONFIG_SMP */

/** @}
 */
