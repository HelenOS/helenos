/*
 * SPDX-FileCopyrightText: 2012 Adam Hraska
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef DOUBLE_TO_STR_H_
#define DOUBLE_TO_STR_H_

#include <stddef.h>

/** Maximum number of digits double_to_*_str conversion functions produce.
 *
 * Both double_to_short_str and double_to_fixed_str generate digits out
 * of a 64bit unsigned int number representation. The max number of
 * of digits is therefore 20. Add 1 to help users who forget to reserve
 * space for a null terminator.
 */
#define MAX_DOUBLE_STR_LEN (20 + 1)

/** Maximum buffer size needed to store the output of double_to_*_str
 *  functions.
 */
#define MAX_DOUBLE_STR_BUF_SIZE  21

/* Fwd decl. */
struct ieee_double_t_tag;

extern int double_to_short_str(struct ieee_double_t_tag, char *, size_t, int *);
extern int double_to_fixed_str(struct ieee_double_t_tag, int, int, char *,
    size_t, int *);

#endif
