/*
 * SPDX-FileCopyrightText: 2011 Petr Koupy
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libposix
 * @{
 */
/** @file Implementation-defined limit constants.
 */

#ifndef POSIX_LIMITS_H_
#define POSIX_LIMITS_H_

#include_next <limits.h>

#ifndef PATH_MAX
#define PATH_MAX 256
#endif

#endif /* POSIX_LIMITS_H_ */

/** @}
 */
