/*
 * SPDX-FileCopyrightText: 2011 Jiri Zarevucky
 * SPDX-FileCopyrightText: 2011 Petr Koupy
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libposix
 * @{
 */
/** @file Additional string manipulation.
 */

#include "internal/common.h"
#include <strings.h>

#include <string.h>
#include <ctype.h>

#include <mem.h>
#include <str.h>

/**
 * Find first set bit (beginning with the least significant bit).
 *
 * @param i Integer in which to look for the first set bit.
 * @return Index of first set bit. Bits are numbered starting at one.
 */
int ffs(int i)
{
	if (i == 0) {
		return 0;
	}

	int result = 0;

	// XXX: assumes at most 32-bit int
	if (!(i & 0xFFFF)) {
		result |= 16;
		i >>= 16;
	}
	if (!(i & 0xFF)) {
		result |= 8;
		i >>= 8;
	}
	if (!(i & 0xF)) {
		result |= 4;
		i >>= 4;
	}
	if (!(i & 0x3)) {
		result |= 2;
		i >>= 2;
	}
	if (!(i & 0x1)) {
		result |= 1;
	}

	return result + 1;
}

/**
 * Compare two strings (case-insensitive).
 *
 * @param s1 First string to be compared.
 * @param s2 Second string to be compared.
 * @return Difference of the first pair of inequal characters,
 *     or 0 if strings have the same content.
 */
int strcasecmp(const char *s1, const char *s2)
{
	return strncasecmp(s1, s2, STR_NO_LIMIT);
}

/**
 * Compare part of two strings (case-insensitive).
 *
 * @param s1 First string to be compared.
 * @param s2 Second string to be compared.
 * @param n Maximum number of characters to be compared.
 * @return Difference of the first pair of inequal characters,
 *     or 0 if strings have the same content.
 */
int strncasecmp(const char *s1, const char *s2, size_t n)
{
	for (size_t i = 0; i < n; ++i) {
		int cmp = tolower(s1[i]) - tolower(s2[i]);
		if (cmp != 0) {
			return cmp;
		}

		if (s1[i] == 0) {
			return 0;
		}
	}

	return 0;
}

/**
 * Compare two memory areas.
 *
 * @param mem1 Pointer to the first area to compare.
 * @param mem2 Pointer to the second area to compare.
 * @param n Common size of both areas.
 * @return If n is 0, return zero. If the areas match, return
 *     zero. Otherwise return non-zero.
 */
int bcmp(const void *mem1, const void *mem2, size_t n)
{
	return memcmp(mem1, mem2, n);
}

/**
 * Copy bytes in memory with overlapping areas.
 *
 * @param src Source area.
 * @param dest Destination area.
 * @param n Number of bytes to copy.
 */
void bcopy(const void *src, void *dest, size_t n)
{
	/* Note that memmove has different order of arguments. */
	memmove(dest, src, n);
}

/**
 * Reset bytes in memory area to zero.
 *
 * @param mem Memory area to be zeroed.
 * @param n Number of bytes to reset.
 */
void bzero(void *mem, size_t n)
{
	memset(mem, 0, n);
}

/**
 * Scan string for a first occurence of a character.
 *
 * @param s String in which to look for the character.
 * @param c Character to look for.
 * @return Pointer to the specified character on success,
 *     NULL pointer otherwise.
 */
char *index(const char *s, int c)
{
	return strchr(s, c);
}

/**
 * Scan string for a last occurence of a character.
 *
 * @param s String in which to look for the character.
 * @param c Character to look for.
 * @return Pointer to the specified character on success,
 *     NULL pointer otherwise.
 */
char *rindex(const char *s, int c)
{
	return strrchr(s, c);
}

/** @}
 */
