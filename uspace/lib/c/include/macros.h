/*
 * Copyright (c) 2009 Martin Decky
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

#ifndef LIBC_MACROS_H_
#define LIBC_MACROS_H_

#define min(a, b)  ((a) < (b) ? (a) : (b))
#define max(a, b)  ((a) > (b) ? (a) : (b))
#define abs(a)     ((a) >= 0 ? (a) : -(a))

#define ARRAY_SIZE(array)   (sizeof(array) / sizeof(array[0]))

#define KiB2SIZE(kb)  ((kb) << 10)
#define MiB2SIZE(mb)  ((mb) << 20)

#define STRING(arg)      STRING_ARG(arg)
#define STRING_ARG(arg)  #arg

#define LOWER32(arg)  (((uint64_t) (arg)) & 0xffffffff)
#define UPPER32(arg)  (((((uint64_t) arg)) >> 32) & 0xffffffff)

#define MERGE_LOUP32(lo, up) \
	((((uint64_t) (lo)) & 0xffffffff) \
	    | ((((uint64_t) (up)) & 0xffffffff) << 32))

#define member_to_inst(ptr_member, type, member_identif) \
	((type *) (((void *) (ptr_member)) - \
	    ((void *) &(((type *) 0)->member_identif))))

#define _paddname(line)     PADD_ ## line ## __
#define _padd(width, line)  uint ## width ## _t _paddname(line)

#define PADD32  _padd(32, __LINE__)
#define PADD16  _padd(16, __LINE__)
#define PADD8   _padd(8, __LINE__)

#endif

/** @}
 */
