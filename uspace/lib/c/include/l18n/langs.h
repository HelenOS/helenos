/*
 * SPDX-FileCopyrightText: 2005 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
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
