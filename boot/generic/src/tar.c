/*
 * Copyright (c) 2018 Jiří Zárevúcky
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

#include <align.h>
#include <tar.h>

static int _digit_val(char c)
{
	if (c >= '0' && c <= '9') {
		return c - '0';
	}
	if (c >= 'a' && c <= 'z') {
		return c - 'a' + 10;
	}
	if (c >= 'A' && c <= 'Z') {
		return c - 'A' + 10;
	}

	// TODO: error message
	return 0;
}

static int64_t _parse_size(const char *s, size_t len, int base)
{
	int64_t val = 0;

	for (size_t i = 0; i < len; i++) {
		if (*s == 0)
			return val;

		if (*s == ' ') {
			s++;
			continue;
		}

		if ((INT64_MAX - 64) / base <= val) {
			// TODO: error message
			return INT64_MAX;
		}

		val *= base;
		val += _digit_val(*s);
		s++;
	}

	return val;
}

bool tar_info(const uint8_t *start, const uint8_t *end,
    const char **name, size_t *length)
{
	if (end - start < TAR_BLOCK_SIZE) {
		// TODO: error message
		return false;
	}

	const tar_header_raw_t *h = (const tar_header_raw_t *) start;
	if (h->filename[0] == '\0')
		return false;

	ptrdiff_t len = _parse_size(h->size, sizeof(h->size), 8);

	if (end - start < TAR_BLOCK_SIZE + len) {
		// TODO: error message
		return false;
	}

	*name = h->filename;
	*length = len;
	return true;
}
