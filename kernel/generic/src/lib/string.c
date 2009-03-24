/*
 * Copyright (c) 2001-2004 Jakub Jermar
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

/** @addtogroup generic
 * @{
 */

/**
 * @file
 * @brief Miscellaneous functions.
 */

#include <string.h>
#include <print.h>
#include <cpu.h>
#include <arch/asm.h>
#include <arch.h>
#include <console/kconsole.h>

/** Decode a single UTF-8 character from a NULL-terminated string.
 *
 * Decode a single UTF-8 character from a plain char NULL-terminated
 * string. Decoding starts at @index and this index is incremented
 * if the current UTF-8 string is encoded in more than a single byte.
 *
 * @param str   Plain character NULL-terminated string.
 * @param index Index (counted in plain characters) where to start
 *              the decoding.
 *
 * @return Decoded character in UTF-32 or '?' if the encoding is wrong.
 *
 */
wchar_t utf8_decode(const char *str, index_t *index)
{
	uint8_t c1;           /* First plain character from str */
	uint8_t c2;           /* Second plain character from str */
	uint8_t c3;           /* Third plain character from str */
	uint8_t c4;           /* Fourth plain character from str */
	
	c1 = (uint8_t) str[*index];
	
	if ((c1 & 0x80) == 0) {
		/* Plain ASCII (code points 0 .. 127) */
		return (wchar_t) c1;
	} else if ((c1 & 0xe0) == 0xc0) {
		/* Code points 128 .. 2047 */
		c2 = (uint8_t) str[*index + 1];
		if ((c2 & 0xc0) == 0x80) {
			(*index)++;
			return ((wchar_t) ((c1 & 0x1f) << 6) | (c2 & 0x3f));
		} else
			return ((wchar_t) '?');
	} else if ((c1 & 0xf0) == 0xe0) {
		/* Code points 2048 .. 65535 */
		c2 = (uint8_t) str[*index + 1];
		if ((c2 & 0xc0) == 0x80) {
			(*index)++;
			c3 = (uint8_t) str[*index + 1];
			if ((c3 & 0xc0) == 0x80) {
				(*index)++;
				return ((wchar_t) ((c1 & 0x0f) << 12) | ((c2 & 0x3f) << 6) | (c3 & 0x3f));
			} else
				return ((wchar_t) '?');
		} else
			return ((wchar_t) '?');
	} else if ((c1 & 0xf8) == 0xf0) {
		/* Code points 65536 .. 1114111 */
		c2 = (uint8_t) str[*index + 1];
		if ((c2 & 0xc0) == 0x80) {
			(*index)++;
			c3 = (uint8_t) str[*index + 1];
			if ((c3 & 0xc0) == 0x80) {
				(*index)++;
				c4 = (uint8_t) str[*index + 1];
				if ((c4 & 0xc0) == 0x80) {
					(*index)++;
					return ((wchar_t) ((c1 & 0x07) << 18) | ((c2 & 0x3f) << 12) | ((c3 & 0x3f) << 6) | (c4 & 0x3f));
				} else
					return ((wchar_t) '?');
			} else
				return ((wchar_t) '?');
		} else
			return ((wchar_t) '?');
	}
	
	return ((wchar_t) '?');
}

/** Return number of characters in a string.
 *
 * @param str NULL terminated string.
 *
 * @return Number of characters in str.
 *
 */
size_t strlen(const char *str)
{
	int i;
	
	for (i = 0; str[i]; i++);
	
	return i;
}

/** Compare two NULL terminated strings
 *
 * Do a char-by-char comparison of two NULL terminated strings.
 * The strings are considered equal iff they consist of the same
 * characters on the minimum of their lengths.
 *
 * @param src First string to compare.
 * @param dst Second string to compare.
 *
 * @return 0 if the strings are equal, -1 if first is smaller, 1 if second smaller.
 *
 */
int strcmp(const char *src, const char *dst)
{
	for (; *src && *dst; src++, dst++) {
		if (*src < *dst)
			return -1;
		if (*src > *dst)
			return 1;
	}
	if (*src == *dst)
		return 0;
	
	if (!*src)
		return -1;
	
	return 1;
}


/** Compare two NULL terminated strings
 *
 * Do a char-by-char comparison of two NULL terminated strings.
 * The strings are considered equal iff they consist of the same
 * characters on the minimum of their lengths and specified maximal
 * length.
 *
 * @param src First string to compare.
 * @param dst Second string to compare.
 * @param len Maximal length for comparison.
 *
 * @return 0 if the strings are equal, -1 if first is smaller, 1 if second smaller.
 *
 */
int strncmp(const char *src, const char *dst, size_t len)
{
	unsigned int i;
	
	for (i = 0; (*src) && (*dst) && (i < len); src++, dst++, i++) {
		if (*src < *dst)
			return -1;
		
		if (*src > *dst)
			return 1;
	}
	
	if (i == len || *src == *dst)
		return 0;
	
	if (!*src)
		return -1;
	
	return 1;
}



/** Copy NULL terminated string.
 *
 * Copy at most 'len' characters from string 'src' to 'dest'.
 * If 'src' is shorter than 'len', '\0' is inserted behind the
 * last copied character.
 *
 * @param src  Source string.
 * @param dest Destination buffer.
 * @param len  Size of destination buffer.
 *
 */
void strncpy(char *dest, const char *src, size_t len)
{
	unsigned int i;
	
	for (i = 0; i < len; i++) {
		if (!(dest[i] = src[i]))
			return;
	}
	
	dest[i - 1] = '\0';
}

/** Find first occurence of character in string.
 *
 * @param s String to search.
 * @param i Character to look for.
 *
 * @return Pointer to character in @a s or NULL if not found.
 */
extern char *strchr(const char *s, int i)
{
	while (*s != '\0') {
		if (*s == i)
			return (char *) s;
		++s;
	}
	
	return NULL;
}

/** @}
 */
