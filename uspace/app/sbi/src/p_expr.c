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
#include "bigint.h"
#include "cspan.h"
#include "debug.h"
#include "lex.h"
#include "list.h"
#include "mytypes.h"
#include "p_type.h"
#include "parse.h"
#include "stree.h"

#include "p_expr.h"

static stree_expr_t *parse_assign(parse_t *parse);
static stree_expr_t *parse_disjunctive(parse_t *parse);
static stree_expr_t *parse_conjunctive(parse_t *parse);
static stree_expr_t *parse_comparative(parse_t *parse);
static stree_expr_t *parse_additive(parse_t *parse);
static stree_expr_t *parse_multip(parse_t *parse);
static stree_expr_t *parse_prefix(parse_t *parse);
static stree_expr_t *parse_prefix_new(parse_t *parse);
static stree_expr_t *parse_postfix(parse_t *parse);
static stree_expr_t *parse_pf_access(parse_t *parse, stree_expr_t *a);
static stree_expr_t *parse_pf_call(parse_t *parse, stree_expr_t *a);
static stree_expr_t *parse_pf_index(parse_t *parse, stree_expr_t *a);
static stree_expr_t *parse_pf_as(parse_t *parse, stree_expr_t *a);
static stree_expr_t *parse_paren(parse_t *parse);
static stree_expr_t *parse_primitive(parse_t *parse);
static stree_expr_t *parse_nameref(parse_t *parse);
static stree_expr_t *parse_lit_bool(parse_t *parse);
static stree_expr_t *parse_lit_char(parse_t *parse);
static stree_expr_t *parse_lit_int(parse_t *parse);
static stree_expr_t *parse_lit_ref(parse_t *parse);
static stree_expr_t *parse_lit_string(parse_t *parse);
static stree_expr_t *parse_self_ref(parse_t *parse);

static stree_expr_t *parse_recovery_expr(parse_t *parse);

/** Parse expression.
 *
 * Input is read from the input object associated with @a parse. If any
 * error occurs, parse->error will @c b_true when this function
 * returns. parse->error_bailout will be @c b_true if the error has not
 * been recovered yet. Similar holds for other parsing functions in this
 * module.
 *
 * @param parse		Parser object.
 */
stree_expr_t *parse_expr(parse_t *parse)
{
#ifdef DEBUG_PARSE_TRACE
	printf("Parse expression.\n");
#endif
	if (parse_is_error(parse))
		return parse_recovery_expr(parse);

	return parse_assign(parse);
}

/** Parse assignment expression.
 *
 * @param parse		Parser object.
 */
static stree_expr_t *parse_assign(parse_t *parse)
{
	stree_expr_t *a, *b, *tmp;
	stree_assign_t *assign;

	a = parse_disjunctive(parse);

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
	b = parse_disjunctive(parse);

	assign->dest = a;
	assign->src = b;

	tmp = stree_expr_new(ec_assign);
	tmp->u.assign = assign;
	tmp->cspan = cspan_merge(a->cspan, b->cspan);

	assign->expr = tmp;

	return tmp;
}

/** Parse disjunctive expression.
 *
 * @param parse		Parser object.
 */
static stree_expr_t *parse_disjunctive(parse_t *parse)
{
	stree_expr_t *a, *b, *tmp;
	stree_binop_t *binop;
	cspan_t *cs;

	a = parse_conjunctive(parse);
	cs = a->cspan;

	while (lcur_lc(parse) == lc_or) {
		if (parse_is_error(parse))
			break;

		lskip(parse);
		b = parse_conjunctive(parse);

		binop = stree_binop_new(bo_or);
		binop->arg1 = a;
		binop->arg2 = b;

		tmp = stree_expr_new(ec_binop);
		tmp->u.binop = binop;
		tmp->cspan = cspan_merge(cs, b->cspan);
		binop->expr = tmp;

		a = tmp;
		cs = tmp->cspan;
	}

	return a;
}

/** Parse conjunctive expression.
 *
 * @param parse		Parser object.
 */
static stree_expr_t *parse_conjunctive(parse_t *parse)
{
	stree_expr_t *a, *b, *tmp;
	stree_binop_t *binop;
	cspan_t *cs;

	a = parse_comparative(parse);
	cs = a->cspan;

	while (lcur_lc(parse) == lc_and) {
		if (parse_is_error(parse))
			break;

		lskip(parse);
		b = parse_comparative(parse);

		binop = stree_binop_new(bo_and);
		binop->arg1 = a;
		binop->arg2 = b;

		tmp = stree_expr_new(ec_binop);
		tmp->u.binop = binop;
		tmp->cspan = cspan_merge(cs, b->cspan);
		binop->expr = tmp;

		a = tmp;
		cs = tmp->cspan;
	}

	return a;
}

/** Parse comparative expression.
 *
 * @param parse		Parser object.
 */
static stree_expr_t *parse_comparative(parse_t *parse)
{
	stree_expr_t *a, *b, *tmp;
	stree_binop_t *binop;
	binop_class_t bc;
	cspan_t *cs;

	a = parse_additive(parse);
	cs = a->cspan;

	while (lcur_lc(parse) == lc_equal || lcur_lc(parse) == lc_notequal ||
	    lcur_lc(parse) == lc_lt || lcur_lc(parse) == lc_gt ||
	    lcur_lc(parse) == lc_lt_equal || lcur_lc(parse) == lc_gt_equal) {

		if (parse_is_error(parse))
			break;

		switch (lcur_lc(parse)) {
		case lc_equal:
			bc = bo_equal;
			break;
		case lc_notequal:
			bc = bo_notequal;
			break;
		case lc_lt:
			bc = bo_lt;
			break;
		case lc_gt:
			bc = bo_gt;
			break;
		case lc_lt_equal:
			bc = bo_lt_equal;
			break;
		case lc_gt_equal:
			bc = bo_gt_equal;
			break;
		default:
			assert(b_false);
		}

		lskip(parse);
		b = parse_additive(parse);

		binop = stree_binop_new(bc);
		binop->arg1 = a;
		binop->arg2 = b;

		tmp = stree_expr_new(ec_binop);
		tmp->u.binop = binop;
		tmp->cspan = cspan_merge(cs, b->cspan);
		binop->expr = tmp;

		a = tmp;
		cs = tmp->cspan;
	}

	return a;
}

/** Parse additive expression.
 *
 * @param parse		Parser object.
 */
static stree_expr_t *parse_additive(parse_t *parse)
{
	stree_expr_t *a, *b, *tmp;
	stree_binop_t *binop;
	binop_class_t bc;
	cspan_t *cs;

	a = parse_multip(parse);
	cs = a->cspan;

	while (lcur_lc(parse) == lc_plus || lcur_lc(parse) == lc_minus) {
		if (parse_is_error(parse))
			break;

		switch (lcur_lc(parse)) {
		case lc_plus:
			bc = bo_plus;
			break;
		case lc_minus:
			bc = bo_minus;
			break;
		default:
			assert(b_false);
		}

		lskip(parse);
		b = parse_multip(parse);

		binop = stree_binop_new(bc);
		binop->arg1 = a;
		binop->arg2 = b;

		tmp = stree_expr_new(ec_binop);
		tmp->u.binop = binop;
		tmp->cspan = cspan_merge(cs, b->cspan);
		binop->expr = tmp;

		a = tmp;
		cs = tmp->cspan;
	}

	return a;
}

/** Parse multiplicative expression.
 *
 * @param parse		Parser object.
 */
static stree_expr_t *parse_multip(parse_t *parse)
{
	stree_expr_t *a, *b, *tmp;
	stree_binop_t *binop;
	binop_class_t bc;
	cspan_t *cs;

	a = parse_prefix(parse);
	cs = a->cspan;

	while (lcur_lc(parse) == lc_mult) {
		if (parse_is_error(parse))
			break;

		switch (lcur_lc(parse)) {
		case lc_mult:
			bc = bo_mult;
			break;
		default:
			assert(b_false);
		}

		lskip(parse);
		b = parse_prefix(parse);

		binop = stree_binop_new(bc);
		binop->arg1 = a;
		binop->arg2 = b;

		tmp = stree_expr_new(ec_binop);
		tmp->u.binop = binop;
		tmp->cspan = cspan_merge(cs, b->cspan);
		binop->expr = tmp;

		a = tmp;
		cs = tmp->cspan;
	}

	return a;
}

/** Parse prefix expression.
 *
 * @param parse		Parser object.
 */
static stree_expr_t *parse_prefix(parse_t *parse)
{
	stree_expr_t *a;
	stree_expr_t *tmp;
	stree_unop_t *unop;
	unop_class_t uc;
	cspan_t *cs0;

	switch (lcur_lc(parse)) {
	case lc_plus:
	case lc_minus:
	case lc_not:
		if (parse_is_error(parse))
			return parse_recovery_expr(parse);

		switch (lcur_lc(parse)) {
		case lc_plus:
			uc = uo_plus;
			break;
		case lc_minus:
			uc = uo_minus;
			break;
		case lc_not:
			uc = uo_not;
			break;
		default:
			assert(b_false);
		}

		cs0 = lcur_span(parse);
		lskip(parse);
		a = parse_postfix(parse);

		unop = stree_unop_new(uc);
		unop->arg = a;

		tmp = stree_expr_new(ec_unop);
		tmp->u.unop = unop;
		tmp->cspan = cspan_merge(cs0, a->cspan);
		unop->expr = tmp;
		a = tmp;
		break;
	case lc_new:
		a = parse_prefix_new(parse);
		break;
	default:
		a = parse_postfix(parse);
		break;
	}

	return a;
}

/** Parse @c new operator.
 *
 * @param parse		Parser object.
 */
static stree_expr_t *parse_prefix_new(parse_t *parse)
{
	stree_texpr_t *texpr;
	stree_new_t *new_op;
	stree_expr_t *expr;
	stree_expr_t *arg;
	cspan_t *cs0, *cs1;

	cs0 = lcur_span(parse);
	lmatch(parse, lc_new);
	texpr = parse_texpr(parse);

	/* XXX Take span from texpr */
	cs1 = lprev_span(parse);

	new_op = stree_new_new();
	new_op->texpr = texpr;
	expr = stree_expr_new(ec_new);
	expr->u.new_op = new_op;

	list_init(&new_op->ctor_args);

	/* Parenthesized arguments should be present except for arrays. */
	if (texpr->tc != tc_tindex) {
		lmatch(parse, lc_lparen);

		/* Parse constructor arguments */

		if (lcur_lc(parse) != lc_rparen) {
			while (!parse_is_error(parse)) {
				arg = parse_expr(parse);
				list_append(&new_op->ctor_args, arg);

				if (lcur_lc(parse) == lc_rparen)
					break;
				lmatch(parse, lc_comma);
			}
		}

		lmatch(parse, lc_rparen);
		cs1 = cspan_merge(cs0, lprev_span(parse));
	}

	expr->cspan = cspan_merge(cs0, cs1);
	new_op->expr = expr;

	return expr;
}

/** Parse postfix expression.
 *
 * @param parse		Parser object.
 */
static stree_expr_t *parse_postfix(parse_t *parse)
{
	stree_expr_t *a;
	stree_expr_t *tmp;

	a = parse_paren(parse);

	while (lcur_lc(parse) == lc_period || lcur_lc(parse) == lc_lparen ||
	    lcur_lc(parse) == lc_lsbr || lcur_lc(parse) == lc_as) {

		if (parse_is_error(parse))
			break;

		switch (lcur_lc(parse)) {
		case lc_period:
			tmp = parse_pf_access(parse, a);
			break;
		case lc_lparen:
			tmp = parse_pf_call(parse, a);
			break;
		case lc_lsbr:
			tmp = parse_pf_index(parse, a);
			break;
		case lc_as:
			tmp = parse_pf_as(parse, a);
			break;
		default:
			assert(b_false);
		}

		a = tmp;
	}

	return a;
}

/** Parse member access expression
 *
 * @param parse		Parser object.
 */
static stree_expr_t *parse_pf_access(parse_t *parse, stree_expr_t *a)
{
	stree_ident_t *ident;
	stree_expr_t *expr;
	stree_access_t *access;
	cspan_t *cs1;

	lmatch(parse, lc_period);
	ident = parse_ident(parse);

	/* XXX Take span from ident */
	cs1 = lprev_span(parse);

	access = stree_access_new();
	access->arg = a;
	access->member_name = ident;

	expr = stree_expr_new(ec_access);
	expr->u.access = access;
	expr->cspan = cspan_merge(a->cspan, cs1);

	access->expr = expr;

	return expr;
}

/** Parse function call expression.
 *
 * @param parse		Parser object.
 */
static stree_expr_t *parse_pf_call(parse_t *parse, stree_expr_t *a)
{
	stree_expr_t *expr;
	stree_call_t *call;
	stree_expr_t *arg;
	cspan_t *cs1;

	lmatch(parse, lc_lparen);

	call = stree_call_new();
	call->fun = a;
	list_init(&call->args);

	/* Parse function arguments */

	if (lcur_lc(parse) != lc_rparen) {
		while (!parse_is_error(parse)) {
			arg = parse_expr(parse);
			list_append(&call->args, arg);

			if (lcur_lc(parse) == lc_rparen)
				break;
			lmatch(parse, lc_comma);
		}
	}

	lmatch(parse, lc_rparen);
	cs1 = lprev_span(parse);

	expr = stree_expr_new(ec_call);
	expr->u.call = call;
	expr->cspan = cspan_merge(a->cspan, cs1);
	call->expr = expr;

	return expr;
}

/** Parse index expression.
 *
 * @param parse		Parser object.
 */
static stree_expr_t *parse_pf_index(parse_t *parse, stree_expr_t *a)
{
	stree_expr_t *expr;
	stree_index_t *index;
	stree_expr_t *arg;
	cspan_t *cs1;

	lmatch(parse, lc_lsbr);

	index = stree_index_new();
	index->base = a;
	list_init(&index->args);

	/* Parse indices */

	if (lcur_lc(parse) != lc_rsbr) {
		while (!parse_is_error(parse)) {
			arg = parse_expr(parse);
			list_append(&index->args, arg);

			if (lcur_lc(parse) == lc_rsbr)
				break;
			lmatch(parse, lc_comma);
		}
	}

	lmatch(parse, lc_rsbr);
	cs1 = lprev_span(parse);

	expr = stree_expr_new(ec_index);
	expr->u.index = index;
	expr->cspan = cspan_merge(a->cspan, cs1);
	index->expr = expr;

	return expr;
}

/** Parse @c as operator.
 *
 * @param parse		Parser object.
 */
static stree_expr_t *parse_pf_as(parse_t *parse, stree_expr_t *a)
{
	stree_expr_t *expr;
	stree_texpr_t *texpr;
	stree_as_t *as_op;
	cspan_t *cs1;

	lmatch(parse, lc_as);
	texpr = parse_texpr(parse);

	/* XXX Take span from texpr */
	cs1 = lprev_span(parse);

	as_op = stree_as_new();
	as_op->arg = a;
	as_op->dtype = texpr;

	expr = stree_expr_new(ec_as);
	expr->u.as_op = as_op;
	expr->cspan = cspan_merge(a->cspan, cs1);

	as_op->expr = expr;

	return expr;
}

/** Parse possibly partenthesized expression.
 *
 * @param parse		Parser object.
 */
static stree_expr_t *parse_paren(parse_t *parse)
{
	stree_expr_t *expr;
	cspan_t *cs0, *cs1;

	if (lcur_lc(parse) == lc_lparen) {
		cs0 = lcur_span(parse);
		lskip(parse);
		expr = parse_expr(parse);
		lmatch(parse, lc_rparen);
		cs1 = lprev_span(parse);

		expr->cspan = cspan_merge(cs0, cs1);
	} else {
		expr = parse_primitive(parse);
	}

	return expr;
}

/** Parse primitive expression.
 *
 * @param parse		Parser object.
 */
static stree_expr_t *parse_primitive(parse_t *parse)
{
	stree_expr_t *expr;

	switch (lcur_lc(parse)) {
	case lc_ident:
		expr = parse_nameref(parse);
		break;
	case lc_false:
	case lc_true:
		expr = parse_lit_bool(parse);
		break;
	case lc_lit_char:
		expr = parse_lit_char(parse);
		break;
	case lc_lit_int:
		expr = parse_lit_int(parse);
		break;
	case lc_nil:
		expr = parse_lit_ref(parse);
		break;
	case lc_lit_string:
		expr = parse_lit_string(parse);
		break;
	case lc_self:
		expr = parse_self_ref(parse);
		break;
	default:
		lunexpected_error(parse);
		expr = parse_recovery_expr(parse);
	}

	return expr;
}

/** Parse name reference.
 *
 * @param parse		Parser object.
 */
static stree_expr_t *parse_nameref(parse_t *parse)
{
	stree_nameref_t *nameref;
	stree_expr_t *expr;

	nameref = stree_nameref_new();
	nameref->name = parse_ident(parse);
	expr = stree_expr_new(ec_nameref);
	expr->u.nameref = nameref;
	expr->cspan = lprev_span(parse);
	nameref->expr = expr;

	return expr;
}

/** Parse boolean literal.
 *
 * @param parse		Parser object.
 */
static stree_expr_t *parse_lit_bool(parse_t *parse)
{
	stree_literal_t *literal;
	stree_expr_t *expr;
	bool_t value;

	switch (lcur_lc(parse)) {
	case lc_false:
		value = b_false;
		break;
	case lc_true:
		value = b_true;
		break;
	default:
		assert(b_false);
	}

	lskip(parse);

	literal = stree_literal_new(ltc_bool);
	literal->u.lit_bool.value = value;

	expr = stree_expr_new(ec_literal);
	expr->u.literal = literal;
	expr->cspan = lprev_span(parse);
	literal->expr = expr;

	return expr;
}

/** Parse character literal.
 *
 * @param parse		Parser object.
 */
static stree_expr_t *parse_lit_char(parse_t *parse)
{
	stree_literal_t *literal;
	stree_expr_t *expr;

	lcheck(parse, lc_lit_char);

	literal = stree_literal_new(ltc_char);
	bigint_clone(&lcur(parse)->u.lit_char.value,
	    &literal->u.lit_char.value);

	lskip(parse);

	expr = stree_expr_new(ec_literal);
	expr->u.literal = literal;
	expr->cspan = lprev_span(parse);
	literal->expr = expr;

	return expr;
}

/** Parse integer literal.
 *
 * @param parse		Parser object.
 */
static stree_expr_t *parse_lit_int(parse_t *parse)
{
	stree_literal_t *literal;
	stree_expr_t *expr;

	lcheck(parse, lc_lit_int);

	literal = stree_literal_new(ltc_int);
	bigint_clone(&lcur(parse)->u.lit_int.value,
	    &literal->u.lit_int.value);

	lskip(parse);

	expr = stree_expr_new(ec_literal);
	expr->u.literal = literal;
	expr->cspan = lprev_span(parse);
	literal->expr = expr;

	return expr;
}

/** Parse reference literal (@c nil).
 *
 * @param parse		Parser object.
 */
static stree_expr_t *parse_lit_ref(parse_t *parse)
{
	stree_literal_t *literal;
	stree_expr_t *expr;

	lmatch(parse, lc_nil);

	literal = stree_literal_new(ltc_ref);

	expr = stree_expr_new(ec_literal);
	expr->u.literal = literal;
	expr->cspan = lprev_span(parse);
	literal->expr = expr;

	return expr;
}

/** Parse string literal.
 *
 * @param parse		Parser object.
 */
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
	expr->cspan = lprev_span(parse);
	literal->expr = expr;

	return expr;
}

/** Parse @c self keyword.
 *
 * @param parse		Parser object.
 */
static stree_expr_t *parse_self_ref(parse_t *parse)
{
	stree_self_ref_t *self_ref;
	stree_expr_t *expr;

	lmatch(parse, lc_self);

	self_ref = stree_self_ref_new();

	expr = stree_expr_new(ec_self_ref);
	expr->u.self_ref = self_ref;
	expr->cspan = lprev_span(parse);
	self_ref->expr = expr;

	return expr;
}

/** Construct a special recovery expression.
 *
 * @param parse		Parser object.
 */
static stree_expr_t *parse_recovery_expr(parse_t *parse)
{
	stree_literal_t *literal;
	stree_expr_t *expr;

	(void) parse;

	literal = stree_literal_new(ltc_ref);

	expr = stree_expr_new(ec_literal);
	expr->u.literal = literal;
	literal->expr = expr;

	return expr;
}
