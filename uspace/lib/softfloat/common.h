/*
 * Copyright (c) 2005 Josef Cejka
 * Copyright (c) 2011 Petr Koupy
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

/** @addtogroup softfloat
 * @{
 */
/** @file Common helper operations.
 */

#ifndef __COMMON_H__
#define __COMMON_H__

#include "sftypes.h"

extern float64 finish_float64(int32_t, uint64_t, char);
extern float128 finish_float128(int32_t, uint64_t, uint64_t, char, uint64_t);

extern int count_zeroes8(uint8_t);
extern int count_zeroes32(uint32_t);
extern int count_zeroes64(uint64_t);

extern void round_float32(int32_t *, uint32_t *);
extern void round_float64(int32_t *, uint64_t *);
extern void round_float128(int32_t *, uint64_t *, uint64_t *);

extern void lshift128(uint64_t, uint64_t, int, uint64_t *, uint64_t *);
extern void rshift128(uint64_t, uint64_t, int, uint64_t *, uint64_t *);

extern void and128(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t *, uint64_t *);
extern void or128(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t *, uint64_t *);
extern void xor128(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t *, uint64_t *);
extern void not128(uint64_t, uint64_t, uint64_t *, uint64_t *);

extern int eq128(uint64_t, uint64_t, uint64_t, uint64_t);
extern int le128(uint64_t, uint64_t, uint64_t, uint64_t);
extern int lt128(uint64_t, uint64_t, uint64_t, uint64_t);

extern void add128(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t *, uint64_t *);
extern void sub128(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t *, uint64_t *);

extern void mul64(uint64_t, uint64_t, uint64_t *, uint64_t *);
extern void mul128(uint64_t, uint64_t, uint64_t, uint64_t,
    uint64_t *, uint64_t *, uint64_t *, uint64_t *);

extern uint64_t div128est(uint64_t, uint64_t, uint64_t);

#endif

/** @}
 */
