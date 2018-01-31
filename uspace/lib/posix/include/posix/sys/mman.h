/*
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

/** @addtogroup libposix
 * @{
 */
/** @file Memory management declarations.
 */

#ifndef POSIX_SYS_MMAN_H_
#define POSIX_SYS_MMAN_H_

#include "types.h"
#include <abi/mm/as.h>

#define MAP_FAILED ((void *) -1)

#define MAP_SHARED     (1 << 0)
#define MAP_PRIVATE    (1 << 1)
#define MAP_FIXED      (1 << 2)
#define MAP_ANONYMOUS  (1 << 3)
#define MAP_ANON MAP_ANONYMOUS

#undef PROT_NONE
#undef PROT_READ
#undef PROT_WRITE
#undef PROT_EXEC
#define PROT_NONE  0
#define PROT_READ  AS_AREA_READ
#define PROT_WRITE AS_AREA_WRITE
#define PROT_EXEC  AS_AREA_EXEC

extern void *mmap(void *start, size_t length, int prot, int flags, int fd,
    off_t offset);
extern int munmap(void *start, size_t length);


#endif /* POSIX_SYS_MMAN_H_ */

/** @}
 */
