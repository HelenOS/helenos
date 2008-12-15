/*
 * Copyright (C) 2006 Josef Cejka
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

#ifndef __LIBC__LIMITS_H__
#define __LIBC__LIMITS_H__

#include <stdint.h>
#include <libarch/limits.h>

/* char */
#define SCHAR_MIN MIN_INT8
#define SCHAR_MAX MAX_INT8
#define UCHAR_MIN MIN_UINT8
#define UCHAR_MAX MAX_UINT8

#ifdef __CHAR_UNSIGNED__
# define CHAR_MIN UCHAR_MIN
# define CHAR_MAX UCHAR_MAX
#else
# define CHAR_MIN SCHAR_MIN
# define CHAR_MAX SCHAR_MAX
#endif

/* short int */
#define SHRT_MIN MIN_INT16
#define SHRT_MAX MAX_INT16
#define USHRT_MIN MIN_UINT16
#define USHRT_MAX MAX_UINT16

#define INT_MIN MIN_INT32
#define INT_MAX MAX_INT32
#define UINT_MIN MIN_UINT32
#define UINT_MAX MAX_UINT32

#define LLONG_MIN MIN_INT64
#define LLONG_MAX MAX_INT64
#define ULLONG_MIN MIN_UINT64
#define ULLONG_MAX MAX_UINT64

#endif



 /** @}
 */
 
 
