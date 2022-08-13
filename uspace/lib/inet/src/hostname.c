/*
 * SPDX-FileCopyrightText: 2016 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libinet
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
