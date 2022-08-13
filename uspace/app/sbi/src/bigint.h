/*
 * SPDX-FileCopyrightText: 2010 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef BIGINT_H_
#define BIGINT_H_

#include "mytypes.h"

void bigint_init(bigint_t *bigint, int value);
void bigint_shallow_copy(bigint_t *src, bigint_t *dest);
void bigint_clone(bigint_t *src, bigint_t *dest);
void bigint_reverse_sign(bigint_t *src, bigint_t *dest);
void bigint_destroy(bigint_t *bigint);

errno_t bigint_get_value_int(bigint_t *bigint, int *dval);
bool_t bigint_is_zero(bigint_t *bigint);
bool_t bigint_is_negative(bigint_t *bigint);

void bigint_div_digit(bigint_t *a, bigint_word_t b, bigint_t *quot,
    bigint_word_t *rem);

void bigint_add(bigint_t *a, bigint_t *b, bigint_t *dest);
void bigint_sub(bigint_t *a, bigint_t *b, bigint_t *dest);
void bigint_mul(bigint_t *a, bigint_t *b, bigint_t *dest);

void bigint_get_as_string(bigint_t *bigint, char **dptr);
void bigint_print(bigint_t *bigint);

#endif
