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

/**
 * @file
 * @brief String functions.
 *
 * Strings and characters use the Universal Character Set (UCS). The standard
 * strings, called just strings are encoded in UTF-8. Wide strings (encoded
 * in UTF-32) are supported to a limited degree. A single character is
 * represented as wchar_t.@n
 *
 * Overview of the terminology:@n
 *
 *  Term                  Meaning
 *  --------------------  ----------------------------------------------------
 *  byte                  8 bits stored in uint8_t (unsigned 8 bit integer)
 *
 *  character             UTF-32 encoded Unicode character, stored in wchar_t
 *                        (signed 32 bit integer), code points 0 .. 1114111
 *                        are valid
 *
 *  ASCII character       7 bit encoded ASCII character, stored in char
 *                        (usually signed 8 bit integer), code points 0 .. 127
 *                        are valid
 *
 *  string                UTF-8 encoded NULL-terminated Unicode string, char *
 *
 *  wide string           UTF-32 encoded NULL-terminated Unicode string,
 *                        wchar_t *
 *
 *  [wide] string size    number of BYTES in a [wide] string (excluding
 *                        the NULL-terminator), size_t
 *
 *  [wide] string length  number of CHARACTERS in a [wide] string (excluding
 *                        the NULL-terminator), size_t
 *
 *  [wide] string width   number of display cells on a monospace display taken
 *                        by a [wide] string, size_t
 *
 *
 * Overview of string metrics:@n
 *
 *  Metric  Abbrev.  Type     Meaning
 *  ------  ------   ------   -------------------------------------------------
 *  size    n        size_t   number of BYTES in a string (excluding the
 *                            NULL-terminator)
 *
 *  length  l        size_t   number of CHARACTERS in a string (excluding the
 *                            null terminator)
 *
 *  width  w         size_t   number of display cells on a monospace display
 *                            taken by a string
 *
 *
 * Function naming prefixes:@n
 *
 *  chr_    operate on characters
 *  ascii_  operate on ASCII characters
 *  str_    operate on strings
 *  wstr_   operate on wide strings
 *
 *  [w]str_[n|l|w]  operate on a prefix limited by size, length
 *                  or width
 *
 *
 * A specific character inside a [wide] string can be referred to by:@n
 *
 *  pointer (char *, wchar_t *)
 *  byte offset (size_t)
 *  character index (size_t)
 *
 */

#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <str.h>

/** Check the condition if wchar_t is signed */
#ifdef __WCHAR_UNSIGNED__
	#define WCHAR_SIGNED_CHECK(cond)  (true)
#else
	#define WCHAR_SIGNED_CHECK(cond)  (cond)
#endif

/** Byte mask consisting of lowest @n bits (out of 8) */
#define LO_MASK_8(n)  ((uint8_t) ((1 << (n)) - 1))

/** Byte mask consisting of lowest @n bits (out of 32) */
#define LO_MASK_32(n)  ((uint32_t) ((1 << (n)) - 1))

/** Byte mask consisting of highest @n bits (out of 8) */
#define HI_MASK_8(n)  (~LO_MASK_8(8 - (n)))

/** Number of data bits in a UTF-8 continuation byte */
#define CONT_BITS  6

/** Decode a single character from a string.
 *
 * Decode a single character from a string of size @a size. Decoding starts
 * at @a offset and this offset is moved to the beginning of the next
 * character. In case of decoding error, offset generally advances at least
 * by one. However, offset is never moved beyond size.
 *
 * @param str    String (not necessarily NULL-terminated).
 * @param offset Byte offset in string where to start decoding.
 * @param size   Size of the string (in bytes).
 *
 * @return Value of decoded character, U_SPECIAL on decoding error or
 *         NULL if attempt to decode beyond @a size.
 *
 */
wchar_t str_decode(const char *str, size_t *offset, size_t size)
{
	if (*offset + 1 > size)
		return 0;

	/* First byte read from string */
	uint8_t b0 = (uint8_t) str[(*offset)++];

	/* Determine code length */

	unsigned int b0_bits;  /* Data bits in first byte */
	unsigned int cbytes;   /* Number of continuation bytes */

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
		/* 10xxxxxx -- unexpected continuation byte */
		return U_SPECIAL;
	}

	if (*offset + cbytes > size)
		return U_SPECIAL;

	wchar_t ch = b0 & LO_MASK_8(b0_bits);

	/* Decode continuation bytes */
	while (cbytes > 0) {
		uint8_t b = (uint8_t) str[(*offset)++];

		/* Must be 10xxxxxx */
		if ((b & 0xc0) != 0x80)
			return U_SPECIAL;

		/* Shift data bits to ch */
		ch = (ch << CONT_BITS) | (wchar_t) (b & LO_MASK_8(CONT_BITS));
		cbytes--;
	}

	return ch;
}

/** Encode a single character to string representation.
 *
 * Encode a single character to string representation (i.e. UTF-8) and store
 * it into a buffer at @a offset. Encoding starts at @a offset and this offset
 * is moved to the position where the next character can be written to.
 *
 * @param ch     Input character.
 * @param str    Output buffer.
 * @param offset Byte offset where to start writing.
 * @param size   Size of the output buffer (in bytes).
 *
 * @return EOK if the character was encoded successfully, EOVERFLOW if there
 *         was not enough space in the output buffer or EINVAL if the character
 *         code was invalid.
 */
int chr_encode(const wchar_t ch, char *str, size_t *offset, size_t size)
{
	if (*offset >= size)
		return EOVERFLOW;

	if (!chr_check(ch))
		return EINVAL;

	/* Unsigned version of ch (bit operations should only be done
	   on unsigned types). */
	uint32_t cc = (uint32_t) ch;

	/* Determine how many continuation bytes are needed */

	unsigned int b0_bits;  /* Data bits in first byte */
	unsigned int cbytes;   /* Number of continuation bytes */

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
		/* Codes longer than 21 bits are not supported */
		return EINVAL;
	}

	/* Check for available space in buffer */
	if (*offset + cbytes >= size)
		return EOVERFLOW;

	/* Encode continuation bytes */
	unsigned int i;
	for (i = cbytes; i > 0; i--) {
		str[*offset + i] = 0x80 | (cc & LO_MASK_32(CONT_BITS));
		cc = cc >> CONT_BITS;
	}

	/* Encode first byte */
	str[*offset] = (cc & LO_MASK_32(b0_bits)) | HI_MASK_8(8 - b0_bits - 1);

	/* Advance offset */
	*offset += cbytes + 1;

	return EOK;
}

/** Get size of string.
 *
 * Get the number of bytes which are used by the string @a str (excluding the
 * NULL-terminator).
 *
 * @param str String to consider.
 *
 * @return Number of bytes used by the string
 *
 */
size_t str_size(const char *str)
{
	size_t size = 0;

	while (*str++ != 0)
		size++;

	return size;
}

/** Get size of string with length limit.
 *
 * Get the number of bytes which are used by up to @a max_len first
 * characters in the string @a str. If @a max_len is greater than
 * the length of @a str, the entire string is measured (excluding the
 * NULL-terminator).
 *
 * @param str     String to consider.
 * @param max_len Maximum number of characters to measure.
 *
 * @return Number of bytes used by the characters.
 *
 */
size_t str_lsize(const char *str, size_t max_len)
{
	size_t len = 0;
	size_t offset = 0;

	while (len < max_len) {
		if (str_decode(str, &offset, STR_NO_LIMIT) == 0)
			break;

		len++;
	}

	return offset;
}

/** Get number of characters in a string.
 *
 * @param str NULL-terminated string.
 *
 * @return Number of characters in string.
 *
 */
size_t str_length(const char *str)
{
	size_t len = 0;
	size_t offset = 0;

	while (str_decode(str, &offset, STR_NO_LIMIT) != 0)
		len++;

	return len;
}

/** Check whether character is plain ASCII.
 *
 * @return True if character is plain ASCII.
 *
 */
bool ascii_check(wchar_t ch)
{
	if (WCHAR_SIGNED_CHECK(ch >= 0) && (ch <= 127))
		return true;

	return false;
}

/** Check whether character is valid
 *
 * @return True if character is a valid Unicode code point.
 *
 */
bool chr_check(wchar_t ch)
{
	if (WCHAR_SIGNED_CHECK(ch >= 0) && (ch <= 1114111))
		return true;

	return false;
}

/** Compare two NULL terminated strings.
 *
 * Do a char-by-char comparison of two NULL-terminated strings.
 * The strings are considered equal iff their length is equal
 * and both strings consist of the same sequence of characters.
 *
 * A string S1 is less than another string S2 if it has a character with
 * lower value at the first character position where the strings differ.
 * If the strings differ in length, the shorter one is treated as if
 * padded by characters with a value of zero.
 *
 * @param s1 First string to compare.
 * @param s2 Second string to compare.
 *
 * @return 0 if the strings are equal, -1 if the first is less than the second,
 *         1 if the second is less than the first.
 *
 */
int str_cmp(const char *s1, const char *s2)
{
	wchar_t c1 = 0;
	wchar_t c2 = 0;

	size_t off1 = 0;
	size_t off2 = 0;

	while (true) {
		c1 = str_decode(s1, &off1, STR_NO_LIMIT);
		c2 = str_decode(s2, &off2, STR_NO_LIMIT);

		if (c1 < c2)
			return -1;

		if (c1 > c2)
			return 1;

		if ((c1 == 0) || (c2 == 0))
			break;
	}

	return 0;
}

/** Copy string.
 *
 * Copy source string @a src to destination buffer @a dest.
 * No more than @a size bytes are written. If the size of the output buffer
 * is at least one byte, the output string will always be well-formed, i.e.
 * null-terminated and containing only complete characters.
 *
 * @param dest  Destination buffer.
 * @param count Size of the destination buffer (must be > 0).
 * @param src   Source string.
 *
 */
void str_cpy(char *dest, size_t size, const char *src)
{
	size_t src_off = 0;
	size_t dest_off = 0;

	wchar_t ch;
	while ((ch = str_decode(src, &src_off, STR_NO_LIMIT)) != 0) {
		if (chr_encode(ch, dest, &dest_off, size - 1) != EOK)
			break;
	}

	dest[dest_off] = '\0';
}

/** @}
 */
