/*
 * SPDX-FileCopyrightText: 2019 Vojtech Horky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libc
 * @{
 */
/** @file
 */

#ifndef _LIBC_TYPES_CASTING_H_
#define _LIBC_TYPES_CASTING_H_

#include <stdbool.h>
#include <stddef.h>

/**
 * Checks that the size_t value can be casted to int without loss of data.
 *
 * @param val Value to test.
 * @return Whether it is safe to typecast value to int.
 */
static inline bool can_cast_size_t_to_int(size_t val)
{
	unsigned int as_uint = (unsigned int) val;
	return !((as_uint != val) || (((int) as_uint) < 0));
}

#endif

/** @}
 */
