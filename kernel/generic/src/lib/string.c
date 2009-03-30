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

char invalch = '?';

/** Byte mask consisting of bits 0 - (@n - 1) */
#define LO_MASK_8(n) ((uint8_t)((1 << (n)) - 1))

/** Number of data bits in a UTF-8 continuation byte. */
#define CONT_BITS 6

/** Decode a single UTF-8 character from a NULL-terminated string.
 *
 * Decode a single UTF-8 character from a plain char NULL-terminated
 * string. Decoding starts at @index and this index is incremented
 * if the current UTF-8 string is encoded in more than a single byte.
 *
 * @param str   Plain character NULL-terminated string.
 * @param index Index (counted in plain characters) where to start
 *              the decoding.
 * @param limit Maximal allowed value of index.
 *
 * @return Decoded character in UTF-32 or '?' if the encoding is wrong.
 *
 */
wchar_t utf8_decode(const char *str, index_t *index, index_t limit)
{
	uint8_t b0, b;          /* Bytes read from str. */
	wchar_t ch;

	int b0_bits;		/* Data bits in first byte. */
	int cbytes;		/* Number of continuation bytes. */

	if (*index > limit)
		return invalch;

	b0 = (uint8_t) str[*index];

	/* Determine code length. */

	if ((b0 & 0x80) == 0) {
		/* 0xxxxxxx (Plain ASCII) */
		b0_bits = 7;
		cbytes = 0;
	} else if ((b0 & 0xe0) == 0xc0) {
		/* 110xxxxx 10xxxxxx */
		b0_bits = 5;
		cbytes = 1;
	} else if ((b0 & 0xf0) == 0xe0) {
		/* 1110xxxx 10xxxxxx 10xxxxxx */
		b0_bits = 4;
		cbytes = 2;
	} else if ((b0 & 0xf8) == 0xf0) {
		/* 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx */
		b0_bits = 3;
		cbytes = 3;
	} else {
		/* 10xxxxxx -- unexpected continuation byte. */
		return invalch;
	}

	if (*index + cbytes > limit) {
		return invalch;
	}

	ch = b0 & LO_MASK_8(b0_bits);

	/* Decode continuation bytes. */
	while (cbytes > 0) {
		b = (uint8_t) str[*index + 1];
		++(*index);

		/* Must be 10xxxxxx. */
		if ((b & 0xc0) != 0x80) {
			return invalch;
		}

		/* Shift data bits to ch. */
		ch = (ch << CONT_BITS) | (wchar_t) (b & LO_MASK_8(CONT_BITS));
		--cbytes;
	}

	return ch;
}

/** Encode a single UTF-32 character as UTF-8
 *
 * Encode a single UTF-32 character as UTF-8 and store it into
 * the given buffer at @index. Encoding starts at @index and
 * this index is incremented if the UTF-8 character takes
 * more than a single byte.
 *
 * @param ch    Input UTF-32 character.
 * @param str   Output buffer.
 * @param index Index (counted in plain characters) where to start
 *              the encoding
 * @param limit Maximal allowed value of index.
 *
 * @return True if the character was encoded or false if there is not
 *         enought space in the output buffer or the character is invalid
 *         Unicode code point.
 *
 */
bool utf8_encode(const wchar_t ch, char *str, index_t *index, index_t limit)
{
	if (*index > limit)
		return false;
	
	if ((ch >= 0) && (ch <= 127)) {
		/* Plain ASCII (code points 0 .. 127) */
		str[*index] = ch & 0x7f;
		return true;
	}
	
	if ((ch >= 128) && (ch <= 2047)) {
		/* Code points 128 .. 2047 */
		if (*index + 1 > limit)
			return false;
		
		str[*index] = 0xc0 | ((ch >> 6) & 0x1f);
		(*index)++;
		str[*index] = 0x80 | (ch & 0x3f);
		return true;
	}
	
	if ((ch >= 2048) && (ch <= 65535)) {
		/* Code points 2048 .. 65535 */
		if (*index + 2 > limit)
			return false;
		
		str[*index] = 0xe0 | ((ch >> 12) & 0x0f);
		(*index)++;
		str[*index] = 0x80 | ((ch >> 6) & 0x3f);
		(*index)++;
		str[*index] = 0x80 | (ch & 0x3f);
		return true;
	}
	
	if ((ch >= 65536) && (ch <= 1114111)) {
		/* Code points 65536 .. 1114111 */
		if (*index + 3 > limit)
			return false;
		
		str[*index] = 0xf0 | ((ch >> 18) & 0x07);
		(*index)++;
		str[*index] = 0x80 | ((ch >> 12) & 0x3f);
		(*index)++;
		str[*index] = 0x80 | ((ch >> 6) & 0x3f);
		(*index)++;
		str[*index] = 0x80 | (ch & 0x3f);
		return true;
	}
	
	return false;
}

/** Get bytes used by UTF-8 characters.
 *
 * Get the number of bytes (count of plain characters) which
 * are used by a given count of UTF-8 characters in a string.
 * As UTF-8 encoding is multibyte, there is no constant
 * correspondence between number of characters and used bytes.
 *
 * @param str   UTF-8 string to consider.
 * @param count Number of UTF-8 characters to count.
 *
 * @return Number of bytes used by the characters.
 *
 */
size_t utf8_count_bytes(const char *str, count_t count)
{
	size_t size = 0;
	index_t index = 0;
	
	while ((utf8_decode(str, &index, UTF8_NO_LIMIT) != 0) && (size < count)) {
		size++;
		index++;
	}
	
	return index;
}

/** Check whether character is plain ASCII.
 *
 * @return True if character is plain ASCII.
 *
 */
bool ascii_check(const wchar_t ch)
{
	if ((ch >= 0) && (ch <= 127))
		return true;
	
	return false;
}

/** Check whether character is Unicode.
 *
 * @return True if character is valid Unicode code point.
 *
 */
bool unicode_check(const wchar_t ch)
{
	if ((ch >= 0) && (ch <= 1114111))
		return true;
	
	return false;
}

/** Return number of plain characters in a string.
 *
 * @param str NULL-terminated string.
 *
 * @return Number of characters in str.
 *
 */
size_t strlen(const char *str)
{
	size_t size;
	for (size = 0; str[size]; size++);
	
	return size;
}

/** Return number of UTF-8 characters in a string.
 *
 * @param str NULL-terminated UTF-8 string.
 *
 * @return Number of UTF-8 characters in str.
 *
 */
size_t strlen_utf8(const char *str)
{
	size_t size = 0;
	index_t index = 0;
	
	while (utf8_decode(str, &index, UTF8_NO_LIMIT) != 0) {
		size++;
		index++;
	}
	
	return size;
}

/** Return number of UTF-32 characters in a string.
 *
 * @param str NULL-terminated UTF-32 string.
 *
 * @return Number of UTF-32 characters in str.
 *
 */
size_t strlen_utf32(const wchar_t *str)
{
	size_t size;
	for (size = 0; str[size]; size++);
	
	return size;
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
