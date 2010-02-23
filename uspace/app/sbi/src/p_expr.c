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

/** @file Parse arithmetic expressions. */

#include <assert.h>
#include <stdlib.h>
#include "lex.h"
#include "list.h"
#include "mytypes.h"
#include "parse.h"
#include "stree.h"

#include "p_expr.h"

static stree_expr_t *parse_assign(parse_t *parse);
static stree_expr_t *parse_comparative(parse_t *parse);
static stree_expr_t *parse_additive(parse_t *parse);
static stree_expr_t *parse_postfix(parse_t *parse);
static stree_expr_t *parse_pf_access(parse_t *parse, stree_expr_t *a);
static stree_expr_t *parse_pf_call(parse_t *parse, stree_expr_t *a);
static stree_expr_t *parse_primitive(parse_t *parse);
static stree_expr_t *parse_nameref(parse_t *parse);
static stree_expr_t *parse_lit_int(parse_t *parse);
static stree_expr_t *parse_lit_string(parse_t *parse);

/** Parse expression. */
stree_expr_t *parse_expr(parse_t *parse)
{
	return parse_assign(parse);
}

/** Parse assignment expression. */
static stree_expr_t *parse_assign(parse_t *parse)
{
	stree_expr_t *a, *b, *tmp;
	stree_assign_t *assign;

	a = parse_comparative(parse);

	switch (lcur_lc(parse)) {
	case lc_assign:
		assign = stree_assign_new(ac_set);
		break;
	case lc_increase:
		assign = stree_assign_new(ac_increase);
		break;
	default:
		return a;
	}

	lskip(parse);
	b = parse_comparative(parse);

	assign->dest = a;
	assign->src = b;

	tmp = stree_expr_new(ec_assign);
	tmp->u.assign = assign;
	return tmp;
}

/** Parse comparative expression. */
static stree_expr_t *parse_comparative(parse_t *parse)
{
	stree_expr_t *a, *b, *tmp;
	stree_binop_t *binop;
	binop_class_t bc;

	a = parse_additive(parse);

	while (lcur_lc(parse) == lc_equal || lcur_lc(parse) == lc_notequal ||
	    lcur_lc(parse) == lc_lt || lcur_lc(parse) == lc_gt ||
	    lcur_lc(parse) == lc_lt_equal || lcur_lc(parse) == lc_gt_equal) {

		switch (lcur_lc(parse)) {
		case lc_equal: bc = bo_equal; break;
		case lc_notequal: bc = bo_notequal; break;
		case lc_lt: bc = bo_lt; break;
		case lc_gt: bc = bo_gt; break;
		case lc_lt_equal: bc = bo_lt_equal; break;
		case lc_gt_equal: bc = bo_gt_equal; break;
		default: assert(b_false);
		}

		lskip(parse);
		b = parse_additive(parse);

		binop = stree_binop_new(bc);
		binop->arg1 = a;
		binop->arg2 = b;

		tmp = stree_expr_new(ec_binop);
		tmp->u.binop = binop;
		a = tmp;
	}

	return a;
}

/** Parse additive expression. */
static stree_expr_t *parse_additive(parse_t *parse)
{
	stree_expr_t *a, *b, *tmp;
	stree_binop_t *binop;

	a = parse_postfix(parse);
	while (lcur_lc(parse) == lc_plus) {
		lskip(parse);
		b = parse_postfix(parse);

		binop = stree_binop_new(bo_plus);
		binop->arg1 = a;
		binop->arg2 = b;

		tmp = stree_expr_new(ec_binop);
		tmp->u.binop = binop;
		a = tmp;
	}

	return a;
}

/** Parse postfix expression. */
static stree_expr_t *parse_postfix(parse_t *parse)
{
	stree_expr_t *a;
	stree_expr_t *tmp;

	a = parse_primitive(parse);

	while (lcur_lc(parse) == lc_period || lcur_lc(parse) == lc_lparen) {

		switch (lcur_lc(parse)) {
		case lc_period:
			tmp = parse_pf_access(parse, a);
			break;
		case lc_lparen:
			tmp = parse_pf_call(parse, a);
			break;
		default:
			assert(b_false);
		}

		a = tmp;
	}

	return a;
}

/** Parse member access expression */
static stree_expr_t *parse_pf_access(parse_t *parse, stree_expr_t *a)
{
	stree_ident_t *ident;
	stree_expr_t *expr;
	stree_access_t *access;

	lmatch(parse, lc_period);
	ident = parse_ident(parse);

	access = stree_access_new();
	access->arg = a;
	access->member_name = ident;

	expr = stree_expr_new(ec_access);
	expr->u.access = access;

	return expr;
}

/** Parse function call expression. */
static stree_expr_t *parse_pf_call(parse_t *parse, stree_expr_t *a)
{
	stree_expr_t *expr;
	stree_call_t *call;
	stree_expr_t *arg;

	lmatch(parse, lc_lparen);

	call = stree_call_new();
	call->fun = a;
	list_init(&call->args);

	/* Parse function arguments */

	if (lcur_lc(parse) != lc_rparen) {
		while (b_true) {
			arg = parse_expr(parse);
			list_append(&call->args, arg);

			if (lcur_lc(parse) == lc_rparen)
				break;
			lmatch(parse, lc_comma);
		}
	}

	lmatch(parse, lc_rparen);

	expr = stree_expr_new(ec_call);
	expr->u.call = call;

	return expr;
}

/** Parse primitive expression. */
static stree_expr_t *parse_primitive(parse_t *parse)
{
	stree_expr_t *expr;

	switch (lcur_lc(parse)) {
	case lc_ident:
		expr = parse_nameref(parse);
		break;
	case lc_lit_int:
		expr = parse_lit_int(parse);
		break;
	case lc_lit_string:
		expr = parse_lit_string(parse);
		break;
	default:
		lunexpected_error(parse);
		exit(1);
	}

	return expr;
}

/** Parse name reference. */
static stree_expr_t *parse_nameref(parse_t *parse)
{
	stree_nameref_t *nameref;
	stree_expr_t *expr;

	nameref = stree_nameref_new();
	nameref->name = parse_ident(parse);
	expr = stree_expr_new(ec_nameref);
	expr->u.nameref = nameref;

	return expr;
}

/** Parse integer literal. */
static stree_expr_t *parse_lit_int(parse_t *parse)
{
	stree_literal_t *literal;
	stree_expr_t *expr;

	lcheck(parse, lc_lit_int);

	literal = stree_literal_new(ltc_int);
	literal->u.lit_int.value = lcur(parse)->u.lit_int.value;

	lskip(parse);

	expr = stree_expr_new(ec_literal);
	expr->u.literal = literal;

	return expr;
}

/** Parse string literal. */
static stree_expr_t *parse_lit_string(parse_t *parse)
{
	stree_literal_t *literal;
	stree_expr_t *expr;

	lcheck(parse, lc_lit_string);

	literal = stree_literal_new(ltc_string);
	literal->u.lit_string.value = lcur(parse)->u.lit_string.value;

	lskip(parse);

	expr = stree_expr_new(ec_literal);
	expr->u.literal = literal;

	return expr;
}
