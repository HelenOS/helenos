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

#ifndef _LIBC_PRIVATE_SSTREAM_H_
#define _LIBC_PRIVATE_SSTREAM_H_

#include <stdio.h>

extern void __sstream_init(const char *, FILE *);
extern const char *__sstream_getpos(FILE *);

#endif

/** @}
 */
