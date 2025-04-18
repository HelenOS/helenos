/*
 * Copyright (c) 2025 Jiří Zárevúcky
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

#ifndef _LIBC_UCHAR_H_
#define _LIBC_UCHAR_H_

#include <_bits/mbstate_t.h>
#include <_bits/size_t.h>
#include <_bits/uchar.h>
#include <stdint.h>

size_t mbrtoc8(char8_t *__restrict pc8, const char *__restrict s, size_t n,
    mbstate_t *__restrict ps);
size_t c8rtomb(char *__restrict s, char8_t c8, mbstate_t *__restrict ps);
size_t mbrtoc16(char16_t *__restrict pc16, const char *__restrict s, size_t n,
    mbstate_t *__restrict ps);
size_t c16rtomb(char *__restrict s, char16_t c16, mbstate_t *__restrict ps);
size_t mbrtoc32(char32_t *__restrict pc32, const char *__restrict s, size_t n,
    mbstate_t *__restrict ps);
size_t c32rtomb(char *__restrict s, char32_t c32, mbstate_t *__restrict ps);

#ifdef _HELENOS_SOURCE
#define UCHAR_ILSEQ      ((size_t) -1)
#define UCHAR_INCOMPLETE ((size_t) -2)
#define UCHAR_CONTINUED  ((size_t) -3)
#endif

#endif

/** @}
 */
