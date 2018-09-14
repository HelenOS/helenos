/*
 * Copyright (c) 2011 Jiri Zarevucky
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

/** @addtogroup libposix
 * @{
 */
/** @file Locale-specific definitions.
 */

#include "internal/common.h"
#include <locale.h>

#include <errno.h>

#include <limits.h>
#include <string.h>

/*
 * Just a very basic dummy implementation.
 * This should allow code using locales to work properly, but doesn't provide
 * any localization functionality.
 * Should be extended/rewritten when or if HelenOS supports locales natively.
 */

struct __posix_locale {
	int _dummy;
};

const struct lconv C_LOCALE = {
	.currency_symbol = (char *) "",
	.decimal_point = (char *) ".",
	.frac_digits = CHAR_MAX,
	.grouping = (char *) "",
	.int_curr_symbol = (char *) "",
	.int_frac_digits = CHAR_MAX,
	.int_n_cs_precedes = CHAR_MAX,
	.int_n_sep_by_space = CHAR_MAX,
	.int_n_sign_posn = CHAR_MAX,
	.int_p_cs_precedes = CHAR_MAX,
	.int_p_sep_by_space = CHAR_MAX,
	.int_p_sign_posn = CHAR_MAX,
	.mon_decimal_point = (char *) "",
	.mon_grouping = (char *) "",
	.mon_thousands_sep = (char *) "",
	.negative_sign = (char *) "",
	.n_cs_precedes = CHAR_MAX,
	.n_sep_by_space = CHAR_MAX,
	.n_sign_posn = CHAR_MAX,
	.positive_sign = (char *) "",
	.p_cs_precedes = CHAR_MAX,
	.p_sep_by_space = CHAR_MAX,
	.p_sign_posn = CHAR_MAX,
	.thousands_sep = (char *) ""
};

/**
 * Set program locale.
 *
 * @param category What category to set.
 * @param locale Locale name.
 * @return Original locale name on success, NULL on failure.
 */
char *setlocale(int category, const char *locale)
{
	// TODO
	if (locale == NULL || *locale == '\0' ||
	    strcmp(locale, "C") == 0) {
		return (char *) "C";
	}
	return NULL;
}

/**
 * Return locale-specific information.
 *
 * @return Information about the current locale.
 */
struct lconv *localeconv(void)
{
	// TODO
	return (struct lconv *) &C_LOCALE;
}

/**
 * Duplicate locale object.
 *
 * @param locobj Object to duplicate.
 * @return Duplicated object.
 */
locale_t duplocale(locale_t locobj)
{
	if (locobj == NULL) {
		errno = EINVAL;
		return NULL;
	}
	locale_t copy = malloc(sizeof(struct __posix_locale));
	if (copy == NULL) {
		errno = ENOMEM;
		return NULL;
	}
	memcpy(copy, locobj, sizeof(struct __posix_locale));
	return copy;
}

/**
 * Free locale object.
 *
 * @param locobj Object to free.
 */
void freelocale(locale_t locobj)
{
	if (locobj) {
		free(locobj);
	}
}

/**
 * Create or modify a locale object.
 *
 * @param category_mask Mask of categories to be set or modified.
 * @param locale Locale to be used.
 * @param base Object to modify. 0 if new object is to be created.
 * @return The new/modified locale object.
 */
locale_t newlocale(int category_mask, const char *locale,
    locale_t base)
{
	if (locale == NULL ||
	    (category_mask & LC_ALL_MASK) != category_mask) {
		errno = EINVAL;
		return NULL;
	}
	// TODO
	locale_t new = malloc(sizeof(struct __posix_locale));
	if (new == NULL) {
		errno = ENOMEM;
		return NULL;
	}
	if (base != NULL) {
		freelocale(base);
	}
	return new;
}

/**
 * Set locale for the current thread.
 *
 * @param newloc Locale to use.
 * @return The previously set locale or LC_GLOBAL_LOCALE
 */
locale_t uselocale(locale_t newloc)
{
	// TODO
	return LC_GLOBAL_LOCALE;
}

/** @}
 */
