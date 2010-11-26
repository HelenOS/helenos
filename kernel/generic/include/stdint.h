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

/** @addtogroup generic
 * @{
 */
/** @file
 */

#ifndef KERN_STDINT_H_
#define KERN_STDINT_H_

#define INT8_MIN  INT8_C(0x80)
#define INT8_MAX  INT8_C(0x7F)

#define UINT8_MIN  UINT8_C(0)
#define UINT8_MAX  UINT8_C(0xFF)

#define INT16_MIN  INT16_C(0x8000)
#define INT16_MAX  INT16_C(0x7FFF)

#define UINT16_MIN  UINT16_C(0)
#define UINT16_MAX  UINT16_C(0xFFFF)

#define INT32_MIN  INT32_C(0x80000000)
#define INT32_MAX  INT32_C(0x7FFFFFFF)

#define UINT32_MIN  UINT32_C(0)
#define UINT32_MAX  UINT32_C(0xFFFFFFFF)

#define INT64_MIN  INT64_C(0x8000000000000000)
#define INT64_MAX  INT64_C(0x7FFFFFFFFFFFFFFF)

#define UINT64_MIN  UINT64_C(0)
#define UINT64_MAX  UINT64_C(0xFFFFFFFFFFFFFFFF)

#endif

/** @}
 */
