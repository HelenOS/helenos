/*
 * SPDX-FileCopyrightText: 2011 Jiri Zarevucky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libposix
 * @{
 */
/** @file Locale-specific definitions.
 */

#ifndef POSIX_LOCALE_H_
#define POSIX_LOCALE_H_

#include <stddef.h>
#include <_bits/decls.h>

#define LC_ALL 0
#define LC_COLLATE 1
#define LC_CTYPE 2
#define LC_MESSAGES 3
#define LC_MONETARY 4
#define LC_NUMERIC 5
#define LC_TIME 6

#define LC_COLLATE_MASK (1 << 0)
#define LC_CTYPE_MASK (1 << 1)
#define LC_MESSAGES_MASK (1 << 2)
#define LC_MONETARY_MASK (1 << 3)
#define LC_NUMERIC_MASK (1 << 4)
#define LC_TIME_MASK (1 << 5)
#define LC_ALL_MASK (LC_COLLATE_MASK | LC_CTYPE_MASK | LC_MESSAGES_MASK | \
    LC_MONETARY_MASK | LC_NUMERIC_MASK | LC_TIME_MASK)

#define LC_GLOBAL_LOCALE NULL

__C_DECLS_BEGIN;

#ifndef __locale_t_defined
#define __locale_t_defined
typedef struct __posix_locale *locale_t;
#endif

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

__C_DECLS_END;

#endif /* POSIX_LOCALE_H_ */

/** @}
 */
