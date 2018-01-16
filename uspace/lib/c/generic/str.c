/*
 * Copyright (c) 2005 Martin Decky
 * Copyright (c) 2008 Jiri Svoboda
 * Copyright (c) 2011 Martin Sucha
 * Copyright (c) 2011 Oleg Romanenko
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

/** @addtogroup libc
 * @{
 */
/** @file
 */

#include <str.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <align.h>
#include <mem.h>
#include <limits.h>

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

/** Decode a single character from a string to the left.
 *
 * Decode a single character from a string of size @a size. Decoding starts
 * at @a offset and this offset is moved to the beginning of the previous
 * character. In case of decoding error, offset generally decreases at least
 * by one. However, offset is never moved before 0.
 *
 * @param str    String (not necessarily NULL-terminated).
 * @param offset Byte offset in string where to start decoding.
 * @param size   Size of the string (in bytes).
 *
 * @return Value of decoded character, U_SPECIAL on decoding error or
 *         NULL if attempt to decode beyond @a start of str.
 *
 */
wchar_t str_decode_reverse(const char *str, size_t *offset, size_t size)
{
	if (*offset == 0)
		return 0;
	
	size_t processed = 0;
	/* Continue while continuation bytes found */
	while (*offset > 0 && processed < 4) {
		uint8_t b = (uint8_t) str[--(*offset)];
		
		if (processed == 0 && (b & 0x80) == 0) {
			/* 0xxxxxxx (Plain ASCII) */
			return b & 0x7f;
		}
		else if ((b & 0xe0) == 0xc0 || (b & 0xf0) == 0xe0 ||
		    (b & 0xf8) == 0xf0) {
			/* Start byte */
			size_t start_offset = *offset;
			return str_decode(str, &start_offset, size);
		}
		else if ((b & 0xc0) != 0x80) {
			/* Not a continuation byte */
			return U_SPECIAL;
		}
		processed++;
	}
	/* Too many continuation bytes */
	return U_SPECIAL;
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
errno_t chr_encode(const wchar_t ch, char *str, size_t *offset, size_t size)
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

/** Get size of wide string.
 *
 * Get the number of bytes which are used by the wide string @a str (excluding the
 * NULL-terminator).
 *
 * @param str Wide string to consider.
 *
 * @return Number of bytes used by the wide string
 *
 */
size_t wstr_size(const wchar_t *str)
{
	return (wstr_length(str) * sizeof(wchar_t));
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

/** Get size of string with size limit.
 *
 * Get the number of bytes which are used by the string @a str
 * (excluding the NULL-terminator), but no more than @max_size bytes.
 *
 * @param str      String to consider.
 * @param max_size Maximum number of bytes to measure.
 *
 * @return Number of bytes used by the string
 *
 */
size_t str_nsize(const char *str, size_t max_size)
{
	size_t size = 0;
	
	while ((*str++ != 0) && (size < max_size))
		size++;
	
	return size;
}

/** Get size of wide string with size limit.
 *
 * Get the number of bytes which are used by the wide string @a str
 * (excluding the NULL-terminator), but no more than @max_size bytes.
 *
 * @param str      Wide string to consider.
 * @param max_size Maximum number of bytes to measure.
 *
 * @return Number of bytes used by the wide string
 *
 */
size_t wstr_nsize(const wchar_t *str, size_t max_size)
{
	return (wstr_nlength(str, max_size) * sizeof(wchar_t));
}

/** Get size of wide string with length limit.
 *
 * Get the number of bytes which are used by up to @a max_len first
 * wide characters in the wide string @a str. If @a max_len is greater than
 * the length of @a str, the entire wide string is measured (excluding the
 * NULL-terminator).
 *
 * @param str     Wide string to consider.
 * @param max_len Maximum number of wide characters to measure.
 *
 * @return Number of bytes used by the wide characters.
 *
 */
size_t wstr_lsize(const wchar_t *str, size_t max_len)
{
	return (wstr_nlength(str, max_len * sizeof(wchar_t)) * sizeof(wchar_t));
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

/** Get number of characters in a wide string.
 *
 * @param str NULL-terminated wide string.
 *
 * @return Number of characters in @a str.
 *
 */
size_t wstr_length(const wchar_t *wstr)
{
	size_t len = 0;
	
	while (*wstr++ != 0)
		len++;
	
	return len;
}

/** Get number of characters in a string with size limit.
 *
 * @param str  NULL-terminated string.
 * @param size Maximum number of bytes to consider.
 *
 * @return Number of characters in string.
 *
 */
size_t str_nlength(const char *str, size_t size)
{
	size_t len = 0;
	size_t offset = 0;
	
	while (str_decode(str, &offset, size) != 0)
		len++;
	
	return len;
}

/** Get number of characters in a string with size limit.
 *
 * @param str  NULL-terminated string.
 * @param size Maximum number of bytes to consider.
 *
 * @return Number of characters in string.
 *
 */
size_t wstr_nlength(const wchar_t *str, size_t size)
{
	size_t len = 0;
	size_t limit = ALIGN_DOWN(size, sizeof(wchar_t));
	size_t offset = 0;
	
	while ((offset < limit) && (*str++ != 0)) {
		len++;
		offset += sizeof(wchar_t);
	}
	
	return len;
}

/** Get character display width on a character cell display.
 *
 * @param ch	Character
 * @return	Width of character in cells.
 */
size_t chr_width(wchar_t ch)
{
	return 1;
}

/** Get string display width on a character cell display.
 *
 * @param str	String
 * @return	Width of string in cells.
 */
size_t str_width(const char *str)
{
	size_t width = 0;
	size_t offset = 0;
	wchar_t ch;
	
	while ((ch = str_decode(str, &offset, STR_NO_LIMIT)) != 0)
		width += chr_width(ch);
	
	return width;
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

		if (c1 == 0 || c2 == 0)
			break;
	}

	return 0;
}

/** Compare two NULL terminated strings with length limit.
 *
 * Do a char-by-char comparison of two NULL-terminated strings.
 * The strings are considered equal iff
 * min(str_length(s1), max_len) == min(str_length(s2), max_len)
 * and both strings consist of the same sequence of characters,
 * up to max_len characters.
 *
 * A string S1 is less than another string S2 if it has a character with
 * lower value at the first character position where the strings differ.
 * If the strings differ in length, the shorter one is treated as if
 * padded by characters with a value of zero. Only the first max_len
 * characters are considered.
 *
 * @param s1      First string to compare.
 * @param s2      Second string to compare.
 * @param max_len Maximum number of characters to consider.
 *
 * @return 0 if the strings are equal, -1 if the first is less than the second,
 *         1 if the second is less than the first.
 *
 */
int str_lcmp(const char *s1, const char *s2, size_t max_len)
{
	wchar_t c1 = 0;
	wchar_t c2 = 0;

	size_t off1 = 0;
	size_t off2 = 0;

	size_t len = 0;

	while (true) {
		if (len >= max_len)
			break;

		c1 = str_decode(s1, &off1, STR_NO_LIMIT);
		c2 = str_decode(s2, &off2, STR_NO_LIMIT);

		if (c1 < c2)
			return -1;

		if (c1 > c2)
			return 1;

		if (c1 == 0 || c2 == 0)
			break;

		++len;
	}

	return 0;

}

/** Compare two NULL terminated strings in case-insensitive manner.
 *
 * Do a char-by-char comparison of two NULL-terminated strings.
 * The strings are considered equal iff their length is equal
 * and both strings consist of the same sequence of characters
 * when converted to lower case.
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
int str_casecmp(const char *s1, const char *s2)
{
	wchar_t c1 = 0;
	wchar_t c2 = 0;

	size_t off1 = 0;
	size_t off2 = 0;

	while (true) {
		c1 = tolower(str_decode(s1, &off1, STR_NO_LIMIT));
		c2 = tolower(str_decode(s2, &off2, STR_NO_LIMIT));

		if (c1 < c2)
			return -1;

		if (c1 > c2)
			return 1;

		if (c1 == 0 || c2 == 0)
			break;
	}

	return 0;
}

/** Compare two NULL terminated strings with length limit in case-insensitive
 * manner.
 *
 * Do a char-by-char comparison of two NULL-terminated strings.
 * The strings are considered equal iff
 * min(str_length(s1), max_len) == min(str_length(s2), max_len)
 * and both strings consist of the same sequence of characters,
 * up to max_len characters.
 *
 * A string S1 is less than another string S2 if it has a character with
 * lower value at the first character position where the strings differ.
 * If the strings differ in length, the shorter one is treated as if
 * padded by characters with a value of zero. Only the first max_len
 * characters are considered.
 *
 * @param s1      First string to compare.
 * @param s2      Second string to compare.
 * @param max_len Maximum number of characters to consider.
 *
 * @return 0 if the strings are equal, -1 if the first is less than the second,
 *         1 if the second is less than the first.
 *
 */
int str_lcasecmp(const char *s1, const char *s2, size_t max_len)
{
	wchar_t c1 = 0;
	wchar_t c2 = 0;
	
	size_t off1 = 0;
	size_t off2 = 0;
	
	size_t len = 0;

	while (true) {
		if (len >= max_len)
			break;

		c1 = tolower(str_decode(s1, &off1, STR_NO_LIMIT));
		c2 = tolower(str_decode(s2, &off2, STR_NO_LIMIT));

		if (c1 < c2)
			return -1;

		if (c1 > c2)
			return 1;

		if (c1 == 0 || c2 == 0)
			break;

		++len;	
	}

	return 0;

}

/** Test whether p is a prefix of s.
 *
 * Do a char-by-char comparison of two NULL-terminated strings
 * and determine if p is a prefix of s.
 *
 * @param s The string in which to look
 * @param p The string to check if it is a prefix of s
 *
 * @return true iff p is prefix of s else false
 *
 */
bool str_test_prefix(const char *s, const char *p)
{
	wchar_t c1 = 0;
	wchar_t c2 = 0;
	
	size_t off1 = 0;
	size_t off2 = 0;

	while (true) {
		c1 = str_decode(s, &off1, STR_NO_LIMIT);
		c2 = str_decode(p, &off2, STR_NO_LIMIT);
		
		if (c2 == 0)
			return true;

		if (c1 != c2)
			return false;
		
		if (c1 == 0)
			break;
	}

	return false;
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
	/* There must be space for a null terminator in the buffer. */
	assert(size > 0);
	
	size_t src_off = 0;
	size_t dest_off = 0;
	
	wchar_t ch;
	while ((ch = str_decode(src, &src_off, STR_NO_LIMIT)) != 0) {
		if (chr_encode(ch, dest, &dest_off, size - 1) != EOK)
			break;
	}
	
	dest[dest_off] = '\0';
}

/** Copy size-limited substring.
 *
 * Copy prefix of string @a src of max. size @a size to destination buffer
 * @a dest. No more than @a size bytes are written. The output string will
 * always be well-formed, i.e. null-terminated and containing only complete
 * characters.
 *
 * No more than @a n bytes are read from the input string, so it does not
 * have to be null-terminated.
 *
 * @param dest  Destination buffer.
 * @param count Size of the destination buffer (must be > 0).
 * @param src   Source string.
 * @param n     Maximum number of bytes to read from @a src.
 *
 */
void str_ncpy(char *dest, size_t size, const char *src, size_t n)
{
	/* There must be space for a null terminator in the buffer. */
	assert(size > 0);
	
	size_t src_off = 0;
	size_t dest_off = 0;
	
	wchar_t ch;
	while ((ch = str_decode(src, &src_off, n)) != 0) {
		if (chr_encode(ch, dest, &dest_off, size - 1) != EOK)
			break;
	}
	
	dest[dest_off] = '\0';
}

/** Append one string to another.
 *
 * Append source string @a src to string in destination buffer @a dest.
 * Size of the destination buffer is @a dest. If the size of the output buffer
 * is at least one byte, the output string will always be well-formed, i.e.
 * null-terminated and containing only complete characters.
 *
 * @param dest   Destination buffer.
 * @param count Size of the destination buffer.
 * @param src   Source string.
 */
void str_append(char *dest, size_t size, const char *src)
{
	size_t dstr_size;

	dstr_size = str_size(dest);
	if (dstr_size >= size)
		return;
	
	str_cpy(dest + dstr_size, size - dstr_size, src);
}

/** Convert space-padded ASCII to string.
 *
 * Common legacy text encoding in hardware is 7-bit ASCII fitted into
 * a fixed-width byte buffer (bit 7 always zero), right-padded with spaces
 * (ASCII 0x20). Convert space-padded ascii to string representation.
 *
 * If the text does not fit into the destination buffer, the function converts
 * as many characters as possible and returns EOVERFLOW.
 *
 * If the text contains non-ASCII bytes (with bit 7 set), the whole string is
 * converted anyway and invalid characters are replaced with question marks
 * (U_SPECIAL) and the function returns EIO.
 *
 * Regardless of return value upon return @a dest will always be well-formed.
 *
 * @param dest		Destination buffer
 * @param size		Size of destination buffer
 * @param src		Space-padded ASCII.
 * @param n		Size of the source buffer in bytes.
 *
 * @return		EOK on success, EOVERFLOW if the text does not fit
 *			destination buffer, EIO if the text contains
 *			non-ASCII bytes.
 */
errno_t spascii_to_str(char *dest, size_t size, const uint8_t *src, size_t n)
{
	size_t sidx;
	size_t didx;
	size_t dlast;
	uint8_t byte;
	errno_t rc;
	errno_t result;

	/* There must be space for a null terminator in the buffer. */
	assert(size > 0);
	result = EOK;

	didx = 0;
	dlast = 0;
	for (sidx = 0; sidx < n; ++sidx) {
		byte = src[sidx];
		if (!ascii_check(byte)) {
			byte = U_SPECIAL;
			result = EIO;
		}

		rc = chr_encode(byte, dest, &didx, size - 1);
		if (rc != EOK) {
			assert(rc == EOVERFLOW);
			dest[didx] = '\0';
			return rc;
		}

		/* Remember dest index after last non-empty character */
		if (byte != 0x20)
			dlast = didx;
	}

	/* Terminate string after last non-empty character */
	dest[dlast] = '\0';
	return result;
}

/** Convert wide string to string.
 *
 * Convert wide string @a src to string. The output is written to the buffer
 * specified by @a dest and @a size. @a size must be non-zero and the string
 * written will always be well-formed.
 *
 * @param dest	Destination buffer.
 * @param size	Size of the destination buffer.
 * @param src	Source wide string.
 */
void wstr_to_str(char *dest, size_t size, const wchar_t *src)
{
	wchar_t ch;
	size_t src_idx;
	size_t dest_off;

	/* There must be space for a null terminator in the buffer. */
	assert(size > 0);
	
	src_idx = 0;
	dest_off = 0;

	while ((ch = src[src_idx++]) != 0) {
		if (chr_encode(ch, dest, &dest_off, size - 1) != EOK)
			break;
	}

	dest[dest_off] = '\0';
}

/** Convert UTF16 string to string.
 *
 * Convert utf16 string @a src to string. The output is written to the buffer
 * specified by @a dest and @a size. @a size must be non-zero and the string
 * written will always be well-formed. Surrogate pairs also supported.
 *
 * @param dest	Destination buffer.
 * @param size	Size of the destination buffer.
 * @param src	Source utf16 string.
 *
 * @return EOK, if success, an error code otherwise.
 */
errno_t utf16_to_str(char *dest, size_t size, const uint16_t *src)
{
	size_t idx = 0, dest_off = 0;
	wchar_t ch;
	errno_t rc = EOK;

	/* There must be space for a null terminator in the buffer. */
	assert(size > 0);

	while (src[idx]) {
		if ((src[idx] & 0xfc00) == 0xd800) {
			if (src[idx + 1] && (src[idx + 1] & 0xfc00) == 0xdc00) {
				ch = 0x10000;
				ch += (src[idx] & 0x03FF) << 10;
				ch += (src[idx + 1] & 0x03FF);
				idx += 2;
			}
			else
				break;
		} else {
			ch = src[idx];
			idx++;
		}
		rc = chr_encode(ch, dest, &dest_off, size - 1);
		if (rc != EOK)
			break;
	}
	dest[dest_off] = '\0';
	return rc;
}

/** Convert string to UTF16 string.
 *
 * Convert string @a src to utf16 string. The output is written to the buffer
 * specified by @a dest and @a dlen. @a dlen must be non-zero and the string
 * written will always be well-formed. Surrogate pairs also supported.
 *
 * @param dest	Destination buffer.
 * @param dlen	Number of utf16 characters that fit in the destination buffer.
 * @param src	Source string.
 *
 * @return EOK, if success, an error code otherwise.
 */
errno_t str_to_utf16(uint16_t *dest, size_t dlen, const char *src)
{
	errno_t rc = EOK;
	size_t offset = 0;
	size_t idx = 0;
	wchar_t c;

	assert(dlen > 0);
	
	while ((c = str_decode(src, &offset, STR_NO_LIMIT)) != 0) {
		if (c > 0x10000) {
			if (idx + 2 >= dlen - 1) {
				rc = EOVERFLOW;
				break;
			}
			c = (c - 0x10000);
			dest[idx] = 0xD800 | (c >> 10);
			dest[idx + 1] = 0xDC00 | (c & 0x3FF);
			idx++;
		} else {
			 dest[idx] = c;
		}

		idx++;
		if (idx >= dlen - 1) {
			rc = EOVERFLOW;
			break;
		}
	}

	dest[idx] = '\0';
	return rc;
}

/** Get size of UTF-16 string.
 *
 * Get the number of words which are used by the UTF-16 string @a ustr
 * (excluding the NULL-terminator).
 *
 * @param ustr UTF-16 string to consider.
 *
 * @return Number of words used by the UTF-16 string
 *
 */
size_t utf16_wsize(const uint16_t *ustr)
{
	size_t wsize = 0;

	while (*ustr++ != 0)
		wsize++;

	return wsize;
}


/** Convert wide string to new string.
 *
 * Convert wide string @a src to string. Space for the new string is allocated
 * on the heap.
 *
 * @param src	Source wide string.
 * @return	New string.
 */
char *wstr_to_astr(const wchar_t *src)
{
	char dbuf[STR_BOUNDS(1)];
	char *str;
	wchar_t ch;

	size_t src_idx;
	size_t dest_off;
	size_t dest_size;

	/* Compute size of encoded string. */

	src_idx = 0;
	dest_size = 0;

	while ((ch = src[src_idx++]) != 0) {
		dest_off = 0;
		if (chr_encode(ch, dbuf, &dest_off, STR_BOUNDS(1)) != EOK)
			break;
		dest_size += dest_off;
	}

	str = malloc(dest_size + 1);
	if (str == NULL)
		return NULL;

	/* Encode string. */

	src_idx = 0;
	dest_off = 0;

	while ((ch = src[src_idx++]) != 0) {
		if (chr_encode(ch, str, &dest_off, dest_size) != EOK)
			break;
	}

	str[dest_size] = '\0';
	return str;
}


/** Convert string to wide string.
 *
 * Convert string @a src to wide string. The output is written to the
 * buffer specified by @a dest and @a dlen. @a dlen must be non-zero
 * and the wide string written will always be null-terminated.
 *
 * @param dest	Destination buffer.
 * @param dlen	Length of destination buffer (number of wchars).
 * @param src	Source string.
 */
void str_to_wstr(wchar_t *dest, size_t dlen, const char *src)
{
	size_t offset;
	size_t di;
	wchar_t c;

	assert(dlen > 0);

	offset = 0;
	di = 0;

	do {
		if (di >= dlen - 1)
			break;

		c = str_decode(src, &offset, STR_NO_LIMIT);
		dest[di++] = c;
	} while (c != '\0');

	dest[dlen - 1] = '\0';
}

/** Convert string to wide string.
 *
 * Convert string @a src to wide string. A new wide NULL-terminated
 * string will be allocated on the heap.
 *
 * @param src	Source string.
 */
wchar_t *str_to_awstr(const char *str)
{
	size_t len = str_length(str);
	
	wchar_t *wstr = calloc(len+1, sizeof(wchar_t));
	if (wstr == NULL)
		return NULL;
	
	str_to_wstr(wstr, len + 1, str);
	return wstr;
}

/** Find first occurence of character in string.
 *
 * @param str String to search.
 * @param ch  Character to look for.
 *
 * @return Pointer to character in @a str or NULL if not found.
 */
char *str_chr(const char *str, wchar_t ch)
{
	wchar_t acc;
	size_t off = 0;
	size_t last = 0;
	
	while ((acc = str_decode(str, &off, STR_NO_LIMIT)) != 0) {
		if (acc == ch)
			return (char *) (str + last);
		last = off;
	}
	
	return NULL;
}

/** Removes specified trailing characters from a string.
 *
 * @param str String to remove from.
 * @param ch  Character to remove.
 */
void str_rtrim(char *str, wchar_t ch)
{
	size_t off = 0;
	size_t pos = 0;
	wchar_t c;
	bool update_last_chunk = true;
	char *last_chunk = NULL;

	while ((c = str_decode(str, &off, STR_NO_LIMIT))) {
		if (c != ch) {
			update_last_chunk = true;
			last_chunk = NULL;
		} else if (update_last_chunk) {
			update_last_chunk = false;
			last_chunk = (str + pos);
		}
		pos = off;
	}

	if (last_chunk)
		*last_chunk = '\0';
}

/** Removes specified leading characters from a string.
 *
 * @param str String to remove from.
 * @param ch  Character to remove.
 */
void str_ltrim(char *str, wchar_t ch)
{
	wchar_t acc;
	size_t off = 0;
	size_t pos = 0;
	size_t str_sz = str_size(str);

	while ((acc = str_decode(str, &off, STR_NO_LIMIT)) != 0) {
		if (acc != ch)
			break;
		else
			pos = off;
	}

	if (pos > 0) {
		memmove(str, &str[pos], str_sz - pos);
		pos = str_sz - pos;
		str[pos] = '\0';
	}
}

/** Find last occurence of character in string.
 *
 * @param str String to search.
 * @param ch  Character to look for.
 *
 * @return Pointer to character in @a str or NULL if not found.
 */
char *str_rchr(const char *str, wchar_t ch)
{
	wchar_t acc;
	size_t off = 0;
	size_t last = 0;
	const char *res = NULL;
	
	while ((acc = str_decode(str, &off, STR_NO_LIMIT)) != 0) {
		if (acc == ch)
			res = (str + last);
		last = off;
	}
	
	return (char *) res;
}

/** Insert a wide character into a wide string.
 *
 * Insert a wide character into a wide string at position
 * @a pos. The characters after the position are shifted.
 *
 * @param str     String to insert to.
 * @param ch      Character to insert to.
 * @param pos     Character index where to insert.
 @ @param max_pos Characters in the buffer.
 *
 * @return True if the insertion was sucessful, false if the position
 *         is out of bounds.
 *
 */
bool wstr_linsert(wchar_t *str, wchar_t ch, size_t pos, size_t max_pos)
{
	size_t len = wstr_length(str);
	
	if ((pos > len) || (pos + 1 > max_pos))
		return false;
	
	size_t i;
	for (i = len; i + 1 > pos; i--)
		str[i + 1] = str[i];
	
	str[pos] = ch;
	
	return true;
}

/** Remove a wide character from a wide string.
 *
 * Remove a wide character from a wide string at position
 * @a pos. The characters after the position are shifted.
 *
 * @param str String to remove from.
 * @param pos Character index to remove.
 *
 * @return True if the removal was sucessful, false if the position
 *         is out of bounds.
 *
 */
bool wstr_remove(wchar_t *str, size_t pos)
{
	size_t len = wstr_length(str);
	
	if (pos >= len)
		return false;
	
	size_t i;
	for (i = pos + 1; i <= len; i++)
		str[i - 1] = str[i];
	
	return true;
}


/** Duplicate string.
 *
 * Allocate a new string and copy characters from the source
 * string into it. The duplicate string is allocated via sleeping
 * malloc(), thus this function can sleep in no memory conditions.
 *
 * The allocation cannot fail and the return value is always
 * a valid pointer. The duplicate string is always a well-formed
 * null-terminated UTF-8 string, but it can differ from the source
 * string on the byte level.
 *
 * @param src Source string.
 *
 * @return Duplicate string.
 *
 */
char *str_dup(const char *src)
{
	size_t size = str_size(src) + 1;
	char *dest = (char *) malloc(size);
	if (dest == NULL)
		return (char *) NULL;
	
	str_cpy(dest, size, src);
	return dest;
}

/** Duplicate string with size limit.
 *
 * Allocate a new string and copy up to @max_size bytes from the source
 * string into it. The duplicate string is allocated via sleeping
 * malloc(), thus this function can sleep in no memory conditions.
 * No more than @max_size + 1 bytes is allocated, but if the size
 * occupied by the source string is smaller than @max_size + 1,
 * less is allocated.
 *
 * The allocation cannot fail and the return value is always
 * a valid pointer. The duplicate string is always a well-formed
 * null-terminated UTF-8 string, but it can differ from the source
 * string on the byte level.
 *
 * @param src Source string.
 * @param n   Maximum number of bytes to duplicate.
 *
 * @return Duplicate string.
 *
 */
char *str_ndup(const char *src, size_t n)
{
	size_t size = str_size(src);
	if (size > n)
		size = n;
	
	char *dest = (char *) malloc(size + 1);
	if (dest == NULL)
		return (char *) NULL;
	
	str_ncpy(dest, size + 1, src, size);
	return dest;
}

/** Split string by delimiters.
 *
 * @param s             String to be tokenized. May not be NULL.
 * @param delim		String with the delimiters.
 * @param next		Variable which will receive the pointer to the
 *                      continuation of the string following the first
 *                      occurrence of any of the delimiter characters.
 *                      May be NULL.
 * @return              Pointer to the prefix of @a s before the first
 *                      delimiter character. NULL if no such prefix
 *                      exists.
 */
char *str_tok(char *s, const char *delim, char **next)
{
	char *start, *end;

	if (!s)
		return NULL;
	
	size_t len = str_size(s);
	size_t cur;
	size_t tmp;
	wchar_t ch;

	/* Skip over leading delimiters. */
	for (tmp = cur = 0;
	    (ch = str_decode(s, &tmp, len)) && str_chr(delim, ch); /**/)
		cur = tmp;
	start = &s[cur];

	/* Skip over token characters. */
	for (tmp = cur;
	    (ch = str_decode(s, &tmp, len)) && !str_chr(delim, ch); /**/)
		cur = tmp;
	end = &s[cur];
	if (next)
		*next = (ch ? &s[tmp] : &s[cur]);

	if (start == end)
		return NULL;	/* No more tokens. */

	/* Overwrite delimiter with NULL terminator. */
	*end = '\0';
	return start;
}

/** Convert string to uint64_t (internal variant).
 *
 * @param nptr   Pointer to string.
 * @param endptr Pointer to the first invalid character is stored here.
 * @param base   Zero or number between 2 and 36 inclusive.
 * @param neg    Indication of unary minus is stored here.
 * @apram result Result of the conversion.
 *
 * @return EOK if conversion was successful.
 *
 */
static errno_t str_uint(const char *nptr, char **endptr, unsigned int base,
    bool *neg, uint64_t *result)
{
	assert(endptr != NULL);
	assert(neg != NULL);
	assert(result != NULL);
	
	*neg = false;
	const char *str = nptr;
	
	/* Ignore leading whitespace */
	while (isspace(*str))
		str++;
	
	if (*str == '-') {
		*neg = true;
		str++;
	} else if (*str == '+')
		str++;
	
	if (base == 0) {
		/* Decode base if not specified */
		base = 10;
		
		if (*str == '0') {
			base = 8;
			str++;
			
			switch (*str) {
			case 'b':
			case 'B':
				base = 2;
				str++;
				break;
			case 'o':
			case 'O':
				base = 8;
				str++;
				break;
			case 'd':
			case 'D':
			case 't':
			case 'T':
				base = 10;
				str++;
				break;
			case 'x':
			case 'X':
				base = 16;
				str++;
				break;
			default:
				str--;
			}
		}
	} else {
		/* Check base range */
		if ((base < 2) || (base > 36)) {
			*endptr = (char *) str;
			return EINVAL;
		}
	}
	
	*result = 0;
	const char *startstr = str;
	
	while (*str != 0) {
		unsigned int digit;
		
		if ((*str >= 'a') && (*str <= 'z'))
			digit = *str - 'a' + 10;
		else if ((*str >= 'A') && (*str <= 'Z'))
			digit = *str - 'A' + 10;
		else if ((*str >= '0') && (*str <= '9'))
			digit = *str - '0';
		else
			break;
		
		if (digit >= base)
			break;
		
		uint64_t prev = *result;
		*result = (*result) * base + digit;
		
		if (*result < prev) {
			/* Overflow */
			*endptr = (char *) str;
			return EOVERFLOW;
		}
		
		str++;
	}
	
	if (str == startstr) {
		/*
		 * No digits were decoded => first invalid character is
		 * the first character of the string.
		 */
		str = nptr;
	}
	
	*endptr = (char *) str;
	
	if (str == nptr)
		return EINVAL;
	
	return EOK;
}

/** Convert string to uint8_t.
 *
 * @param nptr   Pointer to string.
 * @param endptr If not NULL, pointer to the first invalid character
 *               is stored here.
 * @param base   Zero or number between 2 and 36 inclusive.
 * @param strict Do not allow any trailing characters.
 * @param result Result of the conversion.
 *
 * @return EOK if conversion was successful.
 *
 */
errno_t str_uint8_t(const char *nptr, const char **endptr, unsigned int base,
    bool strict, uint8_t *result)
{
	assert(result != NULL);
	
	bool neg;
	char *lendptr;
	uint64_t res;
	errno_t ret = str_uint(nptr, &lendptr, base, &neg, &res);
	
	if (endptr != NULL)
		*endptr = (char *) lendptr;
	
	if (ret != EOK)
		return ret;
	
	/* Do not allow negative values */
	if (neg)
		return EINVAL;
	
	/* Check whether we are at the end of
	   the string in strict mode */
	if ((strict) && (*lendptr != 0))
		return EINVAL;
	
	/* Check for overflow */
	uint8_t _res = (uint8_t) res;
	if (_res != res)
		return EOVERFLOW;
	
	*result = _res;
	
	return EOK;
}

/** Convert string to uint16_t.
 *
 * @param nptr   Pointer to string.
 * @param endptr If not NULL, pointer to the first invalid character
 *               is stored here.
 * @param base   Zero or number between 2 and 36 inclusive.
 * @param strict Do not allow any trailing characters.
 * @param result Result of the conversion.
 *
 * @return EOK if conversion was successful.
 *
 */
errno_t str_uint16_t(const char *nptr, const char **endptr, unsigned int base,
    bool strict, uint16_t *result)
{
	assert(result != NULL);
	
	bool neg;
	char *lendptr;
	uint64_t res;
	errno_t ret = str_uint(nptr, &lendptr, base, &neg, &res);
	
	if (endptr != NULL)
		*endptr = (char *) lendptr;
	
	if (ret != EOK)
		return ret;
	
	/* Do not allow negative values */
	if (neg)
		return EINVAL;
	
	/* Check whether we are at the end of
	   the string in strict mode */
	if ((strict) && (*lendptr != 0))
		return EINVAL;
	
	/* Check for overflow */
	uint16_t _res = (uint16_t) res;
	if (_res != res)
		return EOVERFLOW;
	
	*result = _res;
	
	return EOK;
}

/** Convert string to uint32_t.
 *
 * @param nptr   Pointer to string.
 * @param endptr If not NULL, pointer to the first invalid character
 *               is stored here.
 * @param base   Zero or number between 2 and 36 inclusive.
 * @param strict Do not allow any trailing characters.
 * @param result Result of the conversion.
 *
 * @return EOK if conversion was successful.
 *
 */
errno_t str_uint32_t(const char *nptr, const char **endptr, unsigned int base,
    bool strict, uint32_t *result)
{
	assert(result != NULL);
	
	bool neg;
	char *lendptr;
	uint64_t res;
	errno_t ret = str_uint(nptr, &lendptr, base, &neg, &res);
	
	if (endptr != NULL)
		*endptr = (char *) lendptr;
	
	if (ret != EOK)
		return ret;
	
	/* Do not allow negative values */
	if (neg)
		return EINVAL;
	
	/* Check whether we are at the end of
	   the string in strict mode */
	if ((strict) && (*lendptr != 0))
		return EINVAL;
	
	/* Check for overflow */
	uint32_t _res = (uint32_t) res;
	if (_res != res)
		return EOVERFLOW;
	
	*result = _res;
	
	return EOK;
}

/** Convert string to uint64_t.
 *
 * @param nptr   Pointer to string.
 * @param endptr If not NULL, pointer to the first invalid character
 *               is stored here.
 * @param base   Zero or number between 2 and 36 inclusive.
 * @param strict Do not allow any trailing characters.
 * @param result Result of the conversion.
 *
 * @return EOK if conversion was successful.
 *
 */
errno_t str_uint64_t(const char *nptr, const char **endptr, unsigned int base,
    bool strict, uint64_t *result)
{
	assert(result != NULL);
	
	bool neg;
	char *lendptr;
	errno_t ret = str_uint(nptr, &lendptr, base, &neg, result);
	
	if (endptr != NULL)
		*endptr = (char *) lendptr;
	
	if (ret != EOK)
		return ret;
	
	/* Do not allow negative values */
	if (neg)
		return EINVAL;
	
	/* Check whether we are at the end of
	   the string in strict mode */
	if ((strict) && (*lendptr != 0))
		return EINVAL;
	
	return EOK;
}

/** Convert string to size_t.
 *
 * @param nptr   Pointer to string.
 * @param endptr If not NULL, pointer to the first invalid character
 *               is stored here.
 * @param base   Zero or number between 2 and 36 inclusive.
 * @param strict Do not allow any trailing characters.
 * @param result Result of the conversion.
 *
 * @return EOK if conversion was successful.
 *
 */
errno_t str_size_t(const char *nptr, const char **endptr, unsigned int base,
    bool strict, size_t *result)
{
	assert(result != NULL);
	
	bool neg;
	char *lendptr;
	uint64_t res;
	errno_t ret = str_uint(nptr, &lendptr, base, &neg, &res);
	
	if (endptr != NULL)
		*endptr = (char *) lendptr;
	
	if (ret != EOK)
		return ret;
	
	/* Do not allow negative values */
	if (neg)
		return EINVAL;
	
	/* Check whether we are at the end of
	   the string in strict mode */
	if ((strict) && (*lendptr != 0))
		return EINVAL;
	
	/* Check for overflow */
	size_t _res = (size_t) res;
	if (_res != res)
		return EOVERFLOW;
	
	*result = _res;
	
	return EOK;
}

void order_suffix(const uint64_t val, uint64_t *rv, char *suffix)
{
	if (val > UINT64_C(10000000000000000000)) {
		*rv = val / UINT64_C(1000000000000000000);
		*suffix = 'Z';
	} else if (val > UINT64_C(1000000000000000000)) {
		*rv = val / UINT64_C(1000000000000000);
		*suffix = 'E';
	} else if (val > UINT64_C(1000000000000000)) {
		*rv = val / UINT64_C(1000000000000);
		*suffix = 'T';
	} else if (val > UINT64_C(1000000000000)) {
		*rv = val / UINT64_C(1000000000);
		*suffix = 'G';
	} else if (val > UINT64_C(1000000000)) {
		*rv = val / UINT64_C(1000000);
		*suffix = 'M';
	} else if (val > UINT64_C(1000000)) {
		*rv = val / UINT64_C(1000);
		*suffix = 'k';
	} else {
		*rv = val;
		*suffix = ' ';
	}
}

void bin_order_suffix(const uint64_t val, uint64_t *rv, const char **suffix,
    bool fixed)
{
	if (val > UINT64_C(1152921504606846976)) {
		*rv = val / UINT64_C(1125899906842624);
		*suffix = "EiB";
	} else if (val > UINT64_C(1125899906842624)) {
		*rv = val / UINT64_C(1099511627776);
		*suffix = "TiB";
	} else if (val > UINT64_C(1099511627776)) {
		*rv = val / UINT64_C(1073741824);
		*suffix = "GiB";
	} else if (val > UINT64_C(1073741824)) {
		*rv = val / UINT64_C(1048576);
		*suffix = "MiB";
	} else if (val > UINT64_C(1048576)) {
		*rv = val / UINT64_C(1024);
		*suffix = "KiB";
	} else {
		*rv = val;
		if (fixed)
			*suffix = "B  ";
		else
			*suffix = "B";
	}
}

/** @}
 */
