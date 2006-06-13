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

 /** @addtogroup mips32	
 * @{
 */
/** @file
 */

#ifndef __mips32_ARG_H__
#define __mips32_ARG_H__

#include <arch/types.h>

/**
 * va_arg macro for MIPS32 - problem is that 64 bit values must be aligned on an 8-byte boundary (32bit values not)
 * To satisfy this, paddings must be sometimes inserted. 
 */

typedef __address va_list;

#define va_start(ap, lst) \
	((ap) = (va_list)&(lst) + sizeof(lst))

#define va_arg(ap, type)	\
	(((type *)((ap) = (va_list)( (sizeof(type) <= 4) ? ((__address)((ap) + 2*4 - 1) & (~3)) : ((__address)((ap) + 2*8 -1) & (~7)) )))[-1])

#define va_copy(dst,src) ((dst)=(src))

#define va_end(ap)

#endif

 /** @}
 */

