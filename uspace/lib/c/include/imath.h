/*
 * SPDX-FileCopyrightText: 2015 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libc
 * @{
 */
/** @file
 * @brief Integer math functions
 */

#ifndef _LIBC_IMATH_H_
#define _LIBC_IMATH_H_

#include <stdint.h>

extern errno_t ipow10_u64(unsigned, uint64_t *);
extern unsigned ilog10_u64(uint64_t);

#endif

/**
 * @}
 */
