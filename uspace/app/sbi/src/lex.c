/*
 * Copyright (c) 2011 Jiri Svoboda
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
#include "bigint.h"
#include "cspan.h"
#include "mytypes.h"
#include "input.h"
#include "os/os.h"
#include "strtab.h"

#include "lex.h"

#define TAB_WIDTH 8

typedef enum {
	cs_chr,
	cs_str
} chr_str_t;

static void lex_touch(lex_t *lex);
static bool_t lex_read_try(lex_t *lex);

static void lex_skip_comment(lex_t *lex);
static void lex_skip_ws(lex_t *lex);
static bool_t is_wstart(char c);
static bool_t is_wcont(char c);
static bool_t is_digit(char c);
static void lex_word(lex_t *lex);
static void lex_char(lex_t *lex);
static void lex_number(lex_t *lex);
static void lex_string(lex_t *lex);
static void lex_char_string_core(lex_t *lex, chr_str_t cs);
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
	{ lc_and,	"and" },
	{ lc_as,	"as" },
	{ lc_bool,	"bool" },
	{ lc_break,	"break" },
	{ lc_builtin,	"builtin" },
	{ lc_char,	"char" },
	{ lc_class,	"class" },
	{ lc_deleg,	"deleg" },
	{ lc_do,	"do" },
	{ lc_elif,	"elif" },
	{ lc_else,	"else" },
	{ lc_end,	"end" },
	{ lc_enum,	"enum" },
	{ lc_except,	"except" },
	{ lc_false,	"false" },
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
	{ lc_not,	"not" },
	{ lc_nil,	"nil" },
	{ lc_or,	"or" },
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
	{ lc_switch,	"switch" },
	{ lc_then,	"then" },
	{ lc_this,	"this" },
	{ lc_true,	"true" },
	{ lc_var,	"var" },
	{ lc_with,	"with" },
	{ lc_when,	"when" },
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
	{ lc_minus,	"-" },
	{ lc_mult,	"*" },
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
		printf("('%s')", strtab_get_str(lem->u.ident.sid));
		break;
	case lc_lit_int:
		printf("(");
		bigint_print(&lem->u.lit_int.value);
		printf(")");
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
	cspan_print(lem->cspan);
}

/** Initialize lexer instance.
 *
 * @param lex		Lexer object to initialize.
 * @param input		Input to associate with lexer.
 */
void lex_init(lex_t *lex, struct input *input)
{
	errno_t rc;

	lex->input = input;

	rc = input_get_line(lex->input, &lex->inbuf);
	if (rc != EOK) {
		printf("Error reading input.\n");
		exit(1);
	}

	lex->ibp = lex->inbuf;
	lex->col_adj = 0;
	lex->prev_valid = b_false;
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
 * @return		Pointer to current lem. Owned by @a lex and only valid
 *			until next call to lex_xxx().
 */
lem_t *lex_get_current(lex_t *lex)
{
	lex_touch(lex);
	return &lex->current;
}

/** Get previous lem if valid.
 *
 * The returned pointer is invalidated by next call to lex_next()
 *
 * @param lex		Lexer object.
 * @return		Pointer to previous lem. Owned by @a lex and only valid
 *			until next call to lex_xxx().
 */
lem_t *lex_peek_prev(lex_t *lex)
{
	if (lex->current_valid == b_false) {
		/*
		 * This means the head is advanced but next lem was not read.
		 * Thus the previous lem is still in @a current.
		 */
		return &lex->current;
	}

	if (lex->prev_valid != b_true) {
		/* Looks like we are still at the first lem. */
		return NULL;
	}

	/*
	 * Current lem has been read in. Thus the previous lem was moved to
	 * @a previous.
	 */
	return &lex->prev;
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

	/* Copy previous lem */
	lex->prev = lex->current;
	lex->prev_valid = b_true;

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
	char *bp, *lsp;
	int line0, col0;

	lex_skip_ws(lex);

	/*
	 * Record lem coordinates. Line number we already have. For column
	 * number we start with position in the input buffer. This works
	 * for all characters except tab. Thus we keep track of tabs
	 * separately using col_adj.
	 */
	line0 = input_get_line_no(lex->input);
	col0 = 1 + lex->col_adj + (lex->ibp - lex->inbuf);

	lex->current.cspan = cspan_new(lex->input, line0, col0, line0, col0);

	lsp = lex->ibp;
	bp = lex->ibp;

	if (bp[0] == '\0') {
		/* End of input */
		lex->current.lclass = lc_eof;
		goto finish;
	}

	if (is_wstart(bp[0])) {
		lex_word(lex);
		goto finish;
	}

	if (bp[0] == '\'') {
		lex_char(lex);
		goto finish;
	}

	if (is_digit(bp[0])) {
		lex_number(lex);
		goto finish;
	}

	if (bp[0] == '"') {
		lex_string(lex);
		goto finish;
	}

	if (bp[0] == '-' && bp[1] == '-') {
		lex_skip_comment(lex);

		/* Compute ending column number */
		lex->current.cspan->col1 = col0 + (lex->ibp - lsp) - 1;

		/* Try again */
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

	case '-':
		lex->current.lclass = lc_minus; ++bp; break;

	case '*':
		lex->current.lclass = lc_mult; ++bp; break;

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

finish:
	/* Compute ending column number */
	lex->current.cspan->col1 = col0 + (lex->ibp - lsp) - 1;
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

/** Lex a character literal.
 *
 * Reads in a character literal and stores it in the current lem in @a lex.
 *
 * @param lex		Lexer object.
 */
static void lex_char(lex_t *lex)
{
	size_t len;
	int char_val;

	lex_char_string_core(lex, cs_chr);

	len = os_str_length(strlit_buf);
	if (len != 1) {
		printf("Character literal should contain one character, "
		    "but contains %u characters instead.\n", (unsigned) len);
		exit(1);
	}

	os_str_get_char(strlit_buf, 0, &char_val);
	lex->current.lclass = lc_lit_char;
	bigint_init(&lex->current.u.lit_char.value, char_val);
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
	bigint_t value;
	bigint_t dgval;
	bigint_t base;
	bigint_t tprod;

	bp = lex->ibp;

	bigint_init(&value, 0);
	bigint_init(&base, 10);

	while (is_digit(*bp)) {
		bigint_mul(&value, &base, &tprod);
		bigint_init(&dgval, digit_value(*bp));

		bigint_destroy(&value);
		bigint_add(&tprod, &dgval, &value);
		bigint_destroy(&tprod);
		bigint_destroy(&dgval);

		++bp;
	}

	bigint_destroy(&base);

	lex->ibp = bp;

	lex->current.lclass = lc_lit_int;
	bigint_shallow_copy(&value, &lex->current.u.lit_int.value);
}

/** Lex a string literal.
 *
 * Reads in a string literal and stores it in the current lem in @a lex.
 *
 * @param lex		Lexer object.
 */
static void lex_string(lex_t *lex)
{
	lex_char_string_core(lex, cs_str);

	lex->current.lclass = lc_lit_string;
	lex->current.u.lit_string.value = os_str_dup(strlit_buf);
}

static void lex_char_string_core(lex_t *lex, chr_str_t cs)
{
	char *bp;
	int sidx, didx;
	char term;
	const char *descr, *cap_descr;
	char spchar;

	/* Make compiler happy */
	term = '\0';
	descr = NULL;
	cap_descr = NULL;

	switch (cs) {
	case cs_chr:
		term = '\'';
		descr = "character";
		cap_descr = "Character";
		break;
	case cs_str:
		term = '"';
		descr = "string";
		cap_descr = "String";
		break;
	}

	bp = lex->ibp + 1;
	sidx = didx = 0;

	while (bp[sidx] != term) {
		if (didx >= SLBUF_SIZE) {
			printf("Error: %s literal too long.\n", cap_descr);
			exit(1);
		}

		if (bp[sidx] == '\0') {
			printf("Error: Unterminated %s literal.\n", descr);
			exit(1);
		}

		if (bp[sidx] == '\\') {
			switch (bp[sidx + 1]) {
			case '\\':
				spchar = '\\';
				break;
			case '\'':
				spchar = '\'';
				break;
			case '"':
				spchar = '"';
				break;
			case 'n':
				spchar = '\n';
				break;
			case 't':
				spchar = '\t';
				break;
			default:
				printf("Error: Unknown character escape sequence.\n");
				exit(1);
			}

			strlit_buf[didx] = spchar;
			++didx;
			sidx += 2;
		} else {
			strlit_buf[didx] = bp[sidx];
			++sidx; ++didx;
		}
	}

	lex->ibp = bp + sidx + 1;

	strlit_buf[didx] = '\0';
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
	errno_t rc;

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
