/*                              YI
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
#include "p_expr.h"
#include "stree.h"

#include "p_type.h"

static stree_texpr_t *parse_tapply(parse_t *parse);
static stree_texpr_t *parse_tpostfix(parse_t *parse);
static stree_texpr_t *parse_pf_taccess(parse_t *parse, stree_texpr_t *a);
static stree_texpr_t *parse_pf_tindex(parse_t *parse, stree_texpr_t *a);
static stree_texpr_t *parse_tprimitive(parse_t *parse);
static stree_tliteral_t *parse_tliteral(parse_t *parse);
static stree_tnameref_t *parse_tnameref(parse_t *parse);

/** Parse type expression. */
stree_texpr_t *parse_texpr(parse_t *parse)
{
	return parse_tapply(parse);
}

/** Parse type application expression. */
static stree_texpr_t *parse_tapply(parse_t *parse)
{
	stree_texpr_t *a, *b, *tmp;
	stree_tapply_t *tapply;

	a = parse_tpostfix(parse);
	while (lcur_lc(parse) == lc_slash) {
		lskip(parse);
		b = parse_tpostfix(parse);

		tapply = stree_tapply_new();
		tapply->gtype = a;
		tapply->targ = b;

		tmp = stree_texpr_new(tc_tapply);
		tmp->u.tapply = tapply;
		a = tmp;
	}

	return a;
}

/** Parse postfix type expression. */
static stree_texpr_t *parse_tpostfix(parse_t *parse)
{
	stree_texpr_t *a;
	stree_texpr_t *tmp;

	a = parse_tprimitive(parse);

	while (lcur_lc(parse) == lc_period || lcur_lc(parse) == lc_lsbr) {

		switch (lcur_lc(parse)) {
		case lc_period:
			tmp = parse_pf_taccess(parse, a);
			break;
		case lc_lsbr:
			tmp = parse_pf_tindex(parse, a);
			break;
		default:
			lunexpected_error(parse);
			exit(1);
		}

		a = tmp;
	}

	return a;
}

/** Parse access type expression. */
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

	return texpr;
}

/** Parse index type expression. */
static stree_texpr_t *parse_pf_tindex(parse_t *parse, stree_texpr_t *a)
{
	stree_texpr_t *texpr;
	stree_tindex_t *tindex;
	stree_expr_t *expr;

	tindex = stree_tindex_new();
	tindex->base_type = a;

	tindex->n_args = 0;
	list_init(&tindex->args);

	lmatch(parse, lc_lsbr);

	if (lcur_lc(parse) != lc_rsbr && lcur_lc(parse) != lc_comma) {
		while (b_true) {
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
			lskip(parse);
			tindex->n_args += 1;
		}
	}

	lmatch(parse, lc_rsbr);

	texpr = stree_texpr_new(tc_tindex);
	texpr->u.tindex = tindex;

	return texpr;
}

/** Parse primitive type expression. */
static stree_texpr_t *parse_tprimitive(parse_t *parse)
{
	stree_texpr_t *texpr;

	switch (lcur_lc(parse)) {
	case lc_ident:
		texpr = stree_texpr_new(tc_tnameref);
		texpr->u.tnameref = parse_tnameref(parse);
		break;
	case lc_int:
	case lc_string:
	case lc_resource:
		texpr = stree_texpr_new(tc_tliteral);
		texpr->u.tliteral = parse_tliteral(parse);
		break;
	default:
		lunexpected_error(parse);
		exit(1);
	}

	return texpr;
}

/** Parse type literal. */
static stree_tliteral_t *parse_tliteral(parse_t *parse)
{
	stree_tliteral_t *tliteral;
	tliteral_class_t tlc;

	switch (lcur_lc(parse)) {
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
	return tliteral;
}

/** Parse type identifier. */
static stree_tnameref_t *parse_tnameref(parse_t *parse)
{
	stree_tnameref_t *tnameref;

	tnameref = stree_tnameref_new();
	tnameref->name = parse_ident(parse);

	return tnameref;
}
