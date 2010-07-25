/*
 * Copyright (c) 2006 Josef Cejka
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

/** @addtogroup libc
 * @{
 */
/** @file
 */

#ifndef LIBC_STDINT_H_
#define LIBC_STDINT_H_

#define INT8_MIN  (0x80)
#define INT8_MAX  (0x7F)

#define UINT8_MIN  (0u)
#define UINT8_MAX  (0xFFu)

#define INT16_MIN  (0x8000)
#define INT16_MAX  (0x7FFF)

#define UINT16_MIN  (0u)
#define UINT16_MAX  (0xFFFFu)

#define INT32_MIN  (0x80000000l)
#define INT32_MAX  (0x7FFFFFFFl)

#define UINT32_MIN  (0ul)
#define UINT32_MAX  (0xFFFFFFFFul)

#define INT64_MIN  (0x8000000000000000ll)
#define INT64_MAX  (0x7FFFFFFFFFFFFFFFll)

#define UINT64_MIN  (0ull)
#define UINT64_MAX  (0xFFFFFFFFFFFFFFFFull)

#include <libarch/types.h>

/* off64_t */
#define OFF64_MIN  INT64_MIN
#define OFF64_MAX  INT64_MAX

/* aoff64_t */
#define AOFF64_MIN  UINT64_MIN
#define AOFF64_MAX  UINT64_MAX

#endif

/** @}
 */
