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

#ifndef KERN_sparc64_ASID_H_
#define KERN_sparc64_ASID_H_

#include <stdint.h>

/*
 * On SPARC, Context means the same thing as ASID trough out the kernel.
 */
typedef uint16_t asid_t;

#define ASID_MAX_ARCH		8191	/* 2^13 - 1 */

#endif

/** @}
 */
