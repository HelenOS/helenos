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
 */

#ifndef _BITS_INTTYPES_H_
#define _BITS_INTTYPES_H_

#include <_bits/macros.h>
#include <_bits/stdint.h>
#include <_bits/wchar_t.h>

#define PRId8       __PRId8__
#define PRIdLEAST8  __PRId8__
#define PRIdFAST8   __PRId8__

#define PRIi8       __PRIi8__
#define PRIiLEAST8  __PRIi8__
#define PRIiFAST8   __PRIi8__

#define PRIo8       __PRIo8__
#define PRIoLEAST8  __PRIo8__
#define PRIoFAST8   __PRIo8__

#define PRIu8       __PRIu8__
#define PRIuLEAST8  __PRIu8__
#define PRIuFAST8   __PRIu8__

#define PRIx8       __PRIx8__
#define PRIxLEAST8  __PRIx8__
#define PRIxFAST8   __PRIx8__

#define PRIX8       __PRIX8__
#define PRIXLEAST8  __PRIX8__
#define PRIXFAST8   __PRIX8__

#define SCNd8       __SCNd8__
#define SCNdLEAST8  __SCNd8__
#define SCNdFAST8   __SCNd8__

#define SCNi8       __SCNi8__
#define SCNiLEAST8  __SCNi8__
#define SCNiFAST8   __SCNi8__

#define SCNo8       __SCNo8__
#define SCNoLEAST8  __SCNo8__
#define SCNoFAST8   __SCNo8__

#define SCNu8       __SCNu8__
#define SCNuLEAST8  __SCNu8__
#define SCNuFAST8   __SCNu8__

#define SCNx8       __SCNx8__
#define SCNxLEAST8  __SCNx8__
#define SCNxFAST8   __SCNx8__

#define PRId16       __PRId16__
#define PRIdLEAST16  __PRId16__
#define PRIdFAST16   __PRId16__

#define PRIi16       __PRIi16__
#define PRIiLEAST16  __PRIi16__
#define PRIiFAST16   __PRIi16__

#define PRIo16       __PRIo16__
#define PRIoLEAST16  __PRIo16__
#define PRIoFAST16   __PRIo16__

#define PRIu16       __PRIu16__
#define PRIuLEAST16  __PRIu16__
#define PRIuFAST16   __PRIu16__

#define PRIx16       __PRIx16__
#define PRIxLEAST16  __PRIx16__
#define PRIxFAST16   __PRIx16__

#define PRIX16       __PRIX16__
#define PRIXLEAST16  __PRIX16__
#define PRIXFAST16   __PRIX16__

#define SCNd16       __SCNd16__
#define SCNdLEAST16  __SCNd16__
#define SCNdFAST16   __SCNd16__

#define SCNi16       __SCNi16__
#define SCNiLEAST16  __SCNi16__
#define SCNiFAST16   __SCNi16__

#define SCNo16       __SCNo16__
#define SCNoLEAST16  __SCNo16__
#define SCNoFAST16   __SCNo16__

#define SCNu16       __SCNu16__
#define SCNuLEAST16  __SCNu16__
#define SCNuFAST16   __SCNu16__

#define SCNx16       __SCNx16__
#define SCNxLEAST16  __SCNx16__
#define SCNxFAST16   __SCNx16__

#define PRId32       __PRId32__
#define PRIdLEAST32  __PRId32__
#define PRIdFAST32   __PRId32__

#define PRIi32       __PRIi32__
#define PRIiLEAST32  __PRIi32__
#define PRIiFAST32   __PRIi32__

#define PRIo32       __PRIo32__
#define PRIoLEAST32  __PRIo32__
#define PRIoFAST32   __PRIo32__

#define PRIu32       __PRIu32__
#define PRIuLEAST32  __PRIu32__
#define PRIuFAST32   __PRIu32__

#define PRIx32       __PRIx32__
#define PRIxLEAST32  __PRIx32__
#define PRIxFAST32   __PRIx32__

#define PRIX32       __PRIX32__
#define PRIXLEAST32  __PRIX32__
#define PRIXFAST32   __PRIX32__

#define SCNd32       __SCNd32__
#define SCNdLEAST32  __SCNd32__
#define SCNdFAST32   __SCNd32__

#define SCNi32       __SCNi32__
#define SCNiLEAST32  __SCNi32__
#define SCNiFAST32   __SCNi32__

#define SCNo32       __SCNo32__
#define SCNoLEAST32  __SCNo32__
#define SCNoFAST32   __SCNo32__

#define SCNu32       __SCNu32__
#define SCNuLEAST32  __SCNu32__
#define SCNuFAST32   __SCNu32__

#define SCNx32       __SCNx32__
#define SCNxLEAST32  __SCNx32__
#define SCNxFAST32   __SCNx32__

#define PRId64       __PRId64__
#define PRIdLEAST64  __PRId64__
#define PRIdFAST64   __PRId64__

#define PRIi64       __PRIi64__
#define PRIiLEAST64  __PRIi64__
#define PRIiFAST64   __PRIi64__

#define PRIo64       __PRIo64__
#define PRIoLEAST64  __PRIo64__
#define PRIoFAST64   __PRIo64__

#define PRIu64       __PRIu64__
#define PRIuLEAST64  __PRIu64__
#define PRIuFAST64   __PRIu64__

#define PRIx64       __PRIx64__
#define PRIxLEAST64  __PRIx64__
#define PRIxFAST64   __PRIx64__

#define PRIX64       __PRIX64__
#define PRIXLEAST64  __PRIX64__
#define PRIXFAST64   __PRIX64__

#define SCNd64       __SCNd64__
#define SCNdLEAST64  __SCNd64__
#define SCNdFAST64   __SCNd64__

#define SCNi64       __SCNi64__
#define SCNiLEAST64  __SCNi64__
#define SCNiFAST64   __SCNi64__

#define SCNo64       __SCNo64__
#define SCNoLEAST64  __SCNo64__
#define SCNoFAST64   __SCNo64__

#define SCNu64       __SCNu64__
#define SCNuLEAST64  __SCNu64__
#define SCNuFAST64   __SCNu64__

#define SCNx64       __SCNx64__
#define SCNxLEAST64  __SCNx64__
#define SCNxFAST64   __SCNx64__

#define PRIdPTR  __PRIdPTR__
#define PRIiPTR  __PRIiPTR__
#define PRIoPTR  __PRIoPTR__
#define PRIuPTR  __PRIuPTR__
#define PRIxPTR  __PRIxPTR__
#define PRIXPTR  __PRIXPTR__
#define SCNdPTR  __SCNdPTR__
#define SCNiPTR  __SCNiPTR__
#define SCNoPTR  __SCNoPTR__
#define SCNuPTR  __SCNuPTR__
#define SCNxPTR  __SCNxPTR__

#ifndef __HELENOS_DISABLE_INTMAX__

#define PRIdMAX  __PRIdMAX__
#define PRIiMAX  __PRIiMAX__
#define PRIoMAX  __PRIoMAX__
#define PRIuMAX  __PRIuMAX__
#define PRIxMAX  __PRIxMAX__
#define PRIXMAX  __PRIXMAX__
#define SCNdMAX  __SCNdMAX__
#define SCNiMAX  __SCNiMAX__
#define SCNoMAX  __SCNoMAX__
#define SCNuMAX  __SCNuMAX__
#define SCNxMAX  __SCNxMAX__

typedef struct {
	intmax_t quot;
	intmax_t rem;
} imaxdiv_t;

#endif  /* __HELENOS_DISABLE_INTMAX__ */

#endif

/** @}
 */
