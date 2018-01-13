/*
 * Copyright (c) 2016 Jiri Svoboda
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
/** @file Internet host name
 */

#include <ctype.h>
#include <errno.h>
#include <inet/hostname.h>
#include <stdlib.h>
#include <stddef.h>
#include <str.h>

/** Parse host name
 *
 * Determine whether string begins with a valid host name and where that
 * host name ends.
 *
 * Currntly accepting names per 'host' definition in RFC 1738
 * (Uniform Resource Locators) - generic URI syntax not accepted.
 *
 * @param str String containing host name
 * @param rname Place to store pointer to new string containing just the
 *              host name or @c NULL if not interested
 * @param endptr Place to store pointer to next character in string or @c NULL
 *               if no extra characters in the string are allowed
 *
 * @return EOK on success. EINVAL if @a str does not start with a valid
 *         host name or if @a endptr is @c NULL and @a str contains
 *         extra characters at the end. ENOMEM if out of memory
 */
errno_t inet_hostname_parse(const char *str, char **rname, char **endptr)
{
	const char *c;
	const char *dstart = NULL;
	char *name;

	c = str;
	while (isalnum(*c)) {
		/* Domain label */
		dstart = c;

		do {
			++c;
		} while (isalnum(*c) || *c == '-');

		/* Domain separator? */
		if (*c != '.' || !isalnum(c[1]))
			break;

		++c;
	}

	/*
	 * Last domain label must not start with a digit (to distinguish from
	 * IPv4 address)
	 */
	if (dstart == NULL || !isalpha(*dstart))
		return EINVAL;

	/* If endptr is null, check that entire string matched */
	if (endptr == NULL && *c != '\0')
		return EINVAL;

	if (endptr != NULL)
		*endptr = (char *)c;

	if (rname != NULL) {
		name = str_ndup(str, c - str);
		if (name == NULL)
			return ENOMEM;

		*rname = name;
	}

	return EOK;
}

/** @}
 */
