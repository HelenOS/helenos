/*
 * Copyright (c) 2010 Jakub Jermar
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

/** @addtogroup genericmm
 * @{
 */
/** @file
 */

#ifndef ABI_AS_H_
#define ABI_AS_H_

/** Address space area flags. */
#define AS_AREA_READ         0x01
#define AS_AREA_WRITE        0x02
#define AS_AREA_EXEC         0x04
#define AS_AREA_CACHEABLE    0x08
#define AS_AREA_GUARD        0x10
#define AS_AREA_LATE_RESERVE 0x20

#define AS_AREA_ANY    ((void *) -1)
#define AS_MAP_FAILED  ((void *) -1)

#define AS_AREA_UNPAGED NULL

/** Address space area info exported to uspace. */
typedef struct {
	/** Starting address */
	uintptr_t start_addr;

	/** Area size */
	size_t size;

	/** Area flags */
	unsigned int flags;
} as_area_info_t;

typedef struct {
	int pager;
	sysarg_t id1;
	sysarg_t id2;
	sysarg_t id3;
} as_area_pager_info_t;

#endif

/** @}
 */
