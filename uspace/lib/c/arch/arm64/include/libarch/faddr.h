/*
 * SPDX-FileCopyrightText: 2015 Petr Pavlu
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libcarm64
 * @{
 */
/** @file
 * @brief Function address conversion.
 */

#ifndef _LIBC_arm64_FADDR_H_
#define _LIBC_arm64_FADDR_H_

#include <types/common.h>

/** Calculate absolute address of function referenced by fptr pointer.
 *
 * @param f Function pointer.
 */
#define FADDR(f)  ((uintptr_t) (f))

#endif

/** @}
 */
