/*
 * SPDX-FileCopyrightText: 2006 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_sparc64_mm
 * @{
 */
/** @file
 */

#ifndef KERN_sparc64_TSB_H_
#define KERN_sparc64_TSB_H_

#if defined (SUN4U)
#include <arch/mm/sun4u/tsb.h>
#elif defined (SUN4V)
#include <arch/mm/sun4v/tsb.h>
#endif

#endif

/** @}
 */
