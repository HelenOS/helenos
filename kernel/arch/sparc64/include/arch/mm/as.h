/*
 * SPDX-FileCopyrightText: 2005 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_sparc64_mm
 * @{
 */
/** @file
 */

#ifndef KERN_sparc64_AS_H_
#define KERN_sparc64_AS_H_

#if defined (SUN4U)

#include <arch/mm/sun4u/as.h>

#elif defined (SUN4V)

#include <arch/mm/sun4v/as.h>

#endif

#endif

/** @}
 */
