/*
 * SPDX-FileCopyrightText: 2016 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
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
