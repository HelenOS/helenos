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

/* Prevent an error from being generated */
#define _REALLY_WANT_STRING_H
#include <string.h>
#include <pcut/pcut.h>

#ifndef __clang__
#pragma GCC diagnostic ignored "-Wstringop-truncation"
#pragma GCC diagnostic ignored "-Wstringop-overflow"
#pragma GCC diagnostic ignored "-Wstringop-overread"
#endif

PCUT_INIT;

PCUT_TEST_SUITE(string);

/** strcpy function */
PCUT_TEST(strcpy)
{
	const char *s = "hello";
	char buf[7];
	size_t i;
	char *p;

	for (i = 0; i < 7; i++)
		buf[i] = 'X';

	p = strcpy(buf, s);

	PCUT_ASSERT_TRUE(p == buf);
	PCUT_ASSERT_TRUE(buf[0] == 'h');
	PCUT_ASSERT_TRUE(buf[1] == 'e');
	PCUT_ASSERT_TRUE(buf[2] == 'l');
	PCUT_ASSERT_TRUE(buf[3] == 'l');
	PCUT_ASSERT_TRUE(buf[4] == 'o');
	PCUT_ASSERT_TRUE(buf[5] == '\0');
	PCUT_ASSERT_TRUE(buf[6] == 'X');
}

/** strncpy function with n == 0 */
PCUT_TEST(strncpy_zero)
{
	const char *s = "hello";
	char buf[1];
	char *p;

	buf[0] = 'X';

	p = strncpy(buf, s, 0);

	/* No characters are copied */
	PCUT_ASSERT_TRUE(p == buf);
	PCUT_ASSERT_TRUE(buf[0] == 'X');
}

/** strncpy function with string longer than n argument */
PCUT_TEST(strncpy_long)
{
	const char *s = "hello";
	char buf[5];
	size_t i;
	char *p;

	for (i = 0; i < 5; i++)
		buf[i] = 'X';

	p = strncpy(buf, s, 4);

	PCUT_ASSERT_TRUE(p == buf);
	PCUT_ASSERT_TRUE(buf[0] == 'h');
	PCUT_ASSERT_TRUE(buf[1] == 'e');
	PCUT_ASSERT_TRUE(buf[2] == 'l');
	PCUT_ASSERT_TRUE(buf[3] == 'l');
	PCUT_ASSERT_TRUE(buf[4] == 'X');
}

/** strncpy function with string containing exactly n characters */
PCUT_TEST(strncpy_just)
{
	const char *s = "hello";
	char buf[6];
	size_t i;
	char *p;

	for (i = 0; i < 6; i++)
		buf[i] = 'X';

	p = strncpy(buf, s, 5);

	PCUT_ASSERT_TRUE(p == buf);
	PCUT_ASSERT_TRUE(buf[0] == 'h');
	PCUT_ASSERT_TRUE(buf[1] == 'e');
	PCUT_ASSERT_TRUE(buf[2] == 'l');
	PCUT_ASSERT_TRUE(buf[3] == 'l');
	PCUT_ASSERT_TRUE(buf[4] == 'o');
	PCUT_ASSERT_TRUE(buf[5] == 'X');
}

/** strncpy function with string containing exactly n - 1 characters */
PCUT_TEST(strncpy_just_over)
{
	const char *s = "hello";
	char buf[7];
	size_t i;
	char *p;

	for (i = 0; i < 7; i++)
		buf[i] = 'X';

	p = strncpy(buf, s, 6);

	PCUT_ASSERT_TRUE(p == buf);
	PCUT_ASSERT_TRUE(buf[0] == 'h');
	PCUT_ASSERT_TRUE(buf[1] == 'e');
	PCUT_ASSERT_TRUE(buf[2] == 'l');
	PCUT_ASSERT_TRUE(buf[3] == 'l');
	PCUT_ASSERT_TRUE(buf[4] == 'o');
	PCUT_ASSERT_TRUE(buf[5] == '\0');
	PCUT_ASSERT_TRUE(buf[6] == 'X');
}

/** strncpy function with string containing less than n - 1 characters */
PCUT_TEST(strncpy_over)
{
	const char *s = "hello";
	char buf[8];
	size_t i;
	char *p;

	for (i = 0; i < 8; i++)
		buf[i] = 'X';

	p = strncpy(buf, s, 7);

	PCUT_ASSERT_TRUE(p == buf);
	PCUT_ASSERT_TRUE(buf[0] == 'h');
	PCUT_ASSERT_TRUE(buf[1] == 'e');
	PCUT_ASSERT_TRUE(buf[2] == 'l');
	PCUT_ASSERT_TRUE(buf[3] == 'l');
	PCUT_ASSERT_TRUE(buf[4] == 'o');
	PCUT_ASSERT_TRUE(buf[5] == '\0');
	PCUT_ASSERT_TRUE(buf[6] == '\0');
	PCUT_ASSERT_TRUE(buf[7] == 'X');
}

/** strcat function */
PCUT_TEST(strcat)
{
	char buf[7];
	const char *s = "cde";
	size_t i;
	char *p;

	buf[0] = 'a';
	buf[1] = 'b';
	buf[2] = '\0';

	for (i = 3; i < 7; i++)
		buf[i] = 'X';

	p = strcat(buf, s);

	PCUT_ASSERT_TRUE(p == buf);
	PCUT_ASSERT_TRUE(buf[0] == 'a');
	PCUT_ASSERT_TRUE(buf[1] == 'b');
	PCUT_ASSERT_TRUE(buf[2] == 'c');
	PCUT_ASSERT_TRUE(buf[3] == 'd');
	PCUT_ASSERT_TRUE(buf[4] == 'e');
	PCUT_ASSERT_TRUE(buf[5] == '\0');
	PCUT_ASSERT_TRUE(buf[6] == 'X');
}

/** strncat function with n == 0 */
PCUT_TEST(strncat_zero)
{
	const char *s = "cde";
	char buf[4];
	char *p;

	buf[0] = 'a';
	buf[1] = 'b';
	buf[2] = '\0';
	buf[3] = 'X';

	p = strncat(buf, s, 0);

	PCUT_ASSERT_TRUE(p == buf);
	PCUT_ASSERT_TRUE(buf[0] == 'a');
	PCUT_ASSERT_TRUE(buf[1] == 'b');
	PCUT_ASSERT_TRUE(buf[2] == '\0');
	PCUT_ASSERT_TRUE(buf[3] == 'X');
}

/** strncat function with string longer than n argument */
PCUT_TEST(strncat_long)
{
	const char *s = "cde";
	char buf[6];
	size_t i;
	char *p;

	buf[0] = 'a';
	buf[1] = 'b';
	buf[2] = '\0';

	for (i = 3; i < 6; i++)
		buf[i] = 'X';

	p = strncat(buf, s, 2);

	PCUT_ASSERT_TRUE(p == buf);
	PCUT_ASSERT_TRUE(buf[0] == 'a');
	PCUT_ASSERT_TRUE(buf[1] == 'b');
	PCUT_ASSERT_TRUE(buf[2] == 'c');
	PCUT_ASSERT_TRUE(buf[3] == 'd');
	PCUT_ASSERT_TRUE(buf[4] == '\0');
	PCUT_ASSERT_TRUE(buf[5] == 'X');
}

/** strncat function with string containing exactly n characters */
PCUT_TEST(strncat_just)
{
	const char *s = "cde";
	char buf[7];
	size_t i;
	char *p;

	buf[0] = 'a';
	buf[1] = 'b';
	buf[2] = '\0';

	for (i = 3; i < 7; i++)
		buf[i] = 'X';

	p = strncat(buf, s, 3);

	PCUT_ASSERT_TRUE(p == buf);
	PCUT_ASSERT_TRUE(buf[0] == 'a');
	PCUT_ASSERT_TRUE(buf[1] == 'b');
	PCUT_ASSERT_TRUE(buf[2] == 'c');
	PCUT_ASSERT_TRUE(buf[3] == 'd');
	PCUT_ASSERT_TRUE(buf[4] == 'e');
	PCUT_ASSERT_TRUE(buf[5] == '\0');
	PCUT_ASSERT_TRUE(buf[6] == 'X');
}

/** strncat function with string containing exactly n - 1 characters */
PCUT_TEST(strncat_just_over)
{
	const char *s = "cde";
	char buf[7];
	size_t i;
	char *p;

	buf[0] = 'a';
	buf[1] = 'b';
	buf[2] = '\0';

	for (i = 3; i < 7; i++)
		buf[i] = 'X';

	p = strncat(buf, s, 4);

	PCUT_ASSERT_TRUE(p == buf);
	PCUT_ASSERT_TRUE(buf[0] == 'a');
	PCUT_ASSERT_TRUE(buf[1] == 'b');
	PCUT_ASSERT_TRUE(buf[2] == 'c');
	PCUT_ASSERT_TRUE(buf[3] == 'd');
	PCUT_ASSERT_TRUE(buf[4] == 'e');
	PCUT_ASSERT_TRUE(buf[5] == '\0');
	PCUT_ASSERT_TRUE(buf[6] == 'X');
}

/** strncat function with string containing less than n - 1 characters */
PCUT_TEST(strncat_over)
{
	const char *s = "cde";
	char buf[7];
	size_t i;
	char *p;

	buf[0] = 'a';
	buf[1] = 'b';
	buf[2] = '\0';

	for (i = 3; i < 7; i++)
		buf[i] = 'X';

	p = strncat(buf, s, 5);

	PCUT_ASSERT_TRUE(p == buf);
	PCUT_ASSERT_TRUE(buf[0] == 'a');
	PCUT_ASSERT_TRUE(buf[1] == 'b');
	PCUT_ASSERT_TRUE(buf[2] == 'c');
	PCUT_ASSERT_TRUE(buf[3] == 'd');
	PCUT_ASSERT_TRUE(buf[4] == 'e');
	PCUT_ASSERT_TRUE(buf[5] == '\0');
	PCUT_ASSERT_TRUE(buf[6] == 'X');
}

/** strcmp function with different characters after terminating null */
PCUT_TEST(strcmp_same)
{
	PCUT_ASSERT_TRUE(strcmp("apples\0#", "apples\0$") == 0);
}

/** strcmp function with first string less than second */
PCUT_TEST(strcmp_less_than)
{
	PCUT_ASSERT_TRUE(strcmp("apples", "oranges") < 0);
}

/** strcmp function with first string greater than second */
PCUT_TEST(strcmp_greater_than)
{
	PCUT_ASSERT_TRUE(strcmp("oranges", "apples") > 0);
}

/* strcmp function with first string a prefix of second string */
PCUT_TEST(strcmp_prefix)
{
	PCUT_ASSERT_TRUE(strcmp("apple", "apples") < 0);
}

/** strcoll function */
PCUT_TEST(strcoll)
{
	/* Same string with different characters after terminating null */
	PCUT_ASSERT_TRUE(strcoll("apples\0#", "apples\0$") == 0);

	/* First string less than second */
	PCUT_ASSERT_TRUE(strcoll("apples", "oranges") < 0);

	/* First string greater than second */
	PCUT_ASSERT_TRUE(strcoll("oranges", "apples") > 0);

	/* First string is prefix of second */
	PCUT_ASSERT_TRUE(strcoll("apple", "apples") < 0);
}

/** strncmp function with n == 0 */
PCUT_TEST(strncmp_zero)
{
	PCUT_ASSERT_TRUE(strncmp("apple", "orange", 0) == 0);
}

/** strncmp function with strings differing after n characters */
PCUT_TEST(strncmp_long)
{
	PCUT_ASSERT_TRUE(strncmp("apple", "apricot", 2) == 0);
}

/** strncmp function with strings differing in (n-1)th character */
PCUT_TEST(strncmp_just)
{
	PCUT_ASSERT_TRUE(strncmp("apple", "apricot", 3) < 0);
}

/** strncmp function with strings differing before (n-1)th character */
PCUT_TEST(strncmp_over)
{
	PCUT_ASSERT_TRUE(strncmp("dart", "tart", 3) < 0);
}

/** strxfrm function with null destination to determine size needed */
PCUT_TEST(strxfrm_null)
{
	size_t n;

	n = strxfrm(NULL, "hello", 0);
	PCUT_ASSERT_INT_EQUALS(5, n);
}

/** strxfrm function with string longer than n argument */
PCUT_TEST(strxfrm_long)
{
	const char *s = "hello";
	char buf[5];
	size_t i;
	size_t n;

	for (i = 0; i < 5; i++)
		buf[i] = 'X';

	n = strxfrm(buf, s, 4);

	PCUT_ASSERT_INT_EQUALS(5, n);
	PCUT_ASSERT_TRUE(buf[0] == 'h');
	PCUT_ASSERT_TRUE(buf[1] == 'e');
	PCUT_ASSERT_TRUE(buf[2] == 'l');
	PCUT_ASSERT_TRUE(buf[3] == 'l');
	PCUT_ASSERT_TRUE(buf[4] == 'X');
}

/** strxfrm function with string containing exactly n characters */
PCUT_TEST(strxfrm_just)
{
	const char *s = "hello";
	char buf[6];
	size_t i;
	size_t n;

	for (i = 0; i < 6; i++)
		buf[i] = 'X';

	n = strxfrm(buf, s, 5);

	PCUT_ASSERT_INT_EQUALS(5, n);
	PCUT_ASSERT_TRUE(buf[0] == 'h');
	PCUT_ASSERT_TRUE(buf[1] == 'e');
	PCUT_ASSERT_TRUE(buf[2] == 'l');
	PCUT_ASSERT_TRUE(buf[3] == 'l');
	PCUT_ASSERT_TRUE(buf[4] == 'o');
	PCUT_ASSERT_TRUE(buf[5] == 'X');
}

/** strxfrm function with string containing exactly n - 1 characters */
PCUT_TEST(strxfrm_just_over)
{
	const char *s = "hello";
	char buf[7];
	size_t i;
	size_t n;

	for (i = 0; i < 7; i++)
		buf[i] = 'X';

	n = strxfrm(buf, s, 6);

	PCUT_ASSERT_INT_EQUALS(5, n);
	PCUT_ASSERT_TRUE(buf[0] == 'h');
	PCUT_ASSERT_TRUE(buf[1] == 'e');
	PCUT_ASSERT_TRUE(buf[2] == 'l');
	PCUT_ASSERT_TRUE(buf[3] == 'l');
	PCUT_ASSERT_TRUE(buf[4] == 'o');
	PCUT_ASSERT_TRUE(buf[5] == '\0');
	PCUT_ASSERT_TRUE(buf[6] == 'X');
}

/** strxfrm function with string containing less than n - 1 characters */
PCUT_TEST(strxfrm_over)
{
	const char *s = "hello";
	char buf[8];
	size_t i;
	size_t n;

	for (i = 0; i < 8; i++)
		buf[i] = 'X';

	n = strxfrm(buf, s, 7);

	PCUT_ASSERT_INT_EQUALS(5, n);
	PCUT_ASSERT_TRUE(buf[0] == 'h');
	PCUT_ASSERT_TRUE(buf[1] == 'e');
	PCUT_ASSERT_TRUE(buf[2] == 'l');
	PCUT_ASSERT_TRUE(buf[3] == 'l');
	PCUT_ASSERT_TRUE(buf[4] == 'o');
	PCUT_ASSERT_TRUE(buf[5] == '\0');
	PCUT_ASSERT_TRUE(buf[6] == 'X');
	PCUT_ASSERT_TRUE(buf[7] == 'X');
}

/** strchr function searching for null character */
PCUT_TEST(strchr_nullchar)
{
	const char *s = "abcabc";
	char *p;

	p = strchr(s, '\0');
	PCUT_ASSERT_TRUE((const char *)p == &s[6]);
}

/** strchr function with character occurring in string */
PCUT_TEST(strchr_found)
{
	const char *s = "abcabc";
	char *p;

	p = strchr(s, 'b');
	PCUT_ASSERT_TRUE((const char *)p == &s[1]);
}

/** strchr function with character not occurring in string */
PCUT_TEST(strchr_not_found)
{
	const char *s = "abcabc";
	char *p;

	p = strchr(s, 'd');
	PCUT_ASSERT_TRUE(p == NULL);
}

/** strcspn function with empty search string */
PCUT_TEST(strcspn_empty_str)
{
	size_t n;

	n = strcspn("", "abc");
	PCUT_ASSERT_INT_EQUALS(0, n);
}

/** strcspn function with empty character set */
PCUT_TEST(strcspn_empty_set)
{
	size_t n;

	n = strcspn("abc", "");
	PCUT_ASSERT_INT_EQUALS(3, n);
}

/** strcspn function with regular arguments */
PCUT_TEST(strcspn_regular)
{
	size_t n;

	n = strcspn("baBAba", "AB");
	PCUT_ASSERT_INT_EQUALS(2, n);
}

/** strpbrk function with empty search string */
PCUT_TEST(strpbrk_empty_string)
{
	const char *p;

	p = strpbrk("", "abc");
	PCUT_ASSERT_NULL(p);
}

/** strpbrk function with empty character set */
PCUT_TEST(strpbrk_empty_set)
{
	const char *p;

	p = strpbrk("abc", "");
	PCUT_ASSERT_NULL(p);
}

/** strpbrk function with regular parameters */
PCUT_TEST(strpbrk_regular)
{
	const char *s = "baBAba";
	char *p;

	p = strpbrk(s, "ab");
	PCUT_ASSERT_TRUE((const char *)p == s);
}

/** strrchr function searching for null character */
PCUT_TEST(strrchr_nullchar)
{
	const char *s = "abcabc";
	char *p;

	p = strrchr(s, '\0');
	PCUT_ASSERT_TRUE((const char *)p == &s[6]);
}

/** strrchr function with character occurring in string */
PCUT_TEST(strrchr_found)
{
	const char *s = "abcabc";
	char *p;

	p = strrchr(s, 'b');
	PCUT_ASSERT_TRUE((const char *)p == &s[4]);
}

/** strrchr function with character not occurring in string */
PCUT_TEST(strrchr_not_found)
{
	const char *s = "abcabc";
	char *p;

	p = strrchr(s, 'd');
	PCUT_ASSERT_TRUE(p == NULL);
}

/** strspn function with empty search string */
PCUT_TEST(strspn_empty_str)
{
	size_t n;

	n = strspn("", "abc");
	PCUT_ASSERT_INT_EQUALS(0, n);
}

/** strspn function with empty character set */
PCUT_TEST(strspn_empty_set)
{
	size_t n;

	n = strspn("abc", "");
	PCUT_ASSERT_INT_EQUALS(0, n);
}

/** strspn function with regular arguments */
PCUT_TEST(strspn_regular)
{
	size_t n;

	n = strspn("baBAba", "ab");
	PCUT_ASSERT_INT_EQUALS(2, n);
}

/** strstr function looking for empty substring */
PCUT_TEST(strstr_empty)
{
	const char *str = "abcabcabcdabc";
	char *p;

	p = strstr(str, "");
	PCUT_ASSERT_TRUE((const char *) p == str);
}

/** strstr function looking for substring with success */
PCUT_TEST(strstr_found)
{
	const char *str = "abcabcabcdabc";
	char *p;

	p = strstr(str, "abcd");
	PCUT_ASSERT_TRUE((const char *) p == &str[6]);
}

/** strstr function looking for substring with failure */
PCUT_TEST(strstr_notfound)
{
	const char *str = "abcabcabcdabc";
	char *p;

	p = strstr(str, "abcde");
	PCUT_ASSERT_NULL(p);
}

/** strtok function */
PCUT_TEST(strtok)
{
	char str[] = ":a::b;;;$c";
	char *t;

	t = strtok(str, ":");
	PCUT_ASSERT_TRUE(t == &str[1]);
	PCUT_ASSERT_INT_EQUALS(0, strcmp(t, "a"));

	t = strtok(NULL, ";");
	PCUT_ASSERT_TRUE(t == &str[3]);
	PCUT_ASSERT_INT_EQUALS(0, strcmp(t, ":b"));

	t = strtok(NULL, "$;");
	PCUT_ASSERT_TRUE(t == &str[9]);
	PCUT_ASSERT_INT_EQUALS(0, strcmp(t, "c"));

	t = strtok(NULL, "$");
	PCUT_ASSERT_NULL(t);
}

/** strerror function with zero argument */
PCUT_TEST(strerror_zero)
{
	char *p;

	p = strerror(0);
	PCUT_ASSERT_NOT_NULL(p);
}

/** strerror function with errno value argument */
PCUT_TEST(strerror_errno)
{
	char *p;

	p = strerror(EINVAL);
	PCUT_ASSERT_NOT_NULL(p);
}

/** strerror function with negative argument */
PCUT_TEST(strerror_negative)
{
	char *p;

	p = strerror(-1);
	PCUT_ASSERT_NOT_NULL(p);
}

/** strlen function with empty string */
PCUT_TEST(strlen_empty)
{
	PCUT_ASSERT_INT_EQUALS(0, strlen(""));
}

/** strlen function with non-empty string */
PCUT_TEST(strlen_nonempty)
{
	PCUT_ASSERT_INT_EQUALS(3, strlen("abc"));
}

/** strlen function with empty string and non-zero limit */
PCUT_TEST(strnlen_empty_short)
{
	PCUT_ASSERT_INT_EQUALS(0, strnlen("", 1));
}

/** strlen function with empty string and zero limit */
PCUT_TEST(strnlen_empty_eq)
{
	PCUT_ASSERT_INT_EQUALS(0, strnlen("", 0));
}

/** strlen function with non empty string below limit */
PCUT_TEST(strnlen_nonempty_short)
{
	PCUT_ASSERT_INT_EQUALS(3, strnlen("abc", 5));
}

/** strlen function with non empty string just below limit */
PCUT_TEST(strnlen_nonempty_just_short)
{
	PCUT_ASSERT_INT_EQUALS(3, strnlen("abc", 4));
}

/** strlen function with non empty string of length equal to limit */
PCUT_TEST(strnlen_nonempty_eq)
{
	PCUT_ASSERT_INT_EQUALS(3, strnlen("abc", 3));
}

/** strlen function with non empty string of length above limit */
PCUT_TEST(strnlen_nonempty_long)
{
	PCUT_ASSERT_INT_EQUALS(2, strnlen("abc", 2));
}

/** strdup function with empty string */
PCUT_TEST(strdup_empty)
{
	char *d = strdup("");
	PCUT_ASSERT_NOT_NULL(d);
	PCUT_ASSERT_TRUE(d[0] == '\0');
	free(d);
}

/** strdup function with non-empty string */
PCUT_TEST(strdup_nonempty)
{
	char *d = strdup("abc");
	PCUT_ASSERT_NOT_NULL(d);
	PCUT_ASSERT_TRUE(d[0] == 'a');
	PCUT_ASSERT_TRUE(d[1] == 'b');
	PCUT_ASSERT_TRUE(d[2] == 'c');
	PCUT_ASSERT_TRUE(d[3] == '\0');
	free(d);
}

/** strndup function with empty string and non-zero limit */
PCUT_TEST(strndup_empty_short)
{
	char *d = strndup("", 1);
	PCUT_ASSERT_NOT_NULL(d);
	PCUT_ASSERT_TRUE(d[0] == '\0');
	free(d);
}

/** strndup function with empty string and zero limit */
PCUT_TEST(strndup_empty_eq)
{
	char *d = strndup("", 0);
	PCUT_ASSERT_NOT_NULL(d);
	PCUT_ASSERT_TRUE(d[0] == '\0');
	free(d);
}

/** strndup function with non-empty string of length below limit */
PCUT_TEST(strndup_nonempty_short)
{
#pragma GCC diagnostic push
	// Intentionally checking it works with _longer_ size than actual
#if defined(__GNUC__) && (__GNUC__ >= 11)
#pragma GCC diagnostic ignored "-Wstringop-overread"
#endif
	char *d = strndup("abc", 5);
#pragma GCC diagnostic pop
	PCUT_ASSERT_NOT_NULL(d);
	PCUT_ASSERT_TRUE(d[0] == 'a');
	PCUT_ASSERT_TRUE(d[1] == 'b');
	PCUT_ASSERT_TRUE(d[2] == 'c');
	PCUT_ASSERT_TRUE(d[3] == '\0');
	free(d);
}

/** strndup function with non-empty string of length equal to limit */
PCUT_TEST(strndup_nonempty_eq)
{
	char *d = strndup("abc", 3);
	PCUT_ASSERT_NOT_NULL(d);
	PCUT_ASSERT_TRUE(d[0] == 'a');
	PCUT_ASSERT_TRUE(d[1] == 'b');
	PCUT_ASSERT_TRUE(d[2] == 'c');
	PCUT_ASSERT_TRUE(d[3] == '\0');
	free(d);
}

/** strndup function with non-empty string of length above limit */
PCUT_TEST(strndup_nonempty_long)
{
	char *d = strndup("abc", 2);
	PCUT_ASSERT_NOT_NULL(d);
	PCUT_ASSERT_TRUE(d[0] == 'a');
	PCUT_ASSERT_TRUE(d[1] == 'b');
	PCUT_ASSERT_TRUE(d[2] == '\0');
	free(d);
}

PCUT_EXPORT(string);
