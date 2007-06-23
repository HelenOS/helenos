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

/* Definitions of types with fixed size*/
#include <types.h>

#define MAX_INT8 (0x7F)
#define MIN_INT8 (0x80)
#define MAX_UINT8 (0xFFu)
#define MIN_UINT8 (0u)

#define MAX_INT16 (0x7FFF)
#define MIN_INT16 (0x8000)
#define MAX_UINT16 (0xFFFFu)
#define MIN_UINT16 (0u)

#define MAX_INT32 (0x7FFFFFFF)
#define MIN_INT32 (0x80000000)
#define MAX_UINT32 (0xFFFFFFFFu)
#define MIN_UINT32 (0u)

#define MAX_INT64 (0x7FFFFFFFFFFFFFFFll)
#define MIN_INT64 (0x8000000000000000ll)
#define MAX_UINT64 (0xFFFFFFFFFFFFFFFFull)
#define MIN_UINT64 (0ull)

#endif

/** @}
 */
