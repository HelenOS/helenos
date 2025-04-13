/*
 * Copyright (c) 2025 Jiří Zárevúcky
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

#include <uchar.h>
#include <wchar.h>

#if __STDC_HOSTED__
#include <fibril.h>
#endif

wint_t btowc(int c)
{
	return (c < 0x80) ? c : WEOF;
}

int wctob(wint_t c)
{
	return c;
}

int mbsinit(const mbstate_t *ps)
{
	return ps == NULL || ps->continuation == 0;
}

size_t mbrlen(const char *s, size_t n, mbstate_t *ps)
{
#if __STDC_HOSTED__
	static fibril_local mbstate_t global_state;
	if (!ps)
		ps = &global_state;
#endif

	return mbrtowc(NULL, s, n, ps);
}

_Static_assert(sizeof(wchar_t) == sizeof(char16_t) || sizeof(wchar_t) == sizeof(char32_t));

size_t mbrtowc(wchar_t *pwc, const char *s, size_t n, mbstate_t *ps)
{
#if __STDC_HOSTED__
	static fibril_local mbstate_t global_state;
	if (!ps)
		ps = &global_state;
#endif

	if (sizeof(wchar_t) == sizeof(char16_t))
		return mbrtoc16((char16_t *) pwc, s, n, ps);
	else
		return mbrtoc32((char32_t *) pwc, s, n, ps);
}

size_t wcrtomb(char *s, wchar_t wc, mbstate_t * ps)
{
#if __STDC_HOSTED__
	static fibril_local mbstate_t global_state;
	if (!ps)
		ps = &global_state;
#endif

	if (sizeof(wchar_t) == sizeof(char16_t))
		return c16rtomb(s, (char16_t) wc, ps);
	else
		return c32rtomb(s, (char32_t) wc, ps);
}
