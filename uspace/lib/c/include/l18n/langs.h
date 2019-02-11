/*
 * Copyright (c) 2005 Jakub Jermar
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
 * Language and locale ids.
 */

#ifndef _LIBC_L18N_LANGS_H_
#define _LIBC_L18N_LANGS_H_

/** Windows locale IDs.
 * Codes taken from
 * Developing International Software For Windows 95 and Windows NT
 * by Nadine Kano (Microsoft Press).
 * FIXME: add missing codes.
 */
typedef enum {
	L18N_WIN_LOCALE_AFRIKAANS = 0x0436,
	/* ... */
	L18N_WIN_LOCALE_CZECH = 0x0405,
	/* ... */
	L18N_WIN_LOCALE_ENGLISH_UNITED_STATES = 0x0409,
	/* ... */
	L18N_WIN_LOCALE_SLOVAK = 0x041B,
	/* ... */
	L18N_WIN_LOCALE_SPANISH_TRADITIONAL = 0x040A,
	/* ... */
	L18N_WIN_LOCALE_ZULU = 0x0435,
	L18N_WIN_LOCALE_MAX = 0xFFFF
} l18_win_locales_t;

const char *str_l18_win_locale(l18_win_locales_t);

#endif

/** @}
 */
