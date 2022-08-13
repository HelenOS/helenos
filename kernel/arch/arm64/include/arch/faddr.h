/*
 * SPDX-FileCopyrightText: 2015 Petr Pavlu
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_arm64
 * @{
 */
/** @file
 * @brief Function address conversion.
 */

#ifndef KERN_arm64_FADDR_H_
#define KERN_arm64_FADDR_H_

#include <typedefs.h>

/** Calculate absolute address of function referenced by fptr pointer.
 *
 * @param fptr Function pointer.
 */
#define FADDR(fptr)  ((uintptr_t) (fptr))

#endif

/** @}
 */
