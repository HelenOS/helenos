/*
 * Copyright (c) 2010 Jiri Svoboda
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * - Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 * - Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the distribution.
 * - The name of the author may not be used to endorse or promote products
 *   derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
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
