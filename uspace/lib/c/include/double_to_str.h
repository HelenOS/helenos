/*
 * Copyright (c) 2012 Adam Hraska
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

/* Fwd decl.*/
struct ieee_double_t_tag;

extern int double_to_short_str(struct ieee_double_t_tag, char *, size_t, int *);
extern int double_to_fixed_str(struct ieee_double_t_tag, int, int, char *,
    size_t, int *);

#endif
