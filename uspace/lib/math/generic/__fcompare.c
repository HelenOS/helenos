/*
 * Copyright (c) 2018 CZ.NIC, z.s.p.o.
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
