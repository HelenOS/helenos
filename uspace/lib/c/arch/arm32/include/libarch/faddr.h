/*
 * SPDX-FileCopyrightText: 2007 Michal Kebrt
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libcarm32
 * @{
 */
/** @file
 *  @brief Function address conversion.
 */

#ifndef _LIBC_arm32_FADDR_H_
#define _LIBC_arm32_FADDR_H_

#include <types/common.h>

/** Calculate absolute address of function referenced by fptr pointer.
 *
 * @param f Function pointer.
 */
#define FADDR(f)	 (f)

#endif

/** @}
 */
