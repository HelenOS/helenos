/*
 * SPDX-FileCopyrightText: 2011 Vojtech Horky
 *
 * SPDX-License-Identifier: BSD-3-Clause
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
