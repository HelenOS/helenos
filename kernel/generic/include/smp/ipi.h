/*
 * SPDX-FileCopyrightText: 2005 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_generic
 * @{
 */
/** @file
 */

#ifndef KERN_IPI_H_
#define KERN_IPI_H_

#ifdef CONFIG_SMP

extern void ipi_broadcast(int);
extern void ipi_broadcast_arch(int);

#else

#define ipi_broadcast(ipi)

#endif /* CONFIG_SMP */

#endif

/** @}
 */
