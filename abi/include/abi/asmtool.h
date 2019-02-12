/*
 * Copyright (c) 2016 Jakub Jermar
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

/** @addtogroup abi
 * @{
 */
/** @file
 */

#ifndef _ABI_ASMTOOL_H_
#define _ABI_ASMTOOL_H_

#define SYMBOL(sym) \
	.global sym; \
	sym:
#define SYMBOL_BEGIN(sym) \
	SYMBOL(sym)
#define SYMBOL_END(sym) \
	.size sym, . - sym

#define OBJECT_BEGIN(obj) \
	.type obj STT_OBJECT; \
	SYMBOL_BEGIN(obj)
#define OBJECT_END(obj) \
	SYMBOL_END(obj)

#define FUNCTION_BEGIN(func) \
	.type func STT_FUNC; \
	SYMBOL_BEGIN(func)
#define FUNCTION_END(func) \
	SYMBOL_END(func)

#ifdef __PIC__
#define FUNCTION_REF(func) func@PLT
#else
#define FUNCTION_REF(func) func
#endif

#endif

/** @}
 */
