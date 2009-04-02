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
#include <errno.h>
#include <console/kconsole.h>

char invalch = '?';

/** Byte mask consisting of lowest @n bits (out of eight). */
#define LO_MASK_8(n) ((uint8_t)((1 << (n)) - 1))

/** Byte mask consisting of lowest @n bits (out of 32). */
#define LO_MASK_32(n) ((uint32_t)((1 << (n)) - 1))

/** Byte mask consisting of highest @n bits (out of eight). */
#define HI_MASK_8(n) (~LO_MASK_8(8 - (n)))

/** Number of data bits in a UTF-8 continuation byte. */
#define CONT_BITS 6

/** Decode a single character from a substring.
 *
 * Decode a single character from a substring of size @a sz. Decoding starts
 * at @a offset and this offset is moved to the beginning of the next
 * character. In case of decoding error, offset generally advances at least
 * by one. However, offset is never moved beyond (str + sz).
 *
 * @param str   String (not necessarily NULL-terminated).
 * @param index Index (counted in plain characters) where to start
 *              the decoding.
 * @param limit Size of the substring.
 *
 * @return	Value of decoded character or '?' on decoding error.
 */
wchar_t chr_decode(const char *str, size_t *offset, size_t sz)
{
	uint8_t b0, b;          /* Bytes read from str. */
	wchar_t ch;

	int b0_bits;		/* Data bits in first byte. */
	int cbytes;		/* Number of continuation bytes. */

	if (*offset + 1 > sz)
		return invalch;

	b0 = (uint8_t) str[(*offset)++];

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

	if (*offset + cbytes > sz) {
		return invalch;
	}

	ch = b0 & LO_MASK_8(b0_bits);

	/* Decode continuation bytes. */
	while (cbytes > 0) {
		b = (uint8_t) str[(*offset)++];

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

/** Encode a single character to string representation.
 *
 * Encode a single character to string representation (i.e. UTF-8) and store
 * it into a buffer at @a offset. Encoding starts at @a offset and this offset
 * is moved to the position where the next character can be written to.
 *
 * @param ch		Input character.
 * @param str		Output buffer.
 * @param offset	Offset (in bytes) where to start writing.
 * @param sz		Size of the output buffer.
 *
 * @return EOK if the character was encoded successfully, EOVERFLOW if there
 *	   was not enough space in the output buffer or EINVAL if the character
 *	   code was invalid.
 */
int chr_encode(const wchar_t ch, char *str, size_t *offset, size_t sz)
{
	uint32_t cc;		/* Unsigned version of ch. */

	int cbytes;		/* Number of continuation bytes. */
	int b0_bits;		/* Number of data bits in first byte. */
	int i;

	if (*offset >= sz)
		return EOVERFLOW;

	if (ch < 0)
		return EINVAL;

	/* Bit operations should only be done on unsigned numbers. */
	cc = (uint32_t) ch;

	/* Determine how many continuation bytes are needed. */
	if ((cc & ~LO_MASK_32(7)) == 0) {
		b0_bits = 7;
		cbytes = 0;
	} else if ((cc & ~LO_MASK_32(11)) == 0) {
		b0_bits = 5;
		cbytes = 1;
	} else if ((cc & ~LO_MASK_32(16)) == 0) {
		b0_bits = 4;
		cbytes = 2;
	} else if ((cc & ~LO_MASK_32(21)) == 0) {
		b0_bits = 3;
		cbytes = 3;
	} else {
		/* Codes longer than 21 bits are not supported. */
		return EINVAL;
	}

	/* Check for available space in buffer. */
	if (*offset + cbytes >= sz)
		return EOVERFLOW;

	/* Encode continuation bytes. */
	for (i = cbytes; i > 0; --i) {
		str[*offset + i] = 0x80 | (cc & LO_MASK_32(CONT_BITS));
		cc = cc >> CONT_BITS;
	}

	/* Encode first byte. */
	str[*offset] = (cc & LO_MASK_32(b0_bits)) | HI_MASK_8(8 - b0_bits - 1);

	/* Advance offset. */
	*offset += (1 + cbytes);
	
	return EOK;
}

/** Get size of string, with length limit.
 *
 * Get the number of bytes which are used by up to @a max_len first
 * characters in the string @a str. If @a max_len is greater than
 * the length of @a str, the entire string is measured.
 *
 * @param str   String to consider.
 * @param count Maximum number of characters to measure.
 *
 * @return Number of bytes used by the characters.
 */
size_t str_lsize(const char *str, count_t max_len)
{
	count_t len = 0;
	size_t cur = 0;
	size_t prev;
	wchar_t ch;

	while (true) {
		prev = cur;
		if (len >= max_len)
			break;
		ch = chr_decode(str, &cur, UTF8_NO_LIMIT);
		if (ch == '\0') break;

		len++;
	}

	return prev;
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
 */
bool unicode_check(const wchar_t ch)
{
	if ((ch >= 0) && (ch <= 1114111))
		return true;
	
	return false;
}

/** Return number of bytes the string occupies.
 *
 * @param str A string.
 * @return Number of bytes in @a str excluding the null terminator.
 */
size_t str_size(const char *str)
{
	size_t size;

	size = 0;
	while (*str++ != '\0')
		++size;

	return size;
}

/** Return number of characters in a string.
 *
 * @param str NULL-terminated string.
 * @return Number of characters in string.
 */
count_t str_length(const char *str)
{
	count_t len = 0;
	size_t offset = 0;

	while (chr_decode(str, &offset, UTF8_NO_LIMIT) != 0) {
		len++;
	}

	return len;
}

/** Return number of characters in a wide string.
 *
 * @param str NULL-terminated wide string.
 * @return Number of characters in @a str.
 */
count_t wstr_length(const wchar_t *wstr)
{
	count_t len;

	len = 0;
	while (*wstr++ != '\0')
		++len;

	return len;
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
