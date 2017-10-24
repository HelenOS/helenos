/*
 * Copyright (c) 2017 CZ.NIC, z.s.p.o.
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

/* Authors:
 *	Jiří Zárevúcky (jzr) <zarevucky.jiri@gmail.com>
 */

/** @addtogroup bits
 * @{
 */
/** @file
 * Basic macro definitions and redefinitions that support basic C types.
 *
 * Both GCC and clang define macros for fixed-size integer names
 * (e.g. __INT32_TYPE__ for int32_t). However, while clang also provides
 * definitions for formatting macros (PRId32 et al.), GCC does not.
 * This poses a subtle problem for diagnostics -- when 'int' and 'long int'
 * have the same width, it's not possible to detect which one is used to
 * define 'int32_t', but using a formatting specifier for 'int' on 'long int'
 * value, or vice versa, causes a compiler warning. Analogous problem occurs
 * with 'long int' and 'long long int', if they are the same size.
 *
 * In the interest of compatibility, we ignore the compiler's opinion on
 * fixed-size types, and redefine the macros consistently.
 *
 * However, we can't do the same for 'size_t', 'ptrdiff_t', 'wchar_t', 'wint_t',
 * 'intmax_t' and 'uintmax_t', since their format specifiers and other
 * properties are built into the language and their identity is therefore
 * determined by the compiler.
 *
 * We use some macros if they are present, but provide our own fallbacks when
 * they are not. There are several macros that have to be predefined either by
 * the compiler itself, or supplied by the build system if necessary
 * (e.g. using the '-imacros' compiler argument). This is basically a subset
 * of the common denominator of GCC and clang predefined macros at this moment.
 *
 * These required macros include:
 *
 *	__CHAR_BIT__
 *	__SIZEOF_SHORT__
 *	__SIZEOF_INT__
 *	__SIZEOF_LONG__
 *	__SIZEOF_LONG_LONG__
 *	__SIZEOF_POINTER__
 *	__SIZEOF_FLOAT__
 *	__SIZEOF_DOUBLE__
 *	__SIZEOF_LONG_DOUBLE__
 *
 *	__SIZE_TYPE__
 *	__SIZE_MAX__
 *
 *	__PTRDIFF_TYPE__
 *	__PTRDIFF_MAX__
 *
 *	__WCHAR_TYPE__
 *	__WCHAR_MAX__
 *
 *	__WINT_TYPE__
 *	__WINT_MAX__ or
 *	__WINT_WIDTH__ and __WINT_UNSIGNED__ when wint_t is unsigned
 *
 *	__INTMAX_TYPE__
 *	__INTMAX_MAX__
 *	either __INTMAX_C_SUFFIX__ or __INTMAX_C
 *
 *	__CHAR_UNSIGNED__ when char is unsigned
 *
 * After this header is processed, the following additional macros are
 * guaranteed to be defined, or the build will fail:
 *
 * 	for {type} being one of CHAR, SHRT, INT, LONG, LLONG
 * 	__{type}_MIN__
 * 	__{type}_MAX__
 * 	__U{type}_MAX__
 *
 * 	__SCHAR_MIN__
 * 	__SCHAR_MAX__
 *
 * 	for {size} being one of 8, 16, 32, 64, PTR, MAX
 * 	__INT{size}_TYPE__
 * 	__INT{size}_MIN__
 * 	__INT{size}_MAX__
 * 	__UINT{size}_TYPE__
 * 	__UINT{size}_MAX__
 * 	for {tag} being one of d, i, u, o, x, X
 * 	__PRI{tag}{size}__
 * 	for {tag} being one of d, i, u, o, x
 * 	__SCN{tag}{size}__
 * 	__INT{size}_C
 * 	__UINT{size}_C
 * 	(except for __INTPTR_C/__UINTPTR_C, which aren't provided)
 *
 * 	__PTRDIFF_MIN__
 * 	__WCHAR_MIN__
 * 	__WINT_MIN__
 * 	__WINT_MAX__
 * 	__WINT_EOF__
 * 	__WCHAR_SIGNED__ or __WCHAR_UNSIGNED__
 * 	__WINT_SIGNED__ or __WINT_UNSIGNED__
 *
 */

// TODO: add sig_atomic_t for compatibility with C language standard

#ifndef _BITS_MACROS_H_
#define _BITS_MACROS_H_

#if __CHAR_BIT__ != 8
#error HelenOS expects char that is 8 bits wide.
#endif

#ifndef __SCHAR_MAX__
#define __SCHAR_MAX__  0x7f
#endif

#ifndef __SCHAR_MIN__
#define __SCHAR_MIN__  (-__SCHAR_MAX__ - 1)
#endif

#ifndef __UCHAR_MAX__
#define __UCHAR_MAX__  0xff
#endif

#ifndef __CHAR_MIN__
#ifdef __CHAR_UNSIGNED__
#define __CHAR_MIN__  0
#else
#define __CHAR_MIN__  __SCHAR_MIN__
#endif
#endif

#ifndef __CHAR_MAX__
#ifdef __CHAR_UNSIGNED__
#define __CHAR_MAX__  __UCHAR_MAX__
#else
#define __CHAR_MAX__  __SCHAR_MAX__
#endif
#endif

#undef __INT8_TYPE__
#undef __INT8_C
#undef __INT8_C_SUFFIX__
#undef __INT8_MIN__
#undef __INT8_MAX__

#define __INT8_TYPE__  signed char
#define __INT8_C(x)    x
#define __INT8_MIN__   __SCHAR_MIN__
#define __INT8_MAX__   __SCHAR_MAX__

#undef __UINT8_TYPE__
#undef __UINT8_C
#undef __UINT8_C_SUFFIX__
#undef __UINT8_MIN__
#undef __UINT8_MAX__

#define __UINT8_TYPE__  unsigned char
#define __UINT8_C(x)    x
#define __UINT8_MAX__   __UCHAR_MAX__

#undef __PRId8__
#undef __PRIi8__
#undef __PRIu8__
#undef __PRIo8__
#undef __PRIx8__
#undef __PRIX8__

#define __PRId8__  "hhd"
#define __PRIi8__  "hhi"
#define __PRIu8__  "hhu"
#define __PRIo8__  "hho"
#define __PRIx8__  "hhx"
#define __PRIX8__  "hhX"

#undef __SCNd8__
#undef __SCNi8__
#undef __SCNu8__
#undef __SCNo8__
#undef __SCNx8__

#define __SCNd8__  "hhd"
#define __SCNi8__  "hhi"
#define __SCNu8__  "hhu"
#define __SCNo8__  "hho"
#define __SCNx8__  "hhx"

#if __SIZEOF_SHORT__ != 2
#error HelenOS expects short that is 16 bits wide.
#endif

#ifndef __SHRT_MAX__
#define __SHRT_MAX__  0x7fff
#endif

#ifndef __SHRT_MIN__
#define __SHRT_MIN__  (-__SHRT_MAX__ - 1)
#endif

#ifndef __USHRT_MAX__
#define __USHRT_MAX__  0xffff
#endif

#undef __INT16_TYPE__
#undef __INT16_C
#undef __INT16_C_SUFFIX__
#undef __INT16_MIN__
#undef __INT16_MAX__

#define __INT16_TYPE__  short
#define __INT16_C(x)    x
#define __INT16_MIN__   __SHRT_MIN__
#define __INT16_MAX__   __SHRT_MAX__

#undef __UINT16_TYPE__
#undef __UINT16_C
#undef __UINT16_C_SUFFIX__
#undef __UINT16_MIN__
#undef __UINT16_MAX__

#define __UINT16_TYPE__  unsigned short
#define __UINT16_C(x)    x
#define __UINT16_MAX__   __USHRT_MAX__

#undef __PRId16__
#undef __PRIi16__
#undef __PRIu16__
#undef __PRIo16__
#undef __PRIx16__
#undef __PRIX16__

#define __PRId16__  "hd"
#define __PRIi16__  "hi"
#define __PRIu16__  "hu"
#define __PRIo16__  "ho"
#define __PRIx16__  "hx"
#define __PRIX16__  "hX"

#undef __SCNd16__
#undef __SCNi16__
#undef __SCNu16__
#undef __SCNo16__
#undef __SCNx16__

#define __SCNd16__  "hd"
#define __SCNi16__  "hi"
#define __SCNu16__  "hu"
#define __SCNo16__  "ho"
#define __SCNx16__  "hx"

#if __SIZEOF_INT__ != 4
#error HelenOS expects int that is 32 bits wide.
#endif

#ifndef __INT_MAX__
#define __INT_MAX__  0x7fffffff
#endif

#ifndef __INT_MIN__
#define __INT_MIN__  (-__INT_MAX__ - 1)
#endif

#ifndef __UINT_MAX__
#define __UINT_MAX__  0xffffffffU
#endif

#undef __INT32_TYPE__
#undef __INT32_C
#undef __INT32_C_SUFFIX__
#undef __INT32_MIN__
#undef __INT32_MAX__

#define __INT32_TYPE__  int
#define __INT32_C(x)    x
#define __INT32_MIN__   __INT_MIN__
#define __INT32_MAX__   __INT_MAX__

#undef __UINT32_TYPE__
#undef __UINT32_C
#undef __UINT32_C_SUFFIX__
#undef __UINT32_MIN__
#undef __UINT32_MAX__

#define __UINT32_TYPE__  unsigned int
#define __UINT32_C(x)    x##U
#define __UINT32_MAX__   __UINT_MAX__

#undef __PRId32__
#undef __PRIi32__
#undef __PRIu32__
#undef __PRIo32__
#undef __PRIx32__
#undef __PRIX32__

#define __PRId32__  "d"
#define __PRIi32__  "i"
#define __PRIu32__  "u"
#define __PRIo32__  "o"
#define __PRIx32__  "x"
#define __PRIX32__  "X"

#undef __SCNd32__
#undef __SCNi32__
#undef __SCNu32__
#undef __SCNo32__
#undef __SCNx32__

#define __SCNd32__  "d"
#define __SCNi32__  "i"
#define __SCNu32__  "u"
#define __SCNo32__  "o"
#define __SCNx32__  "x"

#if __SIZEOF_LONG__ != __SIZEOF_POINTER__
#error HelenOS expects long that has native width for the architecture.
#endif

#if __SIZEOF_POINTER__ < 4 || __SIZEOF_POINTER__ > 8
#error HelenOS expects pointer width to be either 32 bits or 64 bits.
#endif

#ifndef __LONG_MAX__
#if __SIZEOF_LONG__ == 4
#define __LONG_MAX__  0x7fffffffL
#elif __SIZEOF_LONG__ == 8
#define __LONG_MAX__  0x7fffffffffffffffL
#endif
#endif

#ifndef __LONG_MIN__
#define __LONG_MIN__  (-__LONG_MAX__ - 1)
#endif

#ifndef __ULONG_MAX__
#if __SIZEOF_LONG__ == 4
#define __ULONG_MAX__  0xffffffffUL
#elif __SIZEOF_LONG__ == 8
#define __ULONG_MAX__  0xffffffffffffffffUL
#endif
#endif

#undef __INTPTR_TYPE__
#undef __INTPTR_C
#undef __INTPTR_C_SUFFIX__
#undef __INTPTR_MIN__
#undef __INTPTR_MAX__
#undef __UINTPTR_TYPE__
#undef __UINTPTR_C
#undef __UINTPTR_C_SUFFIX__
#undef __UINTPTR_MIN__
#undef __UINTPTR_MAX__
#undef __PRIdPTR__
#undef __PRIiPTR__
#undef __PRIuPTR__
#undef __PRIoPTR__
#undef __PRIxPTR__
#undef __PRIXPTR__
#undef __SCNdPTR__
#undef __SCNiPTR__
#undef __SCNuPTR__
#undef __SCNoPTR__
#undef __SCNxPTR__

#ifdef __intptr_is_long__

#define __INTPTR_TYPE__   long
#define __INTPTR_MIN__    __LONG_MIN__
#define __INTPTR_MAX__    __LONG_MAX__
#define __UINTPTR_TYPE__  unsigned long
#define __UINTPTR_MAX__   __ULONG_MAX__

#define __PRIdPTR__  "ld"
#define __PRIiPTR__  "li"
#define __PRIuPTR__  "lu"
#define __PRIoPTR__  "lo"
#define __PRIxPTR__  "lx"
#define __PRIXPTR__  "lX"

#define __SCNdPTR__  "ld"
#define __SCNiPTR__  "li"
#define __SCNuPTR__  "lu"
#define __SCNoPTR__  "lo"
#define __SCNxPTR__  "lx"

#else

// Original definition results in size_t and uintptr_t always having the same
// type. We keep this definition for the time being, to avoid unnecessary noise.

#define __INTPTR_TYPE__   __PTRDIFF_TYPE__
#define __INTPTR_MIN__    __PTRDIFF_MIN__
#define __INTPTR_MAX__    __PTRDIFF_MAX__
#define __UINTPTR_TYPE__  __SIZE_TYPE__
#define __UINTPTR_MAX__   __SIZE_MAX__

#define __PRIdPTR__  "zd"
#define __PRIiPTR__  "zi"
#define __PRIuPTR__  "zu"
#define __PRIoPTR__  "zo"
#define __PRIxPTR__  "zx"
#define __PRIXPTR__  "zX"

#define __SCNdPTR__  "zd"
#define __SCNiPTR__  "zi"
#define __SCNuPTR__  "zu"
#define __SCNoPTR__  "zo"
#define __SCNxPTR__  "zx"

#endif

#if __SIZEOF_LONG_LONG__ != 8
#error HelenOS expects long long that is 64 bits wide.
#endif

#ifndef __LLONG_MAX__
#define __LLONG_MAX__  0x7fffffffffffffffLL
#endif

#ifndef __LLONG_MIN__
#define __LLONG_MIN__  (-__LLONG_MAX__ - 1)
#endif

#ifndef __ULLONG_MAX__
#define __ULLONG_MAX__  0xffffffffffffffffULL
#endif

#undef __INT64_TYPE__
#undef __INT64_C
#undef __INT64_C_SUFFIX__
#undef __INT64_MIN__
#undef __INT64_MAX__

#define __INT64_TYPE__  long long
#define __INT64_C(x)    x##LL
#define __INT64_MIN__   __LLONG_MIN__
#define __INT64_MAX__   __LLONG_MAX__

#undef __UINT64_TYPE__
#undef __UINT64_C
#undef __UINT64_C_SUFFIX__
#undef __UINT64_MIN__
#undef __UINT64_MAX__

#define __UINT64_TYPE__  unsigned long long
#define __UINT64_C(x)    x##ULL
#define __UINT64_MAX__   __ULLONG_MAX__

#undef __PRId64__
#undef __PRIi64__
#undef __PRIu64__
#undef __PRIo64__
#undef __PRIx64__
#undef __PRIX64__

#define __PRId64__  "lld"
#define __PRIi64__  "lli"
#define __PRIu64__  "llu"
#define __PRIo64__  "llo"
#define __PRIx64__  "llx"
#define __PRIX64__  "llX"

#undef __SCNd64__
#undef __SCNi64__
#undef __SCNu64__
#undef __SCNo64__
#undef __SCNx64__

#define __SCNd64__  "lld"
#define __SCNi64__  "lli"
#define __SCNu64__  "llu"
#define __SCNo64__  "llo"
#define __SCNx64__  "llx"

#if !defined(__SIZE_TYPE__) || !defined(__SIZE_MAX__)
#error HelenOS expects __SIZE_TYPE__ and __SIZE_MAX__ \
    to be defined by the toolchain.
#endif

#if !defined(__PTRDIFF_TYPE__) || !defined(__PTRDIFF_MAX__)
#error HelenOS expects __PTRDIFF_TYPE__ and __PTRDIFF_MAX__ \
    to be defined by the toolchain.
#endif

#ifndef __PTRDIFF_MIN__
#define __PTRDIFF_MIN__  (-__PTRDIFF_MAX__ - 1)
#endif

#if !defined(__WCHAR_TYPE__) || !defined(__WCHAR_MAX__)
#error HelenOS expects __WCHAR_TYPE__ and __WCHAR_MAX__ \
    to be defined by the toolchain.
#endif

#undef __WCHAR_SIGNED__
#undef __WCHAR_UNSIGNED__

#if __WCHAR_MAX__ == __INT32_MAX__
#define __WCHAR_SIGNED__  1
#elif __WCHAR_MAX__ == __UINT32_MAX__
#define __WCHAR_UNSIGNED__  1
#else
#error HelenOS expects __WCHAR_TYPE__ to be 32 bits wide.
#endif

#ifndef __WCHAR_MIN__
#ifdef __WCHAR_UNSIGNED__
#define __WCHAR_MIN__  0
#else
#define __WCHAR_MIN__  (-__WCHAR_MAX__ - 1)
#endif
#endif

#if !defined(__WINT_TYPE__) || !defined(__WINT_WIDTH__)
#error HelenOS expects __WINT_TYPE__ and __WINT_WIDTH__ \
    to be defined by the toolchain.
#endif

#if __WINT_WIDTH__ != 32
#error HelenOS expects __WINT_TYPE__ to be 32 bits wide.
#endif

#undef __WINT_SIGNED__
#undef __WINT_UNSIGNED__

#if __WINT_MAX__ == __INT32_MAX__
#define __WINT_SIGNED__  1
#elif __WINT_MAX__ == __UINT32_MAX__
#define __WINT_UNSIGNED__  1
#elif defined(__WINT_MAX__)
#error Unrecognized value of __WINT_MAX__
#endif

#ifndef __WINT_MAX__
#ifdef __WINT_UNSIGNED__
#define __WINT_MAX__  0xffffffffU
#else
#define __WINT_MAX__  0x7fffffff
#endif
#endif

#ifndef __WINT_MIN__
#ifdef __WINT_UNSIGNED__
#define __WINT_MIN__  0
#else
#define __WINT_MIN__  (-__WINT_MAX__ - 1)
#endif
#endif

#ifndef __WINT_EOF__
#ifdef __WINT_UNSIGNED__
#define __WINT_EOF__  __WINT_MAX__
#else
#define __WINT_EOF__  -1
#endif
#endif

#if !defined(__INTMAX_TYPE__) || !defined(__INTMAX_MAX__)
#error HelenOS expects __INTMAX_TYPE__ and __INTMAX_MAX__ \
    to be defined by the toolchain.
#endif

#ifndef __INTMAX_MIN__
#define __INTMAX_MIN__  (-__INTMAX_MAX__ - 1)
#endif

#ifndef __INTMAX_C
#ifndef __INTMAX_C_SUFFIX__
#error HelenOS expects __INTMAX_C or __INTMAX_C_SUFFIX__ \
    to be defined by the toolchain.
#endif
#define __INTMAX_C__(x, y)  x##y
#define __INTMAX_C(x)       __INTMAX_C__(x, __INTMAX_C_SUFFIX__)
#endif

#ifndef __UINTMAX_TYPE__
#define __UINTMAX_TYPE__  unsigned __INTMAX_TYPE__
#endif

#ifndef __UINTMAX_C
#ifdef __UINTMAX_C_SUFFIX__
#define __UINTMAX_C__(x, y)  x##y
#define __UINTMAX_C(x)       __UINTMAX_C__(x, __UINTMAX_C_SUFFIX__)
#else
#define __UINTMAX_C(x)  __INTMAX_C(x##U)
#endif
#endif

#ifndef __UINTMAX_MAX__
#define ___UNSIGNED_INTMAX_MAX___(x)  x##U
#define __UINTMAX_MAX__  (2 * ___UNSIGNED_INTMAX_MAX___(__INTMAX_MAX__) + 1)
#endif

#undef __PRIdMAX__
#undef __PRIiMAX__
#undef __PRIuMAX__
#undef __PRIoMAX__
#undef __PRIxMAX__
#undef __PRIXMAX__

#define __PRIdMAX__  "jd"
#define __PRIiMAX__  "ji"
#define __PRIuMAX__  "ju"
#define __PRIoMAX__  "jo"
#define __PRIxMAX__  "jx"
#define __PRIXMAX__  "jX"

#undef __SCNdMAX__
#undef __SCNiMAX__
#undef __SCNuMAX__
#undef __SCNoMAX__
#undef __SCNxMAX__

#define __SCNdMAX__  "jd"
#define __SCNiMAX__  "ji"
#define __SCNuMAX__  "ju"
#define __SCNoMAX__  "jo"
#define __SCNxMAX__  "jx"

#ifndef __SIZEOF_FLOAT__
#error HelenOS expects __SIZEOF_FLOAT__ to be defined.
#endif

#ifndef __SIZEOF_DOUBLE__
#error HelenOS expects __SIZEOF_DOUBLE__ to be defined.
#endif

#ifndef __SIZEOF_LONG_DOUBLE__
#error HelenOS expects __SIZEOF_LONG_DOUBLE__ to be defined.
#endif

#endif

/** @}
 */
