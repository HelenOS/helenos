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

#ifndef POSIX_LOCALE_H_
#define POSIX_LOCALE_H_

#include <_bits/NULL.h>

#ifndef __locale_t_defined
#define __locale_t_defined
typedef struct __posix_locale *locale_t;
#endif

#undef LC_ALL
#undef LC_COLLATE
#undef LC_CTYPE
#undef LC_MESSAGES
#undef LC_MONETARY
#undef LC_NUMERIC
#undef LC_TIME
#define LC_ALL 0
#define LC_COLLATE 1
#define LC_CTYPE 2
#define LC_MESSAGES 3
#define LC_MONETARY 4
#define LC_NUMERIC 5
#define LC_TIME 6

#undef LC_COLLATE_MASK
#undef LC_CTYPE_MASK
#undef LC_MESSAGES_MASK
#undef LC_MONETARY_MASK
#undef LC_NUMERIC_MASK
#undef LC_TIME_MASK
#undef LC_ALL_MASK
#define LC_COLLATE_MASK (1 << 0)
#define LC_CTYPE_MASK (1 << 1)
#define LC_MESSAGES_MASK (1 << 2)
#define LC_MONETARY_MASK (1 << 3)
#define LC_NUMERIC_MASK (1 << 4)
#define LC_TIME_MASK (1 << 5)
#define LC_ALL_MASK (LC_COLLATE_MASK | LC_CTYPE_MASK | LC_MESSAGES_MASK | \
    LC_MONETARY_MASK | LC_NUMERIC_MASK | LC_TIME_MASK)

#undef LC_GLOBAL_LOCALE
#define LC_GLOBAL_LOCALE NULL

struct lconv {
	char *currency_symbol;
	char *decimal_point;
	char  frac_digits;
	char *grouping;
	char *int_curr_symbol;
	char  int_frac_digits;
	char  int_n_cs_precedes;
	char  int_n_sep_by_space;
	char  int_n_sign_posn;
	char  int_p_cs_precedes;
	char  int_p_sep_by_space;
	char  int_p_sign_posn;
	char *mon_decimal_point;
	char *mon_grouping;
	char *mon_thousands_sep;
	char *negative_sign;
	char  n_cs_precedes;
	char  n_sep_by_space;
	char  n_sign_posn;
	char *positive_sign;
	char  p_cs_precedes;
	char  p_sep_by_space;
	char  p_sign_posn;
	char *thousands_sep;
};

extern char *setlocale(int category, const char *locale);
extern struct lconv *localeconv(void);

/* POSIX Extensions */
extern locale_t duplocale(locale_t locobj);
extern void freelocale(locale_t locobj);
extern locale_t newlocale(int category_mask, const char *locale,
    locale_t base);
extern locale_t uselocale(locale_t newloc);


#endif /* POSIX_LOCALE_H_ */

/** @}
 */
