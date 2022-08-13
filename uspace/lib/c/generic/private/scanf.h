/*
 * SPDX-FileCopyrightText: 2018 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libc
 * @{
 */
/** @file
 */

#ifndef _LIBC_PRIVATE_SCANF_H_
#define _LIBC_PRIVATE_SCANF_H_

#include <stddef.h>
#include <stdio.h>

extern errno_t __fstrtold(FILE *, int *, size_t, long double *);

#endif

/** @}
 */
