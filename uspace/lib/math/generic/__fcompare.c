/*
 * SPDX-FileCopyrightText: 2018 CZ.NIC, z.s.p.o.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <math.h>
#include <stdarg.h>

/**
 * Fallback symbol used when code including <math.h> is compiled with something
 * other than GCC or Clang. The function itself must be built with GCC or Clang.
 */
int __fcompare(size_t sz1, size_t sz2, ...)
{
	va_list ap;
	va_start(ap, sz2);

	long double val1;
	long double val2;

	switch (sz1) {
	case 4:
		val1 = (long double) va_arg(ap, double);
		break;
	case 8:
		val1 = (long double) va_arg(ap, double);
		break;
	default:
		val1 = va_arg(ap, long double);
		break;
	}

	switch (sz2) {
	case 4:
		val2 = (long double) va_arg(ap, double);
		break;
	case 8:
		val2 = (long double) va_arg(ap, double);
		break;
	default:
		val2 = va_arg(ap, long double);
		break;
	}

	va_end(ap);

	if (isgreaterequal(val1, val2)) {
		if (isgreater(val1, val2))
			return __FCOMPARE_GREATER;
		else
			return __FCOMPARE_EQUAL;
	} else {
		if (isless(val1, val2))
			return __FCOMPARE_LESS;
		else
			return 0;
	}
}
