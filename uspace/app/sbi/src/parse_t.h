/*
 * SPDX-FileCopyrightText: 2010 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef PARSE_T_H_
#define PARSE_T_H_

/** Parser state object */
typedef struct {
	/** Lexer object */
	struct lex *lex;

	/** Program being parsed */
	struct stree_program *program;

	/** Module currently being parsed */
	struct stree_module *cur_mod;

	/** @c b_true if an error occured. */
	bool_t error;

	/** @c b_true if bailing out due to an error. */
	bool_t error_bailout;
} parse_t;

#endif
