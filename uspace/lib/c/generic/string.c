/*
 * Copyright (c) 2018 Jiri Svoboda
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

#include <errno.h>
#include <stddef.h>
#include <stdlib.h>
#include <str_error.h>
#include <string.h>

/** Copy string.
 *
 * Copy the string pointed to by @a s2 to the array pointed to by @a s1
 * including the terminating null character. The source and destination
 * must not overlap.
 *
 * @param s1 Destination array
 * @param s2 Source string
 * @return @a s1
 */
char *strcpy(char *s1, const char *s2)
{
	char *dp = s1;

	/* Copy characters */
	while (*s2 != '\0')
		*dp++ = *s2++;

	/* Copy the terminating null character */
	*dp++ = *s2++;

	return s1;
}

/** Copy not more than @a n characters.
 *
 * Copy not more than @a n characters from  @a s2 to @a s1. Characters
 * following a null character are not copied. If The string @a s2 is
 * shorter than @a n characters, null characters are appended to the
 * copy in @a s1, until @a n characters in all have been written.
 *
 * (I.e. @a s1 is padded with null characters up to size @a n).
 * The source and destination must not overlap.
 *
 * @param s1 Destination array
 * @param s2 Source string
 * @param n Number of characters to copy
 * @return @a s1
 */
char *strncpy(char *s1, const char *s2, size_t n)
{
	char *dp = s1;
	size_t i;

	for (i = 0; i < n; i++) {
		*dp++ = *s2;
		if (*s2 != '\0')
			++s2;
	}

	return s1;
}

/** Append string.
 *
 * Append a copy of string in @a s2 to the string pointed to by @a s1
 * (including the terminating null character). @a s1 and @a s2 must not
 * overlap.
 *
 * @param s1 Destination buffer holding a string to be appended to
 * @param s2 String to be appended
 * @return @a s1
 */
char *strcat(char *s1, const char *s2)
{
	char *dp = s1;

	/* Find end of first string */
	while (*dp != '\0')
		++dp;

	/* Copy second string */
	while (*s2 != '\0')
		*dp++ = *s2++;

	/* Copy null character */
	*dp = '\0';

	return s1;
}

/** Append not more than @a n characters.
 *
 * Append a copy of max. @a n characters from string in @a s2 to the string
 * pointed to by @a s1. The resulting string is always null-terminated.
 *
 * @param s1 Destination buffer holding a string to be appended to
 * @param s2 String to be appended
 * @param n Maximum number of characters to copy
 * @return @a s1
 */
char *strncat(char *s1, const char *s2, size_t n)
{
	char *dp = s1;

	/* Find end of first string */
	while (*dp != '\0')
		++dp;

	/* Copy second string */
	while (*s2 != '\0' && n > 0) {
		*dp++ = *s2++;
		--n;
	}

	/* Copy null character */
	*dp = '\0';

	return s1;
}

/** Compare two strings.
 *
 * @param s1 First string
 * @param s2 Second string
 * @return Greater than, equal to, less than zero if @a s1 > @a s2,
 *         @a s1 == @a s2, @a s1 < @a s2, resp.
 */
int strcmp(const char *s1, const char *s2)
{
	while (*s1 == *s2 && *s1 != '\0') {
		++s1;
		++s2;
	}

	return *s1 - *s2;
}

/** Compare two strings based on LC_COLLATE of current locale.
 *
 * @param s1 First string
 * @param s2 Second string
 * @return Greater than, equal to, less than zero if @a s1 > @a s2,
 *         @a s1 == @a s2, @a s1 < @a s2, resp.
 */
int strcoll(const char *s1, const char *s2)
{
	/* Note: we don't support locale other than "C" */
	return strcmp(s1, s2);
}

/** Compare not more than @a n characters.
 *
 * @param s1 First string
 * @param s2 Second string
 * @param n Maximum number of characters to compare
 * @return Greater than, equal to, less than zero if @a s1 > @a s2,
 *         @a s1 == @a s2, @a s1 < @a s2, resp. (within the first @a n chars.)
 */
int strncmp(const char *s1, const char *s2, size_t n)
{
	while (*s1 == *s2 && *s1 != '\0' && n > 0) {
		++s1;
		++s2;
		--n;
	}

	if (n > 0)
		return *s1 - *s2;

	return 0;
}

/** Transform string for collation.
 *
 * Transform string in @a s2 to the buffer @a s1, writing no more than
 * @a n characters (including the terminating null character). The transformed
 * string is such that using strcmp on two transformed strings should be
 * equivalent to using strcoll on the original strings.
 *
 * @param s1 Destination buffer
 * @param s2 Source string
 * @param n Max. number of characters to write (including terminating null)
 * @return Length of the transformed string not including the null terminator.
 *         If the value returned is @a n or more, the contents of the buffer
 *         pointed to by @a s1 are undefined.
 */
size_t strxfrm(char *s1, const char *s2, size_t n)
{
	size_t i;
	size_t len;

	len = strlen(s2);

	for (i = 0; i < n; i++) {
		*s1++ = *s2;
		if (*s2 == '\0')
			break;
		++s2;
	}

	return len;
}

/** Find the first occurrence of a character in a string.
 *
 * The character @a c is converted to char. The null character is
 * considered part of the string.
 *
 * @param s String
 * @param c Character
 * @return Pointer to the located character or @c NULL if the character
 *         does not occur in the string.
 */
char *strchr(const char *s, int c)
{
	do {
		if (*s == (char) c)
			return (char *) s;
	} while (*s++ != '\0');

	return NULL;
}

/** Compute the size of max. initial segment consisting of a complementary
 * set of characters.
 *
 * Compute the size of the max. initial segment of @a s1 consisting only
 * of characters *not* from @a s2.
 *
 * @param s1 String to search
 * @param s2 String containing set of characters
 * @return Size of initial segment of @a s1 consisting only of characters
 *         not from @a s2.
 */
size_t strcspn(const char *s1, const char *s2)
{
	char *p;
	size_t n;

	n = 0;
	while (*s1 != '\0') {
		/* Look for current character in s2 */
		p = strchr(s2, *s1);

		/* If found, return current character count. */
		if (p != NULL)
			break;

		++s1;
		++n;
	}

	return n;
}

/** Search string for occurrence of any of a set of characters.
 *
 * @param s1 String to search
 * @param s2 String containing a set of characters
 *
 * @return Pointer to first character found or @c NULL if not found
 */
char *strpbrk(const char *s1, const char *s2)
{
	char *p;

	while (*s1 != '\0') {
		/* Look for current character in s2 */
		p = strchr(s2, *s1);

		/* If found, return pointer to current character. */
		if (p != NULL)
			return (char *) s1;

		++s1;
	}

	return NULL;
}

/** Find the last occurrence of a character in a string.
 *
 * The character @a c is converted to char. The null character is
 * considered part of the string.
 *
 * @param s String
 * @param c Character
 * @return Pointer to the located character or @c NULL if the character
 *         does not occur in the string.
 */
char *strrchr(const char *s, int c)
{
	size_t i = strlen(s);

	while (i > 0) {
		if (s[i] == (char) c)
			return (char *)s + i;
		--i;
	}

	return NULL;
}

/** Compute the size of max. initial segment consisting of a set of characters.
 *
 * Compute tha size of the max. initial segment of @a s1 consisting only
 * of characters from @a s2.
 *
 * @param s1 String to search
 * @param s2 String containing set of characters
 * @return Size of initial segment of @a s1 consisting only of characters
 *         from @a s2.
 */
size_t strspn(const char *s1, const char *s2)
{
	char *p;
	size_t n;

	n = 0;
	while (*s1 != '\0') {
		/* Look for current character in s2 */
		p = strchr(s2, *s1);

		/* If not found, return current character count. */
		if (p == NULL)
			break;

		++s1;
		++n;
	}

	return n;
}

/** Find occurrence of substring in a string.
 *
 * Find the first occurrence in @a s1 of the characters in @a s2, excluding
 * the terminating null character. If s2 is an empty string, returns @a s1.
 *
 * @param s1 String to search
 * @param s2 Sequence of characters to find
 *
 * @return Pointer inside @a s1 or @c NULL if not found.
 */
char *strstr(const char *s1, const char *s2)
{
	size_t len;

	/*
	 * Naive search algorithm.
	 *
	 * Two-Way String-Matching might be a plausible alternative
	 * for larger haystack+needle combinations.
	 */

	len = strlen(s2);
	while (*s1 != '\0') {
		if (strncmp(s1, s2, len) == 0)
			return (char *) s1;
		++s1;
	}

	return NULL;
}

/** Tokenize a string (reentrant).
 *
 * The contents of @a s1 are modified (the separators get overwritten by null
 * characters). The separators can be different each iteration, their identity\
 * is, however, lost.
 *
 * @param s1 String buffer to get the first token, @c NULL to get the next
 *           token
 * @param s2 String containing current separators
 * @return Pointer to the next token
 */
char *__strtok_r(char *s1, const char *s2, char **saveptr)
{
	char *s;
	char *tbegin;
	char *tend;

	if (s1 != NULL) {
		/* Starting tokenization of a new string */
		s = s1;
	} else {
		/* Use saved position of next token */
		s = *saveptr;

		/* Check if we ran out of tokens */
		if (s == NULL)
			return NULL;
	}

	/* Find position of first character that is not a separator */
	tbegin = s;
	while (*tbegin != '\0' && strchr(s2, *tbegin) != NULL)
		++tbegin;

	/* If no such character is found, there are no tokens */
	if (*tbegin == '\0')
		return NULL;

	/* Find first character that is a separator */
	tend = strpbrk(tbegin, s2);
	if (tend != NULL) {
		/* Overwrite the separator, next token starts just after */
		*tend = '\0';
		*saveptr = tend + 1;
	} else {
		/* No more tokens will be returned next time */
		*saveptr = NULL;
	}

	return tbegin;
}

/** Saved position of next token for function strtok */
static char *strtok_saveptr = NULL;

/** Tokenize a string.
 *
 * This function has internal state and thus is not reentrant.
 * ISO C says the implementation should behave as if no (standard) library
 * call calls strtok. The contents of @a s1 are modified (the separators
 * get overwritten by null characters). The separators can be different
 * each iteration, their identity is, however, lost.
 *
 * Never use this function since it's not reentrant.
 *
 * @param s1 String buffer to get the first token, @c NULL to get the next
 *           token
 * @param s2 String containing current separators
 * @return Pointer to the next token
 */
char *strtok(char *s1, const char *s2)
{
	return __strtok_r(s1, s2, &strtok_saveptr);
}

/** Map error number to a string.
 *
 * The string returned by the function may be overwritten by a subsequent
 * call to strerror (ISO C). In our implementation the function is, in fact,
 * reentrant, as the string returned is static and never modified.
 *
 * @param errnum Error number
 * @return Pointer to error message
 */
char *strerror(int errnum)
{
	return (char *) str_error(errnum);
}

/** Return number of characters in string.
 *
 * @param s String
 * @return Number of characters preceding the null character.
 */
size_t strlen(const char *s)
{
	size_t n;

	n = 0;
	while (*s != '\0') {
		++s;
		++n;
	}

	return n;
}

/** Return number of characters in string with length limit.
 *
 * @param s String
 * @param maxlen Maximum number of characters to read
 * @return Number of characters preceding the null character, at most @a maxlen.
 */
size_t strnlen(const char *s, size_t maxlen)
{
	size_t n;

	n = 0;
	while (n < maxlen && *s != '\0') {
		++s;
		++n;
	}

	return n;
}

/** Allocate a new duplicate of string.
 *
 * @param s String to duplicate
 * @return New string or @c NULL on failure (in which case @c errno is set
 *         to ENOMEM).
 */
char *strdup(const char *s)
{
	size_t sz;
	char *dup;

	sz = strlen(s);
	dup = malloc(sz + 1);
	if (dup == NULL) {
		errno = ENOMEM;
		return NULL;
	}

	strcpy(dup, s);
	return dup;
}

/** Allocate a new duplicate of string with length limit.
 *
 * Creates a new duplicate of @a s. If @a s is longer than @a n characters,
 * only @a n characters are copied and a null character is appended.
 *
 * @param s String to duplicate
 * @param n Maximum number of characters to copy
 * @return New string or @c NULL on failure (in which case @c errno is set
 *         to ENOMEM).
 */
char *strndup(const char *s, size_t n)
{
	size_t sz;
	char *dup;

	sz = strnlen(s, n);
	dup = malloc(sz + 1);
	if (dup == NULL) {
		errno = ENOMEM;
		return NULL;
	}

	strncpy(dup, s, sz);
	return dup;
}

/** @}
 */
