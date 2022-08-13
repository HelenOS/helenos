/*
 * SPDX-FileCopyrightText: 2017 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libc
 * @{
 */
/** @file
 */

#ifndef _LIBC_UNALIGNED_H_
#define _LIBC_UNALIGNED_H_

#include <stdint.h>

typedef int16_t unaligned_int16_t __attribute__((aligned(1)));
typedef int32_t unaligned_int32_t __attribute__((aligned(1)));
typedef int64_t unaligned_int64_t __attribute__((aligned(1)));

typedef uint16_t unaligned_uint16_t __attribute__((aligned(1)));
typedef uint32_t unaligned_uint32_t __attribute__((aligned(1)));
typedef uint64_t unaligned_uint64_t __attribute__((aligned(1)));

#endif

/** @}
 */
