/*
 * Copyright (c) 2005 Martin Decky
 * Copyright (C) 1998 by Wes Peters <wes@softweyr.com>
 * Copyright (c) 1988, 1993 The Regents of the University of California.
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

#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <limits.h>
#include <align.h>
#include <sys/types.h>
#include <malloc.h>

/* Dummy implementation of mem/ functions */

void *memset(void *s, int c, size_t n)
{
	char *os = s;
	
	while (n--)
		*(os++) = c;
	
	return s;
}

struct along {
	unsigned long n;
} __attribute__ ((packed));

static void *unaligned_memcpy(void *dst, const void *src, size_t n)
{
	int i, j;
	struct along *adst = dst;
	const struct along *asrc = src;

	for (i = 0; i < n / sizeof(unsigned long); i++)
		adst[i].n = asrc[i].n;
		
	for (j = 0; j < n % sizeof(unsigned long); j++)
		((unsigned char *) (((unsigned long *) dst) + i))[j] =
		    ((unsigned char *) (((unsigned long *) src) + i))[j];
		
	return (char *) dst;
}

/** Copy memory block. */
void *memcpy(void *dst, const void *src, size_t n)
{
	size_t i;
	size_t mod, fill;
	size_t word_size;
	size_t n_words;

	const unsigned long *srcw;
	unsigned long *dstw;
	const uint8_t *srcb;
	uint8_t *dstb;

	word_size = sizeof(unsigned long);

	/*
	 * Are source and destination addresses congruent modulo word_size?
	 * If not, use unaligned_memcpy().
	 */

	if (((uintptr_t) dst & (word_size - 1)) !=
	    ((uintptr_t) src & (word_size - 1)))
 		return unaligned_memcpy(dst, src, n);

	/*
	 * mod is the address modulo word size. fill is the length of the
	 * initial buffer segment before the first word boundary.
	 * If the buffer is very short, use unaligned_memcpy(), too.
	 */

	mod = (uintptr_t) dst & (word_size - 1);
	fill = word_size - mod;
	if (fill > n) fill = n;

	/* Copy the initial segment. */

	srcb = src;
	dstb = dst;

	i = fill;
	while (i-- > 0)
		*dstb++ = *srcb++;

	/* Compute remaining length. */

	n -= fill;
	if (n == 0) return dst;

	/* Pointers to aligned segment. */

	dstw = (unsigned long *) dstb;
	srcw = (const unsigned long *) srcb;

	n_words = n / word_size;	/* Number of whole words to copy. */
	n -= n_words * word_size;	/* Remaining bytes at the end. */

	/* "Fast" copy. */
	i = n_words;
	while (i-- > 0)
		*dstw++ = *srcw++;

	/*
	 * Copy the rest.
	 */

	srcb = (const uint8_t *) srcw;
	dstb = (uint8_t *) dstw;

	i = n;
	while (i-- > 0)
		*dstb++ = *srcb++;

	return dst;
}

void *memmove(void *dst, const void *src, size_t n)
{
	int i, j;
	
	if (src > dst)
		return memcpy(dst, src, n);

	for (j = (n % sizeof(unsigned long)) - 1; j >= 0; j--)
		((unsigned char *) ((unsigned long *) dst))[j] =
		    ((unsigned char *) ((unsigned long *) src))[j];

	for (i = n / sizeof(unsigned long) - 1; i >=0 ; i--)
		((unsigned long *) dst)[i] = ((unsigned long *) src)[i];
		
	return (char *) dst;
}

/** Compare two memory areas.
 *
 * @param s1		Pointer to the first area to compare.
 * @param s2		Pointer to the second area to compare.
 * @param len		Size of the first area in bytes. Both areas must have
 * 			the same length.
 * @return		If len is 0, return zero. If the areas match, return
 * 			zero. Otherwise return non-zero.
 */
int bcmp(const char *s1, const char *s2, size_t len)
{
	for (; len && *s1++ == *s2++; len--)
		;
	return len;
}

/** Count the number of characters in the string, not including terminating 0.
 *
 * @param str		String.
 * @return		Number of characters in string.
 */
size_t strlen(const char *str) 
{
	size_t counter = 0;

	while (str[counter] != 0)
		counter++;

	return counter;
}

int strcmp(const char *a, const char *b)
{
	int c = 0;
	
	while (a[c] && b[c] && (!(a[c] - b[c])))
		c++;
	
	return (a[c] - b[c]);
}

int strncmp(const char *a, const char *b, size_t n)
{
	size_t c = 0;

	while (c < n && a[c] && b[c] && (!(a[c] - b[c])))
		c++;
	
	return ( c < n ? a[c] - b[c] : 0);
	
}

int stricmp(const char *a, const char *b)
{
	int c = 0;
	
	while (a[c] && b[c] && (!(tolower(a[c]) - tolower(b[c]))))
		c++;
	
	return (tolower(a[c]) - tolower(b[c]));
}

/** Return pointer to the first occurence of character c in string.
 *
 * @param str		Scanned string.
 * @param c		Searched character (taken as one byte).
 * @return		Pointer to the matched character or NULL if it is not
 * 			found in given string.
 */
char *strchr(const char *str, int c)
{
	while (*str != '\0') {
		if (*str == (char) c)
			return (char *) str;
		str++;
	}

	return NULL;
}

/** Return pointer to the last occurence of character c in string.
 *
 * @param str		Scanned string.
 * @param c		Searched character (taken as one byte).
 * @return		Pointer to the matched character or NULL if it is not
 * 			found in given string.
 */
char *strrchr(const char *str, int c)
{
	char *retval = NULL;

	while (*str != '\0') {
		if (*str == (char) c)
			retval = (char *) str;
		str++;
	}

	return (char *) retval;
}

/** Convert string to a number. 
 * Core of strtol and strtoul functions.
 *
 * @param nptr		Pointer to string.
 * @param endptr	If not NULL, function stores here pointer to the first
 * 			invalid character.
 * @param base		Zero or number between 2 and 36 inclusive.
 * @param sgn		It's set to 1 if minus found.
 * @return		Result of conversion.
 */
static unsigned long
_strtoul(const char *nptr, char **endptr, int base, char *sgn)
{
	unsigned char c;
	unsigned long result = 0;
	unsigned long a, b;
	const char *str = nptr;
	const char *tmpptr;
	
	while (isspace(*str))
		str++;
	
	if (*str == '-') {
		*sgn = 1;
		++str;
	} else if (*str == '+')
		++str;
	
	if (base) {
		if ((base == 1) || (base > 36)) {
			/* FIXME: set errno to EINVAL */
			return 0;
		}
		if ((base == 16) && (*str == '0') && ((str[1] == 'x') ||
		    (str[1] == 'X'))) {
			str += 2;
		}
	} else {
		base = 10;
		
		if (*str == '0') {
			base = 8;
			if ((str[1] == 'X') || (str[1] == 'x'))  {
				base = 16;
				str += 2;
			}
		} 
	}
	
	tmpptr = str;

	while (*str) {
		c = *str;
		c = (c >= 'a' ? c - 'a' + 10 : (c >= 'A' ? c - 'A' + 10 :
		    (c <= '9' ? c - '0' : 0xff)));
		if (c > base) {
			break;
		}
		
		a = (result & 0xff) * base + c;
		b = (result >> 8) * base + (a >> 8);
		
		if (b > (ULONG_MAX >> 8)) {
			/* overflow */
			/* FIXME: errno = ERANGE*/
			return ULONG_MAX;
		}
	
		result = (b << 8) + (a & 0xff);
		++str;
	}
	
	if (str == tmpptr) {
		/*
		 * No number was found => first invalid character is the first
		 * character of the string.
		 */
		/* FIXME: set errno to EINVAL */
		str = nptr;
		result = 0;
	}
	
	if (endptr)
		*endptr = (char *) str;

	if (nptr == str) { 
		/*FIXME: errno = EINVAL*/
		return 0;
	}

	return result;
}

/** Convert initial part of string to long int according to given base.
 * The number may begin with an arbitrary number of whitespaces followed by
 * optional sign (`+' or `-'). If the base is 0 or 16, the prefix `0x' may be
 * inserted and the number will be taken as hexadecimal one. If the base is 0
 * and the number begin with a zero, number will be taken as octal one (as with
 * base 8). Otherwise the base 0 is taken as decimal.
 *
 * @param nptr		Pointer to string.
 * @param endptr	If not NULL, function stores here pointer to the first
 * 			invalid character.
 * @param base		Zero or number between 2 and 36 inclusive.
 * @return		Result of conversion.
 */
long int strtol(const char *nptr, char **endptr, int base)
{
	char sgn = 0;
	unsigned long number = 0;
	
	number = _strtoul(nptr, endptr, base, &sgn);

	if (number > LONG_MAX) {
		if ((sgn) && (number == (unsigned long) (LONG_MAX) + 1)) {
			/* FIXME: set 0 to errno */
			return number;		
		}
		/* FIXME: set ERANGE to errno */
		return (sgn ? LONG_MIN : LONG_MAX);	
	}
	
	return (sgn ? -number : number);
}


/** Convert initial part of string to unsigned long according to given base.
 * The number may begin with an arbitrary number of whitespaces followed by
 * optional sign (`+' or `-'). If the base is 0 or 16, the prefix `0x' may be
 * inserted and the number will be taken as hexadecimal one. If the base is 0
 * and the number begin with a zero, number will be taken as octal one (as with
 * base 8). Otherwise the base 0 is taken as decimal.
 *
 * @param nptr		Pointer to string.
 * @param endptr	If not NULL, function stores here pointer to the first
 * 			invalid character
 * @param base		Zero or number between 2 and 36 inclusive.
 * @return		Result of conversion.
 */
unsigned long strtoul(const char *nptr, char **endptr, int base)
{
	char sgn = 0;
	unsigned long number = 0;
	
	number = _strtoul(nptr, endptr, base, &sgn);

	return (sgn ? -number : number);
}

char *strcpy(char *dest, const char *src)
{
	char *orig = dest;
	
	while ((*(dest++) = *(src++)))
		;
	return orig;
}

char *strncpy(char *dest, const char *src, size_t n)
{
	char *orig = dest;
	
	while ((*(dest++) = *(src++)) && --n)
		;
	return orig;
}

char *strcat(char *dest, const char *src)
{
	char *orig = dest;
	while (*dest++)
		;
	--dest;
	while ((*dest++ = *src++))
		;
	return orig;
}

char * strdup(const char *s1)
{
	size_t len = strlen(s1) + 1;
	void *ret = malloc(len);

	if (ret == NULL)
		return (char *) NULL;

	return (char *) memcpy(ret, s1, len);
}

/* Ported from FBSD strtok.c 8.1 (Berkeley) 6/4/93 */
char * strtok_r(char *s, const char *delim, char **last)
{
	char *spanp, *tok;
	int c, sc;

	if (s == NULL && (s = *last) == NULL)
		return (NULL);

cont:
	c = *s++;
	for (spanp = (char *)delim; (sc = *spanp++) != 0;) {
		if (c == sc)
			goto cont;
	}

	if (c == 0) {		/* no non-delimiter characters */
		*last = NULL;
		return (NULL);
	}

	tok = s - 1;

	for (;;) {
		c = *s++;
		spanp = (char *)delim;
		do {
			if ((sc = *spanp++) == c) {
				if (c == 0)
					s = NULL;
				else
					s[-1] = '\0';
				*last = s;
				return (tok);
			}
		} while (sc != 0);
	}
}

/* Ported from FBSD strtok.c 8.1 (Berkeley) 6/4/93 */
char * strtok(char *s, const char *delim)
{
	static char *last;

	return (strtok_r(s, delim, &last));
}

/** @}
 */
