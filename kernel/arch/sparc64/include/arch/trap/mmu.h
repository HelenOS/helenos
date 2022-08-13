/*
 * SPDX-FileCopyrightText: 2006 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_sparc64_interrupt
 * @{
 */
/**
 * @file
 * @brief This file contains fast MMU trap handlers.
 */

#ifndef KERN_sparc64_MMU_TRAP_H_
#define KERN_sparc64_MMU_TRAP_H_

#if defined (SUN4U)
#include <arch/trap/sun4u/mmu.h>
#elif defined (SUN4V)
#include <arch/trap/sun4v/mmu.h>
#endif

#endif

/** @}
 */
