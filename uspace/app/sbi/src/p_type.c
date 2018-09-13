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
#include "cspan.h"
#include "debug.h"
#include "lex.h"
#include "list.h"
#include "mytypes.h"
#include "parse.h"
#include "p_expr.h"
#include "stree.h"

#include "p_type.h"

static stree_texpr_t *parse_tapply(parse_t *parse);
static stree_texpr_t *parse_tpostfix(parse_t *parse);
static stree_texpr_t *parse_pf_taccess(parse_t *parse, stree_texpr_t *a);
static stree_texpr_t *parse_pf_tindex(parse_t *parse, stree_texpr_t *a);
static stree_texpr_t *parse_tparen(parse_t *parse);
static stree_texpr_t *parse_tprimitive(parse_t *parse);
static stree_texpr_t *parse_tliteral(parse_t *parse);
static stree_texpr_t *parse_tnameref(parse_t *parse);

static stree_texpr_t *parse_recovery_texpr(parse_t *parse);

/** Parse type expression.
 *
 * Input is read from the input object associated with @a parse. If any
 * error occurs, parse->error will @c b_true when this function
 * returns. parse->error_bailout will be @c b_true if the error has not
 * been recovered yet. Similar holds for other parsing functions in this
 * module.
 *
 * @param parse		Parser object.
 */
stree_texpr_t *parse_texpr(parse_t *parse)
{
#ifdef DEBUG_PARSE_TRACE
	printf("Parse type expression.\n");
#endif
	if (parse_is_error(parse))
		return parse_recovery_texpr(parse);

	return parse_tapply(parse);
}

/** Parse type application expression.
 *
 * @param parse		Parser object.
 */
static stree_texpr_t *parse_tapply(parse_t *parse)
{
	stree_texpr_t *gtype;
	stree_texpr_t *aexpr;
	stree_texpr_t *targ;
	stree_tapply_t *tapply;

	gtype = parse_tpostfix(parse);
	if (lcur_lc(parse) != lc_slash)
		return gtype;

	tapply = stree_tapply_new();
	tapply->gtype = gtype;
	list_init(&tapply->targs);

	targ = NULL;

	while (lcur_lc(parse) == lc_slash) {

		if (parse_is_error(parse))
			break;

		lskip(parse);
		targ = parse_tpostfix(parse);

		list_append(&tapply->targs, targ);
	}

	aexpr = stree_texpr_new(tc_tapply);
	aexpr->u.tapply = tapply;
	tapply->texpr = aexpr;

	if (targ != NULL)
		aexpr->cspan = cspan_merge(gtype->cspan, targ->cspan);
	else
		aexpr->cspan = gtype->cspan;

	return aexpr;
}

/** Parse postfix type expression.
 *
 * @param parse		Parser object.
 */
static stree_texpr_t *parse_tpostfix(parse_t *parse)
{
	stree_texpr_t *a;
	stree_texpr_t *tmp;

	a = parse_tparen(parse);

	while (lcur_lc(parse) == lc_period || lcur_lc(parse) == lc_lsbr) {

		if (parse_is_error(parse))
			break;

		switch (lcur_lc(parse)) {
		case lc_period:
			tmp = parse_pf_taccess(parse, a);
			break;
		case lc_lsbr:
			tmp = parse_pf_tindex(parse, a);
			break;
		default:
			lunexpected_error(parse);
			tmp = parse_recovery_texpr(parse);
			break;
		}

		a = tmp;
	}

	return a;
}

/** Parse access type expression.
 *
 * @param parse		Parser object.
 * @param a		Base expression.
 */
static stree_texpr_t *parse_pf_taccess(parse_t *parse, stree_texpr_t *a)
{
	stree_texpr_t *texpr;
	stree_ident_t *ident;
	stree_taccess_t *taccess;

	lmatch(parse, lc_period);
	ident = parse_ident(parse);

	taccess = stree_taccess_new();
	taccess->arg = a;
	taccess->member_name = ident;

	texpr = stree_texpr_new(tc_taccess);
	texpr->u.taccess = taccess;
	taccess->texpr = texpr;
	texpr->cspan = cspan_merge(a->cspan, ident->cspan);

	return texpr;
}

/** Parse index type expression.
 *
 * @param parse		Parser object.
 * @param a		Base expression.
 */
static stree_texpr_t *parse_pf_tindex(parse_t *parse, stree_texpr_t *a)
{
	stree_texpr_t *texpr;
	stree_tindex_t *tindex;
	stree_expr_t *expr;
	cspan_t *cs1;

	tindex = stree_tindex_new();
	tindex->base_type = a;

	tindex->n_args = 0;
	list_init(&tindex->args);

	lmatch(parse, lc_lsbr);

	if (lcur_lc(parse) != lc_rsbr && lcur_lc(parse) != lc_comma) {
		while (b_true) {
			if (parse_is_error(parse))
				break;
			expr = parse_expr(parse);
			tindex->n_args += 1;
			list_append(&tindex->args, expr);

			if (lcur_lc(parse) == lc_rsbr)
				break;

			lmatch(parse, lc_comma);
		}
	} else {
		tindex->n_args = 1;
		while (lcur_lc(parse) == lc_comma) {
			if (parse_is_error(parse))
				break;
			lskip(parse);
			tindex->n_args += 1;
		}
	}

	lmatch(parse, lc_rsbr);
	cs1 = lprev_span(parse);

	texpr = stree_texpr_new(tc_tindex);
	texpr->u.tindex = tindex;
	tindex->texpr = texpr;
	texpr->cspan = cspan_merge(a->cspan, cs1);

	return texpr;
}

/** Parse possibly partenthesized type expression.
 *
 * @param parse		Parser object.
 */
static stree_texpr_t *parse_tparen(parse_t *parse)
{
	stree_texpr_t *texpr;
	cspan_t *cs0, *cs1;

	if (lcur_lc(parse) == lc_lparen) {
		cs0 = lcur_span(parse);
		lskip(parse);
		texpr = parse_texpr(parse);
		lmatch(parse, lc_rparen);
		cs1 = lprev_span(parse);
		texpr->cspan = cspan_merge(cs0, cs1);
	} else {
		texpr = parse_tprimitive(parse);
	}

	return texpr;
}

/** Parse primitive type expression.
 *
 * @param parse		Parser object.
 */
static stree_texpr_t *parse_tprimitive(parse_t *parse)
{
	stree_texpr_t *texpr;

	switch (lcur_lc(parse)) {
	case lc_ident:
		texpr = parse_tnameref(parse);
		break;
	case lc_bool:
	case lc_char:
	case lc_int:
	case lc_string:
	case lc_resource:
		texpr = parse_tliteral(parse);
		break;
	default:
		lunexpected_error(parse);
		texpr = parse_recovery_texpr(parse);
		break;
	}

	return texpr;
}

/** Parse type literal.
 *
 * @param parse		Parser object.
 */
static stree_texpr_t *parse_tliteral(parse_t *parse)
{
	stree_tliteral_t *tliteral;
	tliteral_class_t tlc;
	stree_texpr_t *texpr;

	switch (lcur_lc(parse)) {
	case lc_bool:
		tlc = tlc_bool;
		break;
	case lc_char:
		tlc = tlc_char;
		break;
	case lc_int:
		tlc = tlc_int;
		break;
	case lc_string:
		tlc = tlc_string;
		break;
	case lc_resource:
		tlc = tlc_resource;
		break;
	default:
		assert(b_false);
	}

	lskip(parse);

	tliteral = stree_tliteral_new(tlc);
	texpr = stree_texpr_new(tc_tliteral);
	texpr->u.tliteral = tliteral;
	tliteral->texpr = texpr;
	texpr->cspan = lprev_span(parse);

	return texpr;
}

/** Parse type identifier.
 *
 * @param parse		Parser object.
 */
static stree_texpr_t *parse_tnameref(parse_t *parse)
{
	stree_tnameref_t *tnameref;
	stree_texpr_t *texpr;

	tnameref = stree_tnameref_new();
	tnameref->name = parse_ident(parse);

	texpr = stree_texpr_new(tc_tnameref);
	texpr->u.tnameref = tnameref;
	tnameref->texpr = texpr;
	texpr->cspan = tnameref->name->cspan;

	return texpr;
}

/** Construct a special type expression fore recovery.
 *
 * @param parse		Parser object.
 */
static stree_texpr_t *parse_recovery_texpr(parse_t *parse)
{
	stree_tliteral_t *tliteral;
	stree_texpr_t *texpr;

	(void) parse;

	tliteral = stree_tliteral_new(tlc_int);

	texpr = stree_texpr_new(tc_tliteral);
	texpr->u.tliteral = tliteral;
	tliteral->texpr = texpr;

	return texpr;
}
