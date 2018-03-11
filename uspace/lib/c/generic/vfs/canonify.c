/*
 * Copyright (c) 2008 Jakub Jermar
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

/**
 * @file
 * @brief
 */

#include <stdlib.h>
#include <stddef.h>
#include <vfs/canonify.h>

/** Token types used for tokenization of path. */
typedef enum {
	TK_INVALID,
	TK_SLASH,
	TK_DOT,
	TK_DOTDOT,
	TK_COMP,
	TK_NUL
} tokval_t;

typedef struct {
	tokval_t kind;
	char *start;
	char *stop;
} token_t;

/** Fake up the TK_SLASH token. */
static token_t slash_token(char *start)
{
	token_t ret;
	ret.kind = TK_SLASH;
	ret.start = start;
	ret.stop = start;
	return ret;
}

/** Given a token, return the next token. */
static token_t next_token(token_t *cur)
{
	token_t ret;

	if (cur->stop[1] == '\0') {
		ret.kind = TK_NUL;
		ret.start = cur->stop + 1;
		ret.stop = ret.start;
		return ret;
	}
	if (cur->stop[1] == '/') {
		ret.kind = TK_SLASH;
		ret.start = cur->stop + 1;
		ret.stop = ret.start;
		return ret;
	}
	if (cur->stop[1] == '.' && (!cur->stop[2] || cur->stop[2] == '/')) {
		ret.kind = TK_DOT;
		ret.start = cur->stop + 1;
		ret.stop = ret.start;
		return ret;
	}
	if (cur->stop[1] == '.' && cur->stop[2] == '.' &&
	    (!cur->stop[3] || cur->stop[3] == '/')) {
		ret.kind = TK_DOTDOT;
		ret.start = cur->stop + 1;
		ret.stop = cur->stop + 2;
		return ret;
	}
	unsigned i = 1;
	while (cur->stop[i] && cur->stop[i] != '/')
		i++;
	
	ret.kind = TK_COMP;
	ret.start = &cur->stop[1];
	ret.stop = &cur->stop[i - 1];
	return ret;
}

/** States used by canonify(). */
typedef enum {
	S_INI,
	S_A,
	S_B,
	S_C,
	S_ACCEPT,
	S_RESTART,
	S_REJECT
} state_t;

typedef struct {
	state_t s;
	void (* f)(token_t *, token_t *, token_t *);
} change_state_t;

/*
 * Actions that can be performed when transitioning from one
 * state of canonify() to another.
 */
static void set_first_slash(token_t *t, token_t *tfsl, token_t *tlcomp)
{
	*tfsl = *t;
	*tlcomp = *t;
}
static void save_component(token_t *t, token_t *tfsl, token_t *tlcomp)
{
	*tlcomp = *t;
}
static void terminate_slash(token_t *t, token_t *tfsl, token_t *tlcomp)
{
	if (tfsl->stop[1])	/* avoid writing to a well-formatted path */
		tfsl->stop[1] = '\0';
}
static void remove_trailing_slash(token_t *t, token_t *tfsl, token_t *tlcomp)
{
	t->start[-1] = '\0';
}
/** Eat the extra '/'.
 *
 * @param t		The current TK_SLASH token.
 */
static void shift_slash(token_t *t, token_t *tfsl, token_t *tlcomp)
{
	char *p = t->start;
	char *q = t->stop + 1;
	while ((*p++ = *q++))
		;
}
/** Eat the extra '.'.
 *
 * @param t		The current TK_DOT token.
 */
static void shift_dot(token_t *t, token_t *tfsl, token_t *tlcomp)
{
	char *p = t->start;
	char *q = t->stop + 1;
	while ((*p++ = *q++))
		;
}
/** Collapse the TK_COMP TK_SLASH TK_DOTDOT pattern.
 *
 * @param t		The current TK_DOTDOT token.
 * @param tlcomp	The last TK_COMP token.
 */
static void shift_dotdot(token_t *t, token_t *tfsl, token_t *tlcomp)
{
	char *p = tlcomp->start;
	char *q = t->stop + 1;
	while ((*p++ = *q++))
		;
}

/** Transition function for canonify(). */
static change_state_t trans[4][6] = {
	[S_INI] = {
		[TK_SLASH] = {
			.s = S_A,
			.f = set_first_slash,
		},
		[TK_DOT] = {
			.s = S_REJECT,
			.f = NULL,
		},
		[TK_DOTDOT] = {
			.s = S_REJECT,
			.f = NULL,
		},
		[TK_COMP] = {
			.s = S_REJECT,
			.f = NULL,
		},
		[TK_NUL] = {
			.s = S_REJECT,
			.f = NULL,
		},
		[TK_INVALID] = {
			.s = S_REJECT,
			.f = NULL,
		},
	},
	[S_A] = {
		[TK_SLASH] = {
			.s = S_A,
			.f = set_first_slash,
		},
		[TK_DOT] = {
			.s = S_A,
			.f = NULL,
		},
		[TK_DOTDOT] = {
			.s = S_A,
			.f = NULL,
		},
		[TK_COMP] = {
			.s = S_B,
			.f = save_component,
		},
		[TK_NUL] = {
			.s = S_ACCEPT,
			.f = terminate_slash,
		},
		[TK_INVALID] = {
			.s = S_REJECT,
			.f = NULL,
		},
	},
	[S_B] = {
		[TK_SLASH] = {
			.s = S_C,
			.f = NULL,
		},
		[TK_DOT] = {
			.s = S_REJECT,
			.f = NULL,
		},
		[TK_DOTDOT] = {
			.s = S_REJECT,
			.f = NULL,
		},
		[TK_COMP] = {
			.s = S_REJECT,
			.f = NULL,
		},
		[TK_NUL] = {
			.s = S_ACCEPT,
			.f = NULL,
		},
		[TK_INVALID] = {
			.s = S_REJECT,
			.f = NULL,
		},
	},
	[S_C] = {
		[TK_SLASH] = {
			.s = S_RESTART,
			.f = shift_slash,
		},
		[TK_DOT] = {
			.s = S_RESTART,
			.f = shift_dot,
		},
		[TK_DOTDOT] = {
			.s = S_RESTART,
			.f = shift_dotdot,
		},
		[TK_COMP] = {
			.s = S_B,
			.f = save_component,
		},
		[TK_NUL] = {
			.s = S_ACCEPT,
			.f = remove_trailing_slash,
		},
		[TK_INVALID] = {
			.s = S_REJECT,
			.f = NULL,
		},
	}
};

/** Canonify a file system path.
 *
 * A file system path is canonical, if the following holds:
 *
 * 1) the path is absolute
 *    (i.e. a/b/c is not canonical)
 * 2) there is no trailing slash in the path if it has components
 *    (i.e. /a/b/c/ is not canonical)
 * 3) there is no extra slash in the path
 *    (i.e. /a//b/c is not canonical)
 * 4) there is no '.' component in the path
 *    (i.e. /a/./b/c is not canonical)
 * 5) there is no '..' component in the path
 *    (i.e. /a/b/../c is not canonical)
 *
 * This function makes a potentially non-canonical file system path canonical.
 * It works in-place and requires a NULL-terminated input string.
 *
 * @param path		Path to be canonified.
 * @param lenp		Pointer where the length of the final path will be
 *			stored. Can be NULL.
 *
 * @return		Canonified path or NULL on failure.
 */
char *canonify(char *path, size_t *lenp)
{
	state_t state;
	token_t t;
	token_t tfsl;		/* first slash */
	token_t tlcomp;		/* last component */
	if (*path != '/')
		return NULL;
	tfsl = slash_token(path);
restart:
	state = S_INI;
	t = tfsl;
	tlcomp = tfsl;
	while (state != S_ACCEPT && state != S_RESTART && state != S_REJECT) {
		if (trans[state][t.kind].f)
			trans[state][t.kind].f(&t, &tfsl, &tlcomp);
		state = trans[state][t.kind].s;
		t = next_token(&t);
	}

	switch (state) {
	case S_RESTART:
		goto restart;
	case S_REJECT:
		return NULL;
	case S_ACCEPT:
		if (lenp)
			*lenp = (size_t)((tlcomp.stop - tfsl.start) + 1);
		return tfsl.start;
	default:
		abort();
	}
}

/**
 * @}
 */
