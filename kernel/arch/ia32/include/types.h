/*
 * Copyright (c) 2001-2004 Jakub Jermar
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

/** @addtogroup ia32	
 * @{
 */
/** @file
 */

#ifndef KERN_ia32_TYPES_H_
#define KERN_ia32_TYPES_H_

#define NULL 0
#define false 0
#define true 1

typedef signed char int8_t;
typedef signed short int16_t;
typedef signed long int32_t;
typedef signed long long int64_t;

typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned long uint32_t;
typedef unsigned long long uint64_t;

typedef uint32_t size_t;
typedef uint32_t count_t;
typedef uint32_t index_t;

typedef uint32_t uintptr_t;
typedef uint32_t pfn_t;

typedef uint32_t ipl_t;

typedef uint32_t unative_t;
typedef int32_t native_t;

typedef uint8_t bool;
typedef uint64_t task_id_t;
typedef uint32_t context_id_t;

typedef int32_t inr_t;
typedef int32_t devno_t;

/** Page Table Entry. */
typedef struct {
	unsigned present : 1;
	unsigned writeable : 1;
	unsigned uaccessible : 1;
	unsigned page_write_through : 1;
	unsigned page_cache_disable : 1;
	unsigned accessed : 1;
	unsigned dirty : 1;
	unsigned pat : 1;
	unsigned global : 1;
	unsigned soft_valid : 1;	/**< Valid content even if the present bit is not set. */
	unsigned avl : 2;
	unsigned frame_address : 20;
} __attribute__ ((packed)) pte_t;

#endif

/** @}
 */
