/*
 * SPDX-FileCopyrightText: 2018 Jiří Zárevúcky
 *
 * SPDX-License-Identifier: BSD-3-Clause
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
