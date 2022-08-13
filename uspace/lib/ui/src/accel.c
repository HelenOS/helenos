/*
 * SPDX-FileCopyrightText: 2022 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libui
 * @{
 */
/**
 * @file Accelerator processing
 */

#include <ctype.h>
#include <errno.h>
#include <stdlib.h>
#include <str.h>
#include <ui/accel.h>

/** Process text with accelerator markup.
 *
 * Parse text with tilde markup into a list of strings. @a *rbuf is set
 * to point to a newly allocated buffer containing consecutive null-terminated
 * strings.
 *
 * Each part between two '~' becomes one string. '~~' is translated into
 * a literal '~' character. @a *endptr is set to point to the first character
 * beyond the end of the list.
 *
 * @param str String with tilde markup
 * @param rbuf Place to store pointer to newly allocated buffer.
 * @param endptr Place to store end pointer (just after last character)
 * @return EOK on success or an error code
 */
errno_t ui_accel_process(const char *str, char **rbuf, char **endptr)
{
	const char *sp;
	char *dp;
	char *buf;

	buf = malloc(str_size(str) + 1);
	if (buf == NULL)
		return ENOMEM;

	/* Break down string into list of (non)highlighted parts */
	sp = str;
	dp = buf;
	while (*sp != '\0') {
		if (*sp == '~') {
			if (sp[1] == '~') {
				sp += 2;
				*dp++ = '~';
			} else {
				++sp;
				*dp++ = '\0';
			}
		} else {
			*dp++ = *sp++;
		}
	}

	*dp++ = '\0';
	*rbuf = buf;
	*endptr = dp;

	return EOK;
}

/** Get accelerator character from marked-up string.
 *
 * @param str String with '~' accelerator markup
 *
 * @return Accelerator character (lowercase) or the null character if
 *         the menu has no accelerator.
 */
char32_t ui_accel_get(const char *str)
{
	const char *sp = str;
	size_t off;

	while (*sp != '\0') {
		if (*sp == '~') {
			if (sp[1] == '~') {
				++sp;
			} else {
				/* Found the accelerator */
				++sp;
				break;
			}
		}

		++sp;
	}

	if (*sp == '\0') {
		/* No accelerator */
		return '\0';
	}

	/* Decode accelerator character */
	off = 0;

	/*
	 * NOTE: tolower is unlike to actually convert to lowercase
	 * in case of non-ASCII characters
	 */
	return tolower(str_decode(sp, &off, STR_NO_LIMIT));
}

/** @}
 */
