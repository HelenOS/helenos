/*
 * Copyright (C) 2005 Jakub Jermar
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

#ifndef __ppc_BYTEORDER_H__
#define __ppc_BYTEORDER_H__

#include <arch/types.h>

/** Convert little-endian __native to host __native
 *
 * Convert little-endian __native parameter to host endianess.
 *
 * @param n Little-endian __native argument.
 *
 * @return Result in host endianess.
 *
 */
static inline __u64 u64_le2host(__u64 n)
{
	return ((n & 0xff) << 56) |
		((n & 0xff00) << 40) |
		((n & 0xff0000) << 24) |
		((n & 0xff000000LL) << 8) |
		((n & 0xff00000000LL) >>8) |
		((n & 0xff0000000000LL) >> 24) |
		((n & 0xff000000000000LL) >> 40) |
		((n & 0xff00000000000000LL) >> 56);
}
static inline __native native_le2host(__native n)
{
	__address v;
	
	__asm__ volatile ("lwbrx %0, %1, %2\n" : "=r" (v) : "i" (0) , "r" (&n));
	
	return v;
}
#endif
