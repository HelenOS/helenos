/*
 * Copyright (c) 2011 Jiri Zarevucky
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

/** @addtogroup libposix
 * @{
 */
/** @file
 */

#define LIBPOSIX_INTERNAL

#include "internal/common.h"
#include "fnmatch.h"

#include "stdlib.h"
#include "string.h"
#include "ctype.h"

#define INVALID_PATTERN -1

/* Type for collating element, simple identity with characters now,
 * but may be extended for better locale support.
 */
typedef int _coll_elm_t;

#define COLL_ELM_INVALID -1

static _coll_elm_t _coll_elm_get(const char* str)
{
	if (str[0] == '\0' || str[1] != '\0') {
		return COLL_ELM_INVALID;
	}
	return str[0];
}

static _coll_elm_t _coll_elm_char(int c)
{
	return c;
}

/**
 *
 * @param elm
 * @param pattern
 * @return 0 if the element doesn't match, or the number of characters matched.
 */
static int _coll_elm_match(_coll_elm_t elm, const char *str)
{
	return elm == *str;
}

static int _coll_elm_between(_coll_elm_t first, _coll_elm_t second,
    const char *str)
{
	return *str >= first && *str <= second;
}

/** Read a string delimited by [? and ?].
 *
 * @param pattern Pointer to the string to read from. Its position is moved
 *    to the first character after the closing ].
 * @param seq The character on the inside of brackets.
 * @param buf Read buffer.
 * @param buf_sz Read buffer's size. If the buffer is not large enough for
 *    the entire string, the string is cut with no error indication.
 * @return
 */
static bool _get_delimited(
    const char **pattern, int seq,
    char *buf, size_t buf_sz, int flags)
{
	const bool noescape = (flags & FNM_NOESCAPE) != 0;
	const bool pathname = (flags & FNM_PATHNAME) != 0;

	const char *p = *pattern;
	assert(p[0] == '[' && p[1] == seq /* Caller should ensure this. */);
	p += 2;

	while (true) {
		if (*p == seq && *(p + 1) == ']') {
			/* String properly ended, return. */
			*pattern = p + 2;
			*buf = '\0';
			return true;
		}
		if (!noescape && *p == '\\') {
			p++;
		}
		if (*p == '\0') {
			/* String not ended properly, invalid pattern. */
			return false;
		}
		if (pathname && *p == '/') {
			/* Slash in a pathname pattern is invalid. */
			return false;
		}
		if (buf_sz > 1) {
			/* Only add to the buffer if there is space. */
			*buf = *p;
			buf++;
			buf_sz--;
		}
		p++;
	}
}

/************** CHARACTER CLASSES ****************/

#define MAX_CLASS_OR_COLL_LEN 6

struct _char_class {
	const char *name;
	int (*func) (int);
};

/* List of supported character classes. */
static const struct _char_class _char_classes[] = {
	{ "alnum", isalnum },
	{ "alpha", isalpha },
	{ "blank", posix_isblank },
	{ "cntrl", posix_iscntrl },
	{ "digit", isdigit },
	{ "graph", posix_isgraph },
	{ "lower", islower },
	{ "print", posix_isprint },
	{ "punct", posix_ispunct },
	{ "space", isspace },
	{ "upper", isupper },
	{ "xdigit", posix_isxdigit }
};

static int _class_compare(const void *key, const void *elem)
{
	const struct _char_class *class = elem;
	return posix_strcmp((const char *) key, class->name);
}

static bool _is_in_class (const char *cname, int c)
{
	/* Search for class in the array of supported character classes. */
	const struct _char_class *class = posix_bsearch(cname, _char_classes,
	    sizeof(_char_classes) / sizeof(struct _char_class),
	    sizeof(struct _char_class), _class_compare);

	if (class == NULL) {
		/* No such class supported - treat as an empty class. */
		return false;
	} else {
		/* Class matched. */
		return class->func(c);
	}
}

static int _match_char_class(const char **pattern, const char *str, int flags)
{
	char class[MAX_CLASS_OR_COLL_LEN + 1];

	if (!_get_delimited(pattern, ':', class, sizeof(class), flags)) {
		return INVALID_PATTERN;
	}

	return _is_in_class(class, *str);
}

/************** END CHARACTER CLASSES ****************/

static _coll_elm_t _next_coll_elm(const char **pattern, int flags)
{
	const char *p = *pattern;
	const bool noescape = (flags & FNM_NOESCAPE) != 0;
	const bool pathname = (flags & FNM_PATHNAME) != 0;

	if (*p == '[') {
		if (*(p + 1) == '.') {
			char buf[MAX_CLASS_OR_COLL_LEN + 1];
			if (!_get_delimited(pattern, '.', buf, sizeof(buf), flags)) {
				return COLL_ELM_INVALID;
			}
			return _coll_elm_get(buf);
		}

		if (*(p + 1) == '=') {
			char buf[MAX_CLASS_OR_COLL_LEN + 1];
			if (!_get_delimited(pattern, '=', buf, sizeof(buf), flags)) {
				return COLL_ELM_INVALID;
			}
			return _coll_elm_get(buf);
		}
	}

	if (!noescape && *p == '\\') {
		p++;
	}
	if (pathname && *p == '/') {
		return COLL_ELM_INVALID;
	}

	*pattern = p + 1;
	return _coll_elm_char(*p);
}

/**
 *
 */
static int _match_bracket_expr(const char **pattern, const char *str, int flags)
{
	const bool pathname = (flags & FNM_PATHNAME) != 0;
	const bool special_period = (flags & FNM_PERIOD) != 0;
	const char *p = *pattern;
	bool negative = false;
	int matched = 0;

	#define _matched(match) { \
		int _match = match; \
		if (_match < 0) { \
			/* Invalid pattern */ \
			return _match; \
		} else if (matched == 0 && _match > 0) { \
			/* First match */ \
			matched = _match; \
		} \
	}

	assert(*p == '[');  /* calling code should ensure this */
	p++;

	if (*str == '\0' || (pathname && *str == '/') ||
	    (pathname && special_period && *str == '.' && *(str - 1) == '/')) {
		/* No bracket expression matches end of string,
		 * slash in pathname match or initial period with FNM_PERIOD
		 * option.
		 */
		return 0;
	}

	if (*p == '^' || *p == '!') {
		negative = true;
		p++;
	}

	if (*p == ']') {
		/* When ']' is first, treat it as a normal character. */
		_matched(*str == ']');
		p++;
	}
	
	_coll_elm_t current_elm = COLL_ELM_INVALID;
	
	while (*p != ']') {
		if (*p == '-' && *(p + 1) != ']' &&
		    current_elm != COLL_ELM_INVALID) {
			/* Range expression. */
			p++;
			_coll_elm_t end_elm = _next_coll_elm(&p, flags);
			if (end_elm == COLL_ELM_INVALID) {
				return INVALID_PATTERN;
			}
			_matched(_coll_elm_between(current_elm, end_elm, str));
			continue;
		}
	
		if (*p == '[' && *(p + 1) == ':') {
			current_elm = COLL_ELM_INVALID;
			_matched(_match_char_class(&p, str, flags));
			continue;
		}
		
		current_elm = _next_coll_elm(&p, flags);
		if (current_elm == COLL_ELM_INVALID) {
			return INVALID_PATTERN;
		}
		_matched(_coll_elm_match(current_elm, str));
	}

	/* No error occured - update pattern pointer. */
	*pattern = p + 1;

	if (matched == 0) {
		/* No match found */
		return negative;
	} else {
		/* Matched 'match' characters. */
		return negative ? 0 : matched;
	}

	#undef _matched
}

/**
 *
 */
static bool _partial_match(const char **pattern, const char **string, int flags)
{
	/* Only a single *-delimited subpattern is matched here.
	 * So in this function, '*' is understood as the end of pattern.
	 */

	const bool pathname = (flags & FNM_PATHNAME) != 0;
	const bool special_period = (flags & FNM_PERIOD) != 0;
	const bool noescape = (flags & FNM_NOESCAPE) != 0;
	const char *s = *string;
	const char *p = *pattern;

	for (; *p != '*'; p++) {
		if (*p == '[') {
			/* Bracket expression. */
			int matched = _match_bracket_expr(&p, s, flags);
			if (matched == 0) {
				/* Doesn't match. */
				return false;
			}
			if (matched == INVALID_PATTERN) {
				/* Fall through to match [ as an ordinary
				 * character.
				 */
			} else {
				s += matched;
				continue;
			}
		}

		if (*p == '?') {
			/* Wildcard match. */
			if (*s == '\0' || (pathname && *s == '/') ||
			    (special_period && pathname && *s == '.' &&
			    *(s - 1) == '/')) {
				return false;
			}
			p++;
			s++;
			continue;
		}

		if (!noescape && *p == '\\') {
			/* Escaped character. */
			p++;
		}

		if (*p == *s) {
			/* Exact match. */
			if (*s == '\0') {
				break;
			}
			continue;
		}

		/* Nothing matched. */
		return false;
	}

	/* Entire pattern matched. */
	*pattern = p;
	*string = s;
	return true;
}

static bool _full_match(const char *pattern, const char *string, int flags)
{
	const bool special_period = (flags & FNM_PERIOD) != 0;

	if (special_period && *string == '.') {
		/* Initial dot must be matched by an explicit dot in pattern. */
		if (*pattern != '.') {
			return false;
		}
		pattern++;
		string++;
	}

	if (*pattern != '*') {
		if (!_partial_match(&pattern, &string, flags)) {
			/* The initial match must succeed. */
			return false;
		}
	}

	while (*pattern != '\0') {
		assert(*pattern == '*');

		while (*pattern == '*') {
			pattern++;
		}

		/* Try to match every possible offset. */
		while (*string != '\0') {
			if (_partial_match(&pattern, &string, flags)) {
				break;
			}
			string++;
		}
	}

	return *string == '\0';
}

/**
 * Filename pattern matching.
 *
 * @param pattern
 * @param string
 * @param flags
 * @return
 */
int posix_fnmatch(const char *pattern, const char *string, int flags)
{
	bool result = _full_match(pattern, string, flags);
	return result ? 0 : FNM_NOMATCH;
}

/** @}
 */

