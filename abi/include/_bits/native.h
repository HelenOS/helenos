/*
 * Copyright (c) 2017 CZ.NIC, z.s.p.o.
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

/* Authors:
 *	Jiří Zárevúcky (jzr) <zarevucky.jiri@gmail.com>
 */

/** @addtogroup bits
 * @{
 */
/** @file
 * A bunch of type aliases HelenOS code uses.
 *
 * They were originally defined as either u/int32_t or u/int64_t,
 * specifically for each architecture, but in practice they are
 * currently assumed to be identical to u/intptr_t, so we do just that.
 */

#ifndef _BITS_NATIVE_H_
#define _BITS_NATIVE_H_

#include <_bits/macros.h>

#define ATOMIC_COUNT_MIN  __UINTPTR_MIN__
#define ATOMIC_COUNT_MAX  __UINTPTR_MAX__

typedef __UINTPTR_TYPE__ pfn_t;
typedef __UINTPTR_TYPE__ ipl_t;
typedef __UINTPTR_TYPE__ sysarg_t;
typedef __INTPTR_TYPE__  native_t;
typedef __UINTPTR_TYPE__ atomic_count_t;
typedef __INTPTR_TYPE__  atomic_signed_t;

#define PRIdn  __PRIdPTR__  /**< Format for native_t. */
#define PRIun  __PRIuPTR__  /**< Format for sysarg_t. */
#define PRIxn  __PRIxPTR__  /**< Format for hexadecimal sysarg_t. */
#define PRIua  __PRIuPTR__  /**< Format for atomic_count_t. */

#endif

/** @}
 */
