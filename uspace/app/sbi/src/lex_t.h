/*
 * SPDX-FileCopyrightText: 2011 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef LEX_T_H_
#define LEX_T_H_

#include "bigint_t.h"

/** Lexical element class */
typedef enum {
	lc_invalid,
	lc_eof,

	lc_ident,
	lc_lit_char,
	lc_lit_int,
	lc_lit_string,

	/* Keywords */
	lc_and,
	lc_as,
	lc_break,
	lc_bool,
	lc_builtin,
	lc_char,
	lc_class,
	lc_deleg,
	lc_do,
	lc_elif,
	lc_else,
	lc_end,
	lc_enum,
	lc_except,
	lc_false,
	lc_finally,
	lc_for,
	lc_fun,
	lc_new,
	lc_get,
	lc_if,
	lc_in,
	lc_int,
	lc_interface,
	lc_is,
	lc_nil,
	lc_not,
	lc_or,
	lc_override,
	lc_packed,
	lc_private,
	lc_prop,
	lc_protected,
	lc_public,
	lc_raise,
	lc_resource,
	lc_return,
	lc_self,
	lc_set,
	lc_static,
	lc_string,
	lc_struct,
	lc_switch,
	lc_then,
	lc_this,
	lc_true,
	lc_var,
	lc_with,
	lc_when,
	lc_while,
	lc_yield,

	/* Operators */
	lc_period,
	lc_slash,
	lc_lparen,
	lc_rparen,
	lc_lsbr,
	lc_rsbr,
	lc_equal,
	lc_notequal,
	lc_lt,
	lc_gt,
	lc_lt_equal,
	lc_gt_equal,
	lc_assign,
	lc_plus,
	lc_minus,
	lc_mult,
	lc_increase,

	/* Punctuators */
	lc_comma,
	lc_colon,
	lc_scolon,

	lc__limit
} lclass_t;

typedef struct {
	/* String ID */
	int sid;
} lem_ident_t;

typedef struct {
	/* Character value */
	bigint_t value;
} lem_lit_char_t;

typedef struct {
	/* Integer value */
	bigint_t value;
} lem_lit_int_t;

typedef struct {
	/* String value */
	char *value;
} lem_lit_string_t;

/** Lexical element */
typedef struct {
	/* Lexical element class */
	lclass_t lclass;

	union {
		lem_ident_t ident;
		lem_lit_char_t lit_char;
		lem_lit_int_t lit_int;
		lem_lit_string_t lit_string;
	} u;

	/** Coordinates of this lexical element */
	struct cspan *cspan;
} lem_t;

/** Lexer state object */
typedef struct lex {
	/** Input object */
	struct input *input;

	/** Lexing buffer */
	char *inbuf;

	/** Pointer to current position in lexing buffer */
	char *ibp;

	/** Number of the line currently in inbuf */
	int ib_line;

	/** Column number adjustment (due to tabs) */
	int col_adj;

	/** @c b_true if we have the previous lem in @c prev */
	bool_t prev_valid;

	/** Previous lem (only valid if @c current_valid is true) */
	lem_t prev;

	/** @c b_true if we have the current lem in @c current */
	bool_t current_valid;

	/** Curent lem (only valid if @c current_valid is true) */
	lem_t current;
} lex_t;

#endif
