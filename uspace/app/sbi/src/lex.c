/*
 * Copyright (c) 2010 Jiri Svoboda
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

/** @file Lexer (lexical analyzer).
 *
 * Consumes a text file and produces a sequence of lexical elements (lems).
 */

#include <stdio.h>
#include <stdlib.h>
#include "mytypes.h"
#include "input.h"
#include "os/os.h"
#include "strtab.h"

#include "lex.h"

#define TAB_WIDTH 8

static void lex_touch(lex_t *lex);
static bool_t lex_read_try(lex_t *lex);

static void lex_skip_comment(lex_t *lex);
static void lex_skip_ws(lex_t *lex);
static bool_t is_wstart(char c);
static bool_t is_wcont(char c);
static bool_t is_digit(char c);
static void lex_word(lex_t *lex);
static void lex_number(lex_t *lex);
static void lex_string(lex_t *lex);
static int digit_value(char c);

/* Note: This imposes an implementation limit on identifier length. */
#define IBUF_SIZE 128
static char ident_buf[IBUF_SIZE + 1];

/* XXX This imposes an implementation limit on string literal length. */
#define SLBUF_SIZE 128
static char strlit_buf[SLBUF_SIZE + 1];

/** Lclass-string pair */
struct lc_name {
	lclass_t lclass;
	const char *name;
};

/** Keyword names. Used both for printing and recognition. */
static struct lc_name keywords[] = {
	{ lc_as,	"as" },
	{ lc_builtin,	"builtin" },
	{ lc_class,	"class" },
	{ lc_constructor,	"constructor" },
	{ lc_do,	"do" },
	{ lc_else,	"else" },
	{ lc_end,	"end" },
	{ lc_except,	"except" },
	{ lc_finally,	"finally" },
	{ lc_for,	"for" },
	{ lc_fun,	"fun" },
	{ lc_get,	"get" },
	{ lc_if,	"if" },
	{ lc_in,	"in" },
	{ lc_int,	"int" },
	{ lc_interface,	"interface" },
	{ lc_is,	"is" },
	{ lc_new,	"new" },
	{ lc_nil,	"nil" },
	{ lc_override,	"override" },
	{ lc_packed,	"packed" },
	{ lc_private,	"private" },
	{ lc_prop,	"prop" },
	{ lc_protected,	"protected" },
	{ lc_public,	"public" },
	{ lc_raise,	"raise" },
	{ lc_resource,	"resource" },
	{ lc_return,	"return" },
	{ lc_self,	"self" },
	{ lc_set,	"set" },
	{ lc_static,	"static" },
	{ lc_string,	"string" },
	{ lc_struct,	"struct" },
	{ lc_then,	"then" },
	{ lc_this,	"this" },
	{ lc_var,	"var" },
	{ lc_with,	"with" },
	{ lc_while,	"while" },
	{ lc_yield,	"yield" },

	{ 0,		NULL }
};

/** Other simple lclasses. Only used for printing. */
static struct lc_name simple_lc[] = {
	{ lc_invalid,	"INVALID" },
	{ lc_eof,	"EOF" },

	/* Operators */
	{ lc_period,	"." },
	{ lc_slash,	"/" },
	{ lc_lparen,	"(" },
	{ lc_rparen,	")" },
	{ lc_lsbr,	"[" },
	{ lc_rsbr,	"]" },
	{ lc_equal,	"==" },
	{ lc_notequal,	"!=" },
	{ lc_lt,	"<" },
	{ lc_gt,	">" },
	{ lc_lt_equal,	"<=" },
	{ lc_gt_equal,	">=" },
	{ lc_assign,	"=" },
	{ lc_plus,	"+" },
	{ lc_increase,	"+=" },

	/* Punctuators */
	{ lc_comma,	"," },
	{ lc_colon,	":" },
	{ lc_scolon,	";" },

	{ 0,		NULL },
};

/** Print lclass value.
 *
 * Prints lclass (lexical element class) value in human-readable form
 * (for debugging).
 *
 * @param lclass	Lclass value for display.
 */
void lclass_print(lclass_t lclass)
{
	struct lc_name *dp;

	dp = keywords;
	while (dp->name != NULL) {
		if (dp->lclass == lclass) {
			printf("%s", dp->name);
			return;
		}
		++dp;
	}

	dp = simple_lc;
	while (dp->name != NULL) {
		if (dp->lclass == lclass) {
			printf("%s", dp->name);
			return;
		}
		++dp;
	}

	switch (lclass) {
	case lc_ident:
		printf("ident");
		break;
	case lc_lit_int:
		printf("int_literal");
		break;
	case lc_lit_string:
		printf("string_literal");
		break;
	default:
		printf("<unknown?>");
		break;
	}
}

/** Print lexical element.
 *
 * Prints lexical element in human-readable form (for debugging).
 *
 * @param lem		Lexical element for display.
 */
void lem_print(lem_t *lem)
{
	lclass_print(lem->lclass);

	switch (lem->lclass) {
	case lc_ident:
		printf("(%d)", lem->u.ident.sid);
		break;
	case lc_lit_int:
		printf("(%d)", lem->u.lit_int.value);
		break;
	case lc_lit_string:
		printf("(\"%s\")", lem->u.lit_string.value);
	default:
		break;
	}
}

/** Print lem coordinates.
 *
 * Print the coordinates (line number, column number) of a lexical element.
 *
 * @param lem		Lexical element for coordinate printing.
 */
void lem_print_coords(lem_t *lem)
{
	printf("%d:%d", lem->line_no, lem->col_0);
}

/** Initialize lexer instance.
 *
 * @param lex		Lexer object to initialize.
 * @param input		Input to associate with lexer.
 */
void lex_init(lex_t *lex, struct input *input)
{
	int rc;

	lex->input = input;

	rc = input_get_line(lex->input, &lex->inbuf);
	if (rc != EOK) {
		printf("Error reading input.\n");
		exit(1);
	}

	lex->ibp = lex->inbuf;
	lex->col_adj = 0;
	lex->current_valid = b_true;
}

/** Advance to next lexical element.
 *
 * The new element is read in lazily then it is actually accessed.
 *
 * @param lex		Lexer object.
 */
void lex_next(lex_t *lex)
{
	/* Make sure the current lem has already been read in. */
	lex_touch(lex);

	/* Force a new lem to be read on next access. */
	lex->current_valid = b_false;
}

/** Get current lem.
 *
 * The returned pointer is invalidated by next call to lex_next()
 *
 * @param lex		Lexer object.
 */
lem_t *lex_get_current(lex_t *lex)
{
	lex_touch(lex);
	return &lex->current;
}

/** Read in the current lexical element (unless already read in).
 *
 * @param lex		Lexer object.
 */
static void lex_touch(lex_t *lex)
{
	bool_t got_lem;

	if (lex->current_valid == b_true)
		return;

	do {
		got_lem = lex_read_try(lex);
	} while (got_lem == b_false);

	lex->current_valid = b_true;
}

/** Try reading next lexical element.
 *
 * Attemps to read the next lexical element. In some cases (such as a comment)
 * this function will need to give it another try and returns @c b_false
 * in such case.
 *
 * @param lex		Lexer object.
 * @return		@c b_true on success or @c b_false if it needs
 *			restarting. On success the lem is stored to
 *			the current lem in @a lex.
 */
static bool_t lex_read_try(lex_t *lex)
{
	char *bp;

	lex_skip_ws(lex);

	/*
	 * Record lem coordinates. Line number we already have. For column
	 * number we start with position in the input buffer. This works
	 * for all characters except tab. Thus we keep track of tabs
	 * separately using col_adj.
	 */
	lex->current.line_no = input_get_line_no(lex->input);
	lex->current.col_0 = 1 + lex->col_adj + (lex->ibp - lex->inbuf);

	bp = lex->ibp;

	if (bp[0] == '\0') {
		/* End of input */
		lex->current.lclass = lc_eof;
		return b_true;
	}

	if (is_wstart(bp[0])) {
		lex_word(lex);
		return b_true;
	}

	if (is_digit(bp[0])) {
		lex_number(lex);
		return b_true;
	}

	if (bp[0] == '"') {
		lex_string(lex);
		return b_true;
	}

	if (bp[0] == '-' && bp[1] == '-') {
		lex_skip_comment(lex);
		return b_false;
	}

	switch (bp[0]) {
	case ',': lex->current.lclass = lc_comma; ++bp; break;
	case ':': lex->current.lclass = lc_colon; ++bp; break;
	case ';': lex->current.lclass = lc_scolon; ++bp; break;

	case '.': lex->current.lclass = lc_period; ++bp; break;
	case '/': lex->current.lclass = lc_slash; ++bp; break;
	case '(': lex->current.lclass = lc_lparen; ++bp; break;
	case ')': lex->current.lclass = lc_rparen; ++bp; break;
	case '[': lex->current.lclass = lc_lsbr; ++bp; break;
	case ']': lex->current.lclass = lc_rsbr; ++bp; break;

	case '=':
		if (bp[1] == '=') {
			lex->current.lclass = lc_equal; bp += 2; break;
		}
		lex->current.lclass = lc_assign; ++bp; break;

	case '!':
		if (bp[1] == '=') {
			lex->current.lclass = lc_notequal; bp += 2; break;
		}
		goto invalid;

	case '+':
		if (bp[1] == '=') {
			lex->current.lclass = lc_increase; bp += 2; break;
		}
		lex->current.lclass = lc_plus; ++bp; break;

	case '<':
		if (bp[1] == '=') {
			lex->current.lclass = lc_lt_equal; bp += 2; break;
		}
		lex->current.lclass = lc_lt; ++bp; break;

	case '>':
		if (bp[1] == '=') {
			lex->current.lclass = lc_gt_equal; bp += 2; break;
		}
		lex->current.lclass = lc_gt; ++bp; break;

	default:
		goto invalid;
	}

	lex->ibp = bp;
	return b_true;

invalid:
	lex->current.lclass = lc_invalid;
	++bp;
	lex->ibp = bp;

	return b_true;
}

/** Lex a word (identifier or keyword).
 *
 * Read in a word. This may later turn out to be a keyword or a regular
 * identifier. It is stored in the current lem in @a lex.
 *
 * @param lex		Lexer object.
 */
static void lex_word(lex_t *lex)
{
	struct lc_name *dp;
	char *bp;
	int idx;

	bp = lex->ibp;
	ident_buf[0] = bp[0];
	idx = 1;

	while (is_wcont(bp[idx])) {
		if (idx >= IBUF_SIZE) {
			printf("Error: Identifier too long.\n");
			exit(1);
		}

		ident_buf[idx] = bp[idx];
		++idx;
	}

	lex->ibp = bp + idx;

	ident_buf[idx] = '\0';

	dp = keywords;
	while (dp->name != NULL) {
		if (os_str_cmp(ident_buf, dp->name) == 0) {
			/* Match */
			lex->current.lclass = dp->lclass;
			return;
		}
		++dp;
	}

	/* No matching keyword -- it must be an identifier. */
	lex->current.lclass = lc_ident;
	lex->current.u.ident.sid = strtab_get_sid(ident_buf);
}

/** Lex a numeric literal.
 *
 * Reads in a numeric literal and stores it in the current lem in @a lex.
 *
 * @param lex		Lexer object.
 */
static void lex_number(lex_t *lex)
{
	char *bp;
	int value;

	bp = lex->ibp;
	value = 0;

	while (is_digit(*bp)) {
		value = value * 10 + digit_value(*bp);
		++bp;
	}

	lex->ibp = bp;

	lex->current.lclass = lc_lit_int;
	lex->current.u.lit_int.value = value;
}

/** Lex a string literal.
 *
 * Reads in a string literal and stores it in the current lem in @a lex.
 *
 * @param lex		Lexer object.
 */
static void lex_string(lex_t *lex)
{
	char *bp;
	int idx;

	bp = lex->ibp + 1;
	idx = 0;

	while (bp[idx] != '"') {
		if (idx >= SLBUF_SIZE) {
			printf("Error: String literal too long.\n");
			exit(1);
		}

		if (bp[idx] == '\0') {
			printf("Error: Unterminated string literal.\n");
			exit(1);
		}

		strlit_buf[idx] = bp[idx];
		++idx;
	}

	lex->ibp = bp + idx + 1;

	strlit_buf[idx] = '\0';

	lex->current.lclass = lc_lit_string;
	lex->current.u.lit_string.value = os_str_dup(strlit_buf);
}

/** Lex a single-line comment.
 *
 * This does not produce any lem. The comment is just skipped.
 *
 * @param lex		Lexer object.
 */
static void lex_skip_comment(lex_t *lex)
{
	char *bp;

	bp = lex->ibp + 2;

	while (*bp != '\n' && *bp != '\0') {
		++bp;
	}

	lex->ibp = bp;
}

/** Skip whitespace characters.
 *
 * This does not produce any lem. The whitespace is just skipped.
 *
 * @param lex		Lexer object.
 */
static void lex_skip_ws(lex_t *lex)
{
	char *bp;
	int rc;

	bp = lex->ibp;

	while (b_true) {
		while (*bp == ' ' || *bp == '\t') {
			if (*bp == '\t') {
				/* XXX This is too simplifed. */
				lex->col_adj += (TAB_WIDTH - 1);
			}
			++bp;
		}

		if (*bp != '\n')
			break;

		/* Read next line */
		rc = input_get_line(lex->input, &lex->inbuf);
		if (rc != EOK) {
			printf("Error reading input.\n");
			exit(1);
		}

		bp = lex->inbuf;
		lex->col_adj = 0;
	}

	lex->ibp = bp;
}

/** Determine if character can start a word.
 *
 * @param c 	Character.
 * @return	@c b_true if @a c can start a word, @c b_false otherwise.
 */
static bool_t is_wstart(char c)
{
	return ((c >= 'a') && (c <= 'z')) || ((c >= 'A') && (c <= 'Z')) ||
	    (c == '_');
}

/** Determine if character can continue a word.
 *
 * @param c 	Character.
 * @return	@c b_true if @a c can start continue word, @c b_false
 *		otherwise.
 */
static bool_t is_wcont(char c)
{
	return is_digit(c) || is_wstart(c);
}

/** Determine if character is a numeric digit.
 *
 * @param c 	Character.
 * @return	@c b_true if @a c is a numeric digit, @c b_false otherwise.
 */
static bool_t is_digit(char c)
{
	return ((c >= '0') && (c <= '9'));
}

/** Determine numeric value of digit character.
 *
 * @param c 	Character, must be a valid decimal digit.
 * @return	Value of the digit (0-9).
 */
static int digit_value(char c)
{
	return (c - '0');
}
