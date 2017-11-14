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

#ifndef _BITS_LIMITS_H_
#define _BITS_LIMITS_H_

#include <_bits/macros.h>

/* _MIN macros for unsigned types are non-standard (and of course, always 0),
 * but we already have them for some reason, so whatever.
 */

#define CHAR_BIT  __CHAR_BIT__

#define SCHAR_MIN  __SCHAR_MIN__
#define SCHAR_MAX  __SCHAR_MAX__

#define UCHAR_MIN  0
#define UCHAR_MAX  __UCHAR_MAX__

#define CHAR_MIN  __CHAR_MIN__
#define CHAR_MAX  __CHAR_MAX__

#define MB_LEN_MAX  16

#define SHRT_MIN  __SHRT_MIN__
#define SHRT_MAX  __SHRT_MAX__

#define USHRT_MIN  0
#define USHRT_MAX  __USHRT_MAX__

#define INT_MIN  __INT_MIN__
#define INT_MAX  __INT_MAX__

#define UINT_MIN  0U
#define UINT_MAX  __UINT_MAX__

#define LONG_MIN  __LONG_MIN__
#define LONG_MAX  __LONG_MAX__

#define ULONG_MIN  0UL
#define ULONG_MAX  __ULONG_MAX__

#define LLONG_MIN  __LLONG_MIN__
#define LLONG_MAX  __LLONG_MAX__

#define ULLONG_MIN  0ULL
#define ULLONG_MAX  __ULLONG_MAX__

#endif

/** @}
 */
