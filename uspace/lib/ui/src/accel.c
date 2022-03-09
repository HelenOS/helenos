/*
 * Copyright (c) 2022 Jiri Svoboda
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
