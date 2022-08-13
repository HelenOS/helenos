/*
 * SPDX-FileCopyrightText: 2013 Martin Sucha
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup http
 * @{
 */
/**
 * @file
 */

#ifndef HTTP_CTYPE_H_
#define HTTP_CTYPE_H_

static inline bool is_separator(char c)
{
	switch (c) {
	case '(':
	case ')':
	case '<':
	case '>':
	case '@':
	case ',':
	case ';':
	case ':':
	case '\\':
	case '"':
	case '/':
	case '[':
	case ']':
	case '?':
	case '=':
	case '{':
	case '}':
	case ' ':
	case '\t':
		return true;
	default:
		return false;
	}
}

static inline bool is_lws(char c)
{
	switch (c) {
	case '\r':
	case '\n':
	case ' ':
	case '\t':
		return true;
	default:
		return false;
	}
}

static inline bool is_ctl(char c)
{
	return c < 32 || c == 127;
}

static inline bool is_token(char c)
{
	return !is_separator(c) && !is_ctl(c);
}

#endif

/** @}
 */
