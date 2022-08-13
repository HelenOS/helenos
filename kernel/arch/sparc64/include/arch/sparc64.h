/*
 * SPDX-FileCopyrightText: 2005 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_sparc64
 * @{
 */
/** @file
 */

#ifndef KERN_sparc64_SPARC64_H_
#define KERN_sparc64_SPARC64_H_

#include <interrupt.h>

extern void interrupt_register(unsigned int, const char *, iroutine_t);

#endif

/** @}
 */
