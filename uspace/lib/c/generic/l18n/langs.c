/*
 * Copyright (c) 2011 Vojtech Horky
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

#include <l18n/langs.h>
#include <stdio.h>
#include <fibril.h>

#define UNKNOWN_LOCALE_LEN 64

static fibril_local char unknown_locale[UNKNOWN_LOCALE_LEN];

/** Get string representation of a given locale.
 *
 * @param locale The locale.
 * @return Name of the locale.
 */
const char *str_l18_win_locale(l18_win_locales_t locale)
{
	/*
	 * Static array with names might be better but it would be
	 * way too big.
	 */
	switch (locale) {
	case L18N_WIN_LOCALE_AFRIKAANS:
		return "Afrikaans";
	case L18N_WIN_LOCALE_CZECH:
		return "Czech";
	case L18N_WIN_LOCALE_ENGLISH_UNITED_STATES:
		return "English (US)";
	case L18N_WIN_LOCALE_SLOVAK:
		return "Slovak";
	case L18N_WIN_LOCALE_SPANISH_TRADITIONAL:
		return "Spanish (traditional)";
	case L18N_WIN_LOCALE_ZULU:
		return "Zulu";
	default:
		break;
	}

	snprintf(unknown_locale, UNKNOWN_LOCALE_LEN, "Unknown locale 0x%04d",
	    (int) locale);
	return unknown_locale;
}

/** @}
 */
