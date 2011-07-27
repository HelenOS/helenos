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
/** @file Filename-matching.
 */

// TODO: clean this up a bit

#include "stdbool.h"
#include "ctype.h"
#include "string.h"
#include "stdlib.h"
#include "assert.h"

#define LIBPOSIX_INTERNAL

#include "internal/common.h"
#include "fnmatch.h"

// TODO: documentation

#define INVALID_PATTERN -1

/* Type for collating element, simple identity with characters now,
 * but may be extended for better locale support.
 */
typedef int _coll_elm_t;

#define COLL_ELM_INVALID -1

/** Get collating element matching a string.
 *
 * @param str
 * @return
 */
static _coll_elm_t _coll_elm_get(const char* str)
{
	if (str[0] == '\0' || str[1] != '\0') {
		return COLL_ELM_INVALID;
	}
	return str[0];
}

/** Get collating element matching a single character.
 *
 * @param c
 * @return
 */
static _coll_elm_t _coll_elm_char(int c)
{
	return c;
}

/** Match collating element with a beginning of a string.
 *
 * @param elm
 * @param str
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
	{ "blank", isblank },
	{ "cntrl", iscntrl },
	{ "digit", isdigit },
	{ "graph", isgraph },
	{ "lower", islower },
	{ "print", isprint },
	{ "punct", ispunct },
	{ "space", isspace },
	{ "upper", isupper },
	{ "xdigit", isxdigit }
};

/**
 * 
 * @param key
 * @param elem
 * @return
 */
static int _class_compare(const void *key, const void *elem)
{
	const struct _char_class *class = elem;
	return strcmp((const char *) key, class->name);
}

/**
 * 
 * @param cname
 * @param c
 * @return
 */
static bool _is_in_class (const char *cname, int c)
{
	/* Search for class in the array of supported character classes. */
	const struct _char_class *class = bsearch(cname, _char_classes,
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

/**
 * 
 * @param pattern
 * @param str
 * @param flags
 * @return
 */
static int _match_char_class(const char **pattern, const char *str, int flags)
{
	char class[MAX_CLASS_OR_COLL_LEN + 1];

	if (!_get_delimited(pattern, ':', class, sizeof(class), flags)) {
		return INVALID_PATTERN;
	}

	return _is_in_class(class, *str);
}

/************** END CHARACTER CLASSES ****************/

/**
 * 
 * @param pattern
 * @param flags
 * @return
 */
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
 * @param pattern
 * @param str
 * @param flags
 * @return
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
 * @param pattern
 * @param string
 * @param flags
 * @return
 */
static bool _partial_match(const char **pattern, const char **string, int flags)
{
	/* Only a single *-delimited subpattern is matched here.
	 * So in this function, '*' is understood as the end of pattern.
	 */

	const bool pathname = (flags & FNM_PATHNAME) != 0;
	const bool special_period = (flags & FNM_PERIOD) != 0;
	const bool noescape = (flags & FNM_NOESCAPE) != 0;
	const bool leading_dir = (flags & FNM_LEADING_DIR) != 0;

	const char *s = *string;
	const char *p = *pattern;

	while (*p != '*') {
		/* Bracket expression. */
		if (*p == '[') {
			int matched = _match_bracket_expr(&p, s, flags);
			if (matched == 0) {
				/* Doesn't match. */
				return false;
			}
			if (matched != INVALID_PATTERN) {
				s += matched;
				continue;
			}

			assert(matched == INVALID_PATTERN);
			/* Fall through to match [ as an ordinary character. */
		}

		/* Wildcard match. */
		if (*p == '?') {
			if (*s == '\0') {
				/* No character to match. */
				return false;
			}
			if (pathname && *s == '/') {
				/* Slash must be matched explicitly. */
				return false;
			}
			if (special_period && pathname &&
			    *s == '.' && *(s - 1) == '/') {
				/* Initial period must be matched explicitly. */
				return false;
			}
			
			/* None of the above, match anything else. */
			p++;
			s++;
			continue;
		}

		if (!noescape && *p == '\\') {
			/* Escaped character. */
			p++;
		}

		if (*p == '\0') {
			/* End of pattern, must match end of string or
			 * an end of subdirectory name (optional).
			 */

			if (*s == '\0' || (leading_dir && *s == '/')) {
				break;
			}

			return false;
		}

		if (*p == *s) {
			/* Exact match. */
			p++;
			s++;
			continue;
		}

		/* Nothing matched. */
		return false;
	}

	/* Entire sub-pattern matched. */
	
	/* postconditions */
	assert(*p == '\0' || *p == '*');
	assert(*p != '\0' || *s == '\0' || (leading_dir && *s == '/'));
	
	*pattern = p;
	*string = s;
	return true;
}

/**
 * 
 * @param pattern
 * @param string
 * @param flags
 * @return
 */
static bool _full_match(const char *pattern, const char *string, int flags)
{
	const bool pathname = (flags & FNM_PATHNAME) != 0;
	const bool special_period = (flags & FNM_PERIOD) != 0;
	const bool leading_dir = (flags & FNM_LEADING_DIR) != 0;

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
		pattern++;

		bool matched = false;

		const char *end;
		if (pathname && special_period &&
		    *string == '.' && *(string - 1) == '/') {
			end = string;
		} else {
			end= strchrnul(string, pathname ? '/' : '\0');
		}

		/* Try to match every possible offset. */
		while (string <= end) {
			if (_partial_match(&pattern, &string, flags)) {
				matched = true;
				break;
			}
			string++;
		}

		if (matched) {
			continue;
		}

		return false;
	}

	return *string == '\0' || (leading_dir && *string == '/');
}

/**
 * 
 * @param s
 * @return
 */
static char *_casefold(const char *s)
{
	assert(s != NULL);
	char *result = strdup(s);
	for (char *i = result; *i != '\0'; ++i) {
		*i = tolower(*i);
	}
	return result;
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
	assert(pattern != NULL);
	assert(string != NULL);

	// TODO: don't fold everything in advance, but only when needed

	if ((flags & FNM_CASEFOLD) != 0) {
		/* Just fold the entire pattern and string. */
		pattern = _casefold(pattern);
		string = _casefold(string);
	}

	bool result = _full_match(pattern, string, flags);

	if ((flags & FNM_CASEFOLD) != 0) {
		if (pattern) {
			free((char *) pattern);
		}
		if (string) {
			free((char *) string);
		}
	}

	return result ? 0 : FNM_NOMATCH;
}

// FIXME: put the testcases somewhere else

#if 0

#include <stdio.h>

void __posix_fnmatch_test()
{
	int fail = 0;

	#undef assert
	#define assert(x) { if (x) printf("SUCCESS: "#x"\n"); else { printf("FAILED: "#x"\n"); fail++; } }
	#define match(s1, s2, flags) assert(posix_fnmatch(s1, s2, flags) == 0)
	#define nomatch(s1, s2, flags) assert(posix_fnmatch(s1, s2, flags) == FNM_NOMATCH)

	assert(FNM_PATHNAME == FNM_FILE_NAME);
	match("", "", 0);
	match("*", "hello", 0);
	match("hello", "hello", 0);
	match("hello*", "hello", 0);
	nomatch("hello?", "hello", 0);
	match("*hello", "prdel hello", 0);
	match("he[sl]lo", "hello", 0);
	match("he[sl]lo", "heslo", 0);
	nomatch("he[sl]lo", "heblo", 0);
	nomatch("he[^sl]lo", "hello", 0);
	nomatch("he[^sl]lo", "heslo", 0);
	match("he[^sl]lo", "heblo", 0);
	nomatch("he[!sl]lo", "hello", 0);
	nomatch("he[!sl]lo", "heslo", 0);
	match("he[!sl]lo", "heblo", 0);
	match("al*[c-t]a*vis*ta", "alheimer talir jehovista", 0);
	match("al*[c-t]a*vis*ta", "alfons had jehovista", 0);
	match("[a-ce-z]", "a", 0);
	match("[a-ce-z]", "c", 0);
	nomatch("[a-ce-z]", "d", 0);
	match("[a-ce-z]", "e", 0);
	match("[a-ce-z]", "z", 0);
	nomatch("[^a-ce-z]", "a", 0);
	nomatch("[^a-ce-z]", "c", 0);
	match("[^a-ce-z]", "d", 0);
	nomatch("[^a-ce-z]", "e", 0);
	nomatch("[^a-ce-z]", "z", 0);
	match("helen??", "helenos", 0);
	match("****booo****", "booo", 0);
	
	match("hello[[:space:]]world", "hello world", 0);
	nomatch("hello[[:alpha:]]world", "hello world", 0);
	
	match("/hoooo*", "/hooooooo/hooo", 0);
	nomatch("/hoooo*", "/hooooooo/hooo", FNM_PATHNAME);
	nomatch("/hoooo*/", "/hooooooo/hooo", FNM_PATHNAME);
	match("/hoooo*/*", "/hooooooo/hooo", FNM_PATHNAME);
	match("/hoooo*/hooo", "/hooooooo/hooo", FNM_PATHNAME);
	match("/hoooo*", "/hooooooo/hooo", FNM_PATHNAME | FNM_LEADING_DIR);
	nomatch("/hoooo*/", "/hooooooo/hooo", FNM_PATHNAME | FNM_LEADING_DIR);
	nomatch("/hoooo", "/hooooooo/hooo", FNM_LEADING_DIR);
	match("/hooooooo", "/hooooooo/hooo", FNM_LEADING_DIR);
	
	match("*", "hell", 0);
	match("*?", "hell", 0);
	match("?*?", "hell", 0);
	match("?*??", "hell", 0);
	match("??*??", "hell", 0);
	nomatch("???*??", "hell", 0);
	
	nomatch("", "hell", 0);
	nomatch("?", "hell", 0);
	nomatch("??", "hell", 0);
	nomatch("???", "hell", 0);
	match("????", "hell", 0);
	
	match("*", "h.ello", FNM_PERIOD);
	match("*", "h.ello", FNM_PATHNAME | FNM_PERIOD);
	nomatch("*", ".hello", FNM_PERIOD);
	match("h?ello", "h.ello", FNM_PERIOD);
	nomatch("?hello", ".hello", FNM_PERIOD);
	match("/home/user/.*", "/home/user/.hello", FNM_PATHNAME | FNM_PERIOD);
	match("/home/user/*", "/home/user/.hello", FNM_PERIOD);
	nomatch("/home/user/*", "/home/user/.hello", FNM_PATHNAME | FNM_PERIOD);

	nomatch("HeLlO", "hello", 0);
	match("HeLlO", "hello", FNM_CASEFOLD);

	printf("Failed: %d\n", fail);
}

#endif

/** @}
 */
