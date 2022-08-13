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
int __fpclassify(size_t sz, ...)
{
	va_list ap;
	va_start(ap, sz);

	int result;

	switch (sz) {
	case 4:
		result = fpclassify((float) va_arg(ap, double));
		break;
	case 8:
		result = fpclassify(va_arg(ap, double));
		break;
	default:
		result = fpclassify(va_arg(ap, long double));
		break;
	}

	va_end(ap);
	return result;
}
