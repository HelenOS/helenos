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

 /** @addtogroup ppc32	
 * @{
 */
/** @file
 */

#ifndef __ppc32_BYTEORDER_H__
#define __ppc32_BYTEORDER_H__

#include <arch/types.h>
#include <byteorder.h>

#define BIG_ENDIAN

static inline uint64_t uint64_t_le2host(uint64_t n)
{
	return uint64_t_byteorder_swap(n);
}


/** Convert little-endian unative_t to host unative_t
 *
 * Convert little-endian unative_t parameter to host endianess.
 *
 * @param n Little-endian unative_t argument.
 *
 * @return Result in host endianess.
 *
 */
static inline unative_t unative_t_le2host(unative_t n)
{
	uintptr_t v;
	
	asm volatile (
		"lwbrx %0, %1, %2\n"
		: "=r" (v)
		: "i" (0), "r" (&n)
	);
	return v;
}

#endif

 /** @}
 */

