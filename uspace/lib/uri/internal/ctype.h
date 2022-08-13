/*
 * SPDX-FileCopyrightText: 2013 Martin Sucha
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup uri
 * @{
 */
/**
 * @file
 */

#ifndef URI_CTYPE_H_
#define URI_CTYPE_H_

static inline bool is_unreserved(char c)
{
	return isalpha(c) || isdigit(c) ||
	    c == '-' || c == '.' || c == '_' || c == '~';
}

static inline bool is_hexdig(char c)
{
	return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') ||
	    (c >= 'A' && c <= 'F');
}

static inline bool is_subdelim(char c)
{
	switch (c) {
	case '!':
	case '$':
	case '&':
	case '\'':
	case '(':
	case ')':
	case '*':
	case '+':
	case ',':
	case ';':
	case '=':
		return true;
	default:
		return false;
	}
}

static inline bool is_gendelim(char c)
{
	switch (c) {
	case ':':
	case '/':
	case '?':
	case '#':
	case '[':
	case ']':
	case '@':
		return true;
	default:
		return false;
	}
}

static inline bool is_reserved(char c)
{
	return is_gendelim(c) || is_subdelim(c);
}

#endif

/** @}
 */
