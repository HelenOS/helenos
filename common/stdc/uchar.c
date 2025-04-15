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

#include <assert.h>
#include <errno.h>
#include <limits.h>
#include <stdbool.h>
#include <uchar.h>

#if __STDC_HOSTED__
#include <fibril.h>
#endif

static void _set_ilseq()
{
#ifdef errno
	errno = EILSEQ;
#endif
}

static bool _is_low_surrogate(char16_t c)
{
	return c >= 0xDC00 && c < 0xE000;
}

static bool _is_high_surrogate(char16_t c)
{
	return c >= 0xD800 && c < 0xDC00;
}

static bool _is_surrogate(char16_t c)
{
	return c >= 0xD800 && c < 0xE000;
}

#define UTF8_CONT(c, shift) (0x80 | (((c) >> (shift)) & 0x3F))

size_t c32rtomb(char *s, char32_t c, mbstate_t *mb)
{
	if (!s) {
		// Equivalent to c32rtomb(buf, L’\0’, mb).
		return 1;
	}

	/* 1 byte encoding */
	if (c < 0x80) {
		s[0] = c;
		return 1;
	}

	/* 2 byte encoding */
	if (c < 0x800) {
		s[0] = 0b11000000 | (c >> 6);
		s[1] = UTF8_CONT(c, 0);
		return 2;
	}

	/* 3 byte encoding */
	if (c < 0x10000) {
		if (_is_surrogate(c)) {
			/* illegal range for an unicode code point */
			_set_ilseq();
			return UCHAR_ILSEQ;
		}

		s[0] = 0b11100000 | (c >> 12);
		s[1] = UTF8_CONT(c, 6);
		s[2] = UTF8_CONT(c, 0);
		return 3;
	}

	/* 4 byte encoding */
	if (c < 0x110000) {
		s[0] = 0b11110000 | (c >> 18);
		s[1] = UTF8_CONT(c, 12);
		s[2] = UTF8_CONT(c, 6);
		s[3] = UTF8_CONT(c, 0);
		return 4;
	}

	_set_ilseq();
	return UCHAR_ILSEQ;
}

size_t mbrtoc16(char16_t *c, const char *s, size_t n, mbstate_t *mb)
{
#if __STDC_HOSTED__
	static fibril_local mbstate_t global_state = { };

	if (!mb)
		mb = &global_state;
#else
	assert(mb);
#endif

	char16_t dummy;

	if (!c)
		c = &dummy;

	if (!s) {
		/* Equivalent to mbrtoc16(NULL, "", 1, mb). */
		if (mb->state) {
			_set_ilseq();
			return UCHAR_ILSEQ;
		} else {
			return 0;
		}
	}

	if ((mb->state & 0xD000) == 0xD000) {
		/* mbstate_t contains the second surrogate character. */
		/* mbrtoc32() will never set it to such value.        */
		*c = mb->state;
		mb->state = 0;
		return UCHAR_CONTINUED;
	}

	char32_t c32 = 0;
	size_t ret = mbrtoc32(&c32, s, n, mb);
	if (ret < INT_MAX) {
		if (c32 < 0x10000) {
			*c = c32;
		} else {
			/* Encode UTF-16 surrogates. */
			mb->state = (c32 & 0x3FF) + 0xDC00;
			*c = (c32 >> 10) + 0xD7C0;
		}
		return ret;
	}

	return ret;
}

size_t c16rtomb(char *s, char16_t c, mbstate_t *mb)
{
#if __STDC_HOSTED__
	static fibril_local mbstate_t global_state = { };

	if (!mb)
		mb = &global_state;
#else
	assert(mb);
#endif

	if (!s) {
		// Equivalent to c16rtomb(buf, L’\0’, mb).
		if (mb->state) {
			_set_ilseq();
			return UCHAR_ILSEQ;
		} else {
			return 1;
		}
	}

	if (!_is_surrogate(c)) {
		if (mb->state) {
			_set_ilseq();
			return UCHAR_ILSEQ;
		}

		return c32rtomb(s, c, mb);
	}

	if (!mb->state) {
		mb->state = c;
		return 0;
	}

	char32_t c32;

	/* Decode UTF-16 surrogates. */
	if (_is_low_surrogate(mb->state) && _is_high_surrogate(c)) {
		c32 = ((c - 0xD7C0) << 10) | (mb->state - 0xDC00);
	} else if (_is_high_surrogate(mb->state) && _is_low_surrogate(c)) {
		c32 = ((mb->state - 0xD7C0) << 10) | (c - 0xDC00);
	} else {
		_set_ilseq();
		return UCHAR_ILSEQ;
	}

	mb->state = 0;
	return c32rtomb(s, c32, mb);
}
