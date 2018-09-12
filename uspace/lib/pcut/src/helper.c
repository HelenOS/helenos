/*
 * Copyright (c) 2018 Vojtech Horky
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

/** @file
 *
 * Helper functions.
 */

#include "internal.h"

#pragma warning(push, 0)
#include <stdarg.h>
#include <stdio.h>
#pragma warning(pop)

/*
 * It seems that not all Unixes are the same and snprintf
 * may not be always exported?
 */
#ifdef __unix
extern int snprintf(char *, size_t, const char *, ...);
#endif


int pcut_snprintf(char *dest, size_t size, const char *format, ...) {
	va_list args;
	int ret;

	va_start(args, format);

	/*
	 * Use sprintf_s in Windows but only with Microsoft compiler.
	 * Namely, let MinGW use snprintf.
	 */
#if (defined(__WIN64) || defined(__WIN32) || defined(_WIN32)) && defined(_MSC_VER)
	ret = _vsnprintf_s(dest, size, _TRUNCATE, format, args);
#else
	ret = vsnprintf(dest, size, format, args);
#endif

	va_end(args);

	return ret;
}
