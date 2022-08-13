/*
 * SPDX-FileCopyrightText: 2006 Josef Cejka
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libc
 * @{
 */
/** @file
 */

#include <ctype.h>

int islower(int c)
{
	return ((c >= 'a') && (c <= 'z'));
}

int isupper(int c)
{
	return ((c >= 'A') && (c <= 'Z'));
}

int isalpha(int c)
{
	return (islower(c) || isupper(c));
}

int isdigit(int c)
{
	return ((c >= '0') && (c <= '9'));
}

int isalnum(int c)
{
	return (isalpha(c) || isdigit(c));
}

int isblank(int c)
{
	return c == ' ' || c == '\t';
}

int iscntrl(int c)
{
	return (c >= 0 && c < 0x20) || c == 0x7E;
}

int isprint(int c)
{
	return c >= 0 && c < 0x80 && !iscntrl(c);
}

int isgraph(int c)
{
	return isprint(c) && c != ' ';
}

int isspace(int c)
{
	switch (c) {
	case ' ':
	case '\n':
	case '\t':
	case '\f':
	case '\r':
	case '\v':
		return 1;
	default:
		return 0;
	}
}

int ispunct(int c)
{
	return !isspace(c) && !isalnum(c) && isprint(c);
}

int isxdigit(int c)
{
	return isdigit(c) ||
	    (c >= 'a' && c <= 'f') ||
	    (c >= 'A' && c <= 'F');
}

int tolower(int c)
{
	if (isupper(c))
		return (c + ('a' - 'A'));
	else
		return c;
}

int toupper(int c)
{
	if (islower(c))
		return (c + ('A' - 'a'));
	else
		return c;
}

/** @}
 */
