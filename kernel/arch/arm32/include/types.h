/*
 * Copyright (c) 2007 Pavel Jancik, Michal Kebrt
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

/** @addtogroup arm32
 * @{
 */
/** @file
 *  @brief Type definitions.
 */

#ifndef KERN_arm32_TYPES_H_
#define KERN_arm32_TYPES_H_

#ifndef DOXYGEN
	#define ATTRIBUTE_PACKED __attribute__((packed))
#else
	#define ATTRIBUTE_PACKED
#endif

typedef uint32_t size_t;
typedef int32_t ssize_t;

typedef uint32_t uintptr_t;
typedef uint32_t pfn_t;

typedef uint32_t ipl_t;

typedef uint32_t sysarg_t;
typedef int32_t native_t;
typedef uint32_t atomic_count_t;

typedef struct {
} fncptr_t;

#define INTN_C(c)   INT32_C(c)
#define UINTN_C(c)  UINT32_C(c)

#define PRIdn  PRId32  /**< Format for native_t. */
#define PRIun  PRIu32  /**< Format for sysarg_t. */
#define PRIxn  PRIx32  /**< Format for hexadecimal sysarg_t. */
#define PRIua  PRIu32  /**< Format for atomic_count_t. */

#endif

/** @}
 */
