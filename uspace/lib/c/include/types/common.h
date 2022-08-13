/*
 * SPDX-FileCopyrightText: 2017 Jiri Zarevucky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libc
 * @{
 */
/** @file
 */

#ifndef _LIBC_TYPES_COMMON_H_
#define _LIBC_TYPES_COMMON_H_

#if __SIZEOF_POINTER__ == 4
#define __32_BITS__
#elif __SIZEOF_POINTER__ == 8
#define __64_BITS__
#else
#error __SIZEOF_POINTER__ is not defined.
#endif

#include <_bits/all.h>
#include <_bits/errno.h>

#endif

/** @}
 */
