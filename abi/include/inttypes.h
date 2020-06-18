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

/*
 * Authors:
 *	Jiří Zárevúcky (jzr) <zarevucky.jiri@gmail.com>
 */

/** @addtogroup bits
 * @{
 */
/** @file
 */

#ifndef _BITS_INTTYPES_H_
#define _BITS_INTTYPES_H_

#include <stdint.h>
#include <_bits/wchar_t.h>
#include <_bits/uchar.h>
#include <_bits/decls.h>

/*
 * We assume the -helenos target is in use, which means the following assignment:
 *
 *            32b             64b
 * int8_t     unsigned char   unsigned char
 * int16_t    short           short
 * int32_t    int             int
 * int64_t    long long       long
 * intptr_t   int             long
 * size_t     unsigned int    unsigned long
 * ptrdiff_t  int             long
 * intmax_t   long long       long long
 *
 * intleast*_t    same as int*_t
 * intfast*_t     at least int, otherwise same as int*_t
 */

#define PRId8       "hhd"
#define PRIdLEAST8  "hhd"
#define PRIdFAST8   "d"

#define PRIi8       "hhi"
#define PRIiLEAST8  "hhi"
#define PRIiFAST8   "i"

#define PRIo8       "hho"
#define PRIoLEAST8  "hho"
#define PRIoFAST8   "o"

#define PRIu8       "hhu"
#define PRIuLEAST8  "hhu"
#define PRIuFAST8   "u"

#define PRIx8       "hhx"
#define PRIxLEAST8  "hhx"
#define PRIxFAST8   "x"

#define PRIX8       "hhX"
#define PRIXLEAST8  "hhX"
#define PRIXFAST8   "X"

#define SCNd8       "hhd"
#define SCNdLEAST8  "hhd"
#define SCNdFAST8   "d"

#define SCNi8       "hhi"
#define SCNiLEAST8  "hhi"
#define SCNiFAST8   "i"

#define SCNo8       "hho"
#define SCNoLEAST8  "hho"
#define SCNoFAST8   "o"

#define SCNu8       "hhu"
#define SCNuLEAST8  "hhu"
#define SCNuFAST8   "u"

#define SCNx8       "hhx"
#define SCNxLEAST8  "hhx"
#define SCNxFAST8   "x"

#define PRId16       "hd"
#define PRIdLEAST16  "hd"
#define PRIdFAST16   "d"

#define PRIi16       "hi"
#define PRIiLEAST16  "hi"
#define PRIiFAST16   "i"

#define PRIo16       "ho"
#define PRIoLEAST16  "ho"
#define PRIoFAST16   "o"

#define PRIu16       "hu"
#define PRIuLEAST16  "hu"
#define PRIuFAST16   "u"

#define PRIx16       "hx"
#define PRIxLEAST16  "hx"
#define PRIxFAST16   "x"

#define PRIX16       "hX"
#define PRIXLEAST16  "hX"
#define PRIXFAST16   "X"

#define SCNd16       "hd"
#define SCNdLEAST16  "hd"
#define SCNdFAST16   "d"

#define SCNi16       "hi"
#define SCNiLEAST16  "hi"
#define SCNiFAST16   "i"

#define SCNo16       "ho"
#define SCNoLEAST16  "ho"
#define SCNoFAST16   "o"

#define SCNu16       "hu"
#define SCNuLEAST16  "hu"
#define SCNuFAST16   "u"

#define SCNx16       "hx"
#define SCNxLEAST16  "hx"
#define SCNxFAST16   "x"

#define PRId32       "d"
#define PRIdLEAST32  "d"
#define PRIdFAST32   "d"

#define PRIi32       "i"
#define PRIiLEAST32  "i"
#define PRIiFAST32   "i"

#define PRIo32       "o"
#define PRIoLEAST32  "o"
#define PRIoFAST32   "o"

#define PRIu32       "u"
#define PRIuLEAST32  "u"
#define PRIuFAST32   "u"

#define PRIx32       "x"
#define PRIxLEAST32  "x"
#define PRIxFAST32   "x"

#define PRIX32       "X"
#define PRIXLEAST32  "X"
#define PRIXFAST32   "X"

#define SCNd32       "d"
#define SCNdLEAST32  "d"
#define SCNdFAST32   "d"

#define SCNi32       "i"
#define SCNiLEAST32  "i"
#define SCNiFAST32   "i"

#define SCNo32       "o"
#define SCNoLEAST32  "o"
#define SCNoFAST32   "o"

#define SCNu32       "u"
#define SCNuLEAST32  "u"
#define SCNuFAST32   "u"

#define SCNx32       "x"
#define SCNxLEAST32  "x"
#define SCNxFAST32   "x"

#if INTPTR_MAX == 0x7fffffff

#define PRIdPTR  "d"
#define PRIiPTR  "i"
#define PRIoPTR  "o"
#define PRIuPTR  "u"
#define PRIxPTR  "x"
#define PRIXPTR  "X"
#define SCNdPTR  "d"
#define SCNiPTR  "i"
#define SCNoPTR  "o"
#define SCNuPTR  "u"
#define SCNxPTR  "x"

#define PRId64       "lld"
#define PRIdLEAST64  "lld"
#define PRIdFAST64   "lld"

#define PRIi64       "lli"
#define PRIiLEAST64  "lli"
#define PRIiFAST64   "lli"

#define PRIo64       "llo"
#define PRIoLEAST64  "llo"
#define PRIoFAST64   "llo"

#define PRIu64       "llu"
#define PRIuLEAST64  "llu"
#define PRIuFAST64   "llu"

#define PRIx64       "llx"
#define PRIxLEAST64  "llx"
#define PRIxFAST64   "llx"

#define PRIX64       "llX"
#define PRIXLEAST64  "llX"
#define PRIXFAST64   "llX"

#define SCNd64       "lld"
#define SCNdLEAST64  "lld"
#define SCNdFAST64   "lld"

#define SCNi64       "lli"
#define SCNiLEAST64  "lli"
#define SCNiFAST64   "lli"

#define SCNo64       "llo"
#define SCNoLEAST64  "llo"
#define SCNoFAST64   "llo"

#define SCNu64       "llu"
#define SCNuLEAST64  "llu"
#define SCNuFAST64   "llu"

#define SCNx64       "llx"
#define SCNxLEAST64  "llx"
#define SCNxFAST64   "llx"

#else

#define PRIdPTR  "ld"
#define PRIiPTR  "li"
#define PRIoPTR  "lo"
#define PRIuPTR  "lu"
#define PRIxPTR  "lx"
#define PRIXPTR  "lX"
#define SCNdPTR  "ld"
#define SCNiPTR  "li"
#define SCNoPTR  "lo"
#define SCNuPTR  "lu"
#define SCNxPTR  "lx"

#define PRId64       "ld"
#define PRIdLEAST64  "ld"
#define PRIdFAST64   "ld"

#define PRIi64       "li"
#define PRIiLEAST64  "li"
#define PRIiFAST64   "li"

#define PRIo64       "lo"
#define PRIoLEAST64  "lo"
#define PRIoFAST64   "lo"

#define PRIu64       "lu"
#define PRIuLEAST64  "lu"
#define PRIuFAST64   "lu"

#define PRIx64       "lx"
#define PRIxLEAST64  "lx"
#define PRIxFAST64   "lx"

#define PRIX64       "lX"
#define PRIXLEAST64  "lX"
#define PRIXFAST64   "lX"

#define SCNd64       "ld"
#define SCNdLEAST64  "ld"
#define SCNdFAST64   "ld"

#define SCNi64       "li"
#define SCNiLEAST64  "li"
#define SCNiFAST64   "li"

#define SCNo64       "lo"
#define SCNoLEAST64  "lo"
#define SCNoFAST64   "lo"

#define SCNu64       "lu"
#define SCNuLEAST64  "lu"
#define SCNuFAST64   "lu"

#define SCNx64       "lx"
#define SCNxLEAST64  "lx"
#define SCNxFAST64   "lx"

#endif

#define PRIdMAX  "lld"
#define PRIiMAX  "lli"
#define PRIoMAX  "llo"
#define PRIuMAX  "llu"
#define PRIxMAX  "llx"
#define PRIXMAX  "llX"
#define SCNdMAX  "lld"
#define SCNiMAX  "lli"
#define SCNoMAX  "llo"
#define SCNuMAX  "llu"
#define SCNxMAX  "llx"

#if defined(_HELENOS_SOURCE) && !defined(__cplusplus)
#define PRIdn  PRIdPTR  /**< Format for native_t. */
#define PRIun  PRIuPTR  /**< Format for sysarg_t. */
#define PRIxn  PRIxPTR  /**< Format for hexadecimal sysarg_t. */
#endif

__C_DECLS_BEGIN;

typedef struct {
	intmax_t quot;
	intmax_t rem;
} imaxdiv_t;

extern intmax_t imaxabs(intmax_t);
extern imaxdiv_t imaxdiv(intmax_t, intmax_t);
extern intmax_t strtoimax(const char *__restrict__, char **__restrict__, int);
extern uintmax_t strtoumax(const char *__restrict__, char **__restrict__, int);
extern intmax_t wcstoimax(const wchar_t *__restrict__, wchar_t **__restrict__,
    int);
extern uintmax_t wcstoumax(const wchar_t *__restrict__, wchar_t **__restrict__,
    int);

__C_DECLS_END;

#endif

/** @}
 */
