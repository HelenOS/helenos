/*
 * SPDX-FileCopyrightText: 2010 Martin Decky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libc
 * @{
 */
/** @file
 */

#ifndef _LIBC_STRERROR_H_
#define _LIBC_STRERROR_H_

#include <errno.h>

const char *str_error(errno_t);
const char *str_error_name(errno_t);

#endif

/** @}
 */
