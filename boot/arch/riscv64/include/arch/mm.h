/*
 * Copyright (c) 2017 Martin Decky
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

/** @file
 */

#ifndef BOOT_riscv64_MM_H_
#define BOOT_riscv64_MM_H_

#ifndef __ASSEMBLER__
#define KA2PA(x)  (((uintptr_t) (x)) - UINT64_C(0xffff800000000000))
#define PA2KA(x)  (((uintptr_t) (x)) + UINT64_C(0xffff800000000000))
#else
#define KA2PA(x)  ((x) - 0xffff800000000000)
#define PA2KA(x)  ((x) + 0xffff800000000000)
#endif

#define PTL_DIRTY       (1 << 7)
#define PTL_ACCESSED    (1 << 6)
#define PTL_GLOBAL      (1 << 5)
#define PTL_USER        (1 << 4)
#define PTL_EXECUTABLE  (1 << 3)
#define PTL_WRITABLE    (1 << 2)
#define PTL_READABLE    (1 << 1)
#define PTL_VALID       1

#endif

/** @}
 */
