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

/** @file Evaluate type expressions. */

#include <assert.h>
#include <stdlib.h>
#include "cspan.h"
#include "debug.h"
#include "list.h"
#include "mytypes.h"
#include "stree.h"
#include "strtab.h"
#include "symbol.h"
#include "tdata.h"

#include "run_texpr.h"

static void run_taccess(stree_program_t *prog, stree_csi_t *ctx,
    stree_taccess_t *taccess, tdata_item_t **res);
static void run_tindex(stree_program_t *prog, stree_csi_t *ctx,
    stree_tindex_t *tindex, tdata_item_t **res);
static void run_tliteral(stree_program_t *prog, stree_csi_t *ctx,
    stree_tliteral_t *tliteral, tdata_item_t **res);
static void run_tnameref(stree_program_t *prog, stree_csi_t *ctx,
    stree_tnameref_t *tnameref, tdata_item_t **res);
static void run_tapply(stree_program_t *prog, stree_csi_t *ctx,
    stree_tapply_t *tapply, tdata_item_t **res);

/** Evaluate type expression.
 *
 * Evaluate type expression (this produces a type item). If a type error
 * occurs, the resulting type item is of class @c tic_ignore.
 *
 * @param prog		Program
 * @param ctx		Current CSI (context)
 * @param texpr		Type expression to evaluate
 * @param res		Place to store type result
 */
void run_texpr(stree_program_t *prog, stree_csi_t *ctx, stree_texpr_t *texpr,
    tdata_item_t **res)
{
	switch (texpr->tc) {
	case tc_taccess:
		run_taccess(prog, ctx, texpr->u.taccess, res);
		break;
	case tc_tindex:
		run_tindex(prog, ctx, texpr->u.tindex, res);
		break;
	case tc_tliteral:
		run_tliteral(prog, ctx, texpr->u.tliteral, res);
		break;
	case tc_tnameref:
		run_tnameref(prog, ctx, texpr->u.tnameref, res);
		break;
	case tc_tapply:
		run_tapply(prog, ctx, texpr->u.tapply, res);
		break;
	}
}

/** Evaluate type access expression.
 *
 * Evaluate operation per the type access ('.') operator.
 *
 * @param prog		Program
 * @param ctx		Current CSI (context)
 * @param taccess	Type access expression to evaluate
 * @param res		Place to store type result
 */
static void run_taccess(stree_program_t *prog, stree_csi_t *ctx,
    stree_taccess_t *taccess, tdata_item_t **res)
{
	stree_symbol_t *sym;
	tdata_item_t *targ_i = NULL;
	tdata_item_t *titem = NULL;
	tdata_object_t *tobject;
	tdata_deleg_t *tdeleg;
	stree_csi_t *base_csi;
	stree_deleg_t *deleg;
	stree_enum_t *enum_d;
	tdata_enum_t *tenum;

#ifdef DEBUG_RUN_TRACE
	printf("Evaluating type access operation.\n");
#endif
	/* Evaluate base type. */
	run_texpr(prog, ctx, taccess->arg, &targ_i);

	if (targ_i->tic == tic_ignore) {
		*res = tdata_item_new(tic_ignore);
		return;
	}

	if (targ_i->tic != tic_tobject) {
		cspan_print(taccess->texpr->cspan);
		printf(" Error: Using '.' with type which is not an "
		    "object.\n");
		*res = tdata_item_new(tic_ignore);
		return;
	}

	/* Get base CSI. */
	base_csi = targ_i->u.tobject->csi;

	sym = symbol_lookup_in_csi(prog, base_csi, taccess->member_name);
	if (sym == NULL) {
		cspan_print(taccess->member_name->cspan);
		printf(" Error: CSI '");
		symbol_print_fqn(csi_to_symbol(base_csi));
		printf("' has no member named '%s'.\n",
		    strtab_get_str(taccess->member_name->sid));
		*res = tdata_item_new(tic_ignore);
		return;
	}

	switch (sym->sc) {
	case sc_csi:
		/* Construct type item. */
		titem = tdata_item_new(tic_tobject);
		tobject = tdata_object_new();
		titem->u.tobject = tobject;

		tobject->static_ref = sn_nonstatic;
		tobject->csi = sym->u.csi;
		list_init(&tobject->targs);
		break;
	case sc_ctor:
		/* It is not possible to reference a constructor explicitly. */
		assert(b_false);
		/* Fallthrough */
	case sc_deleg:
		/* Fetch stored delegate type. */
		deleg = symbol_to_deleg(sym);
		assert(deleg != NULL);
		if (deleg->titem == NULL) {
			/*
			 * Prepare a partial delegate which will be completed
			 * later.
			 */
			titem = tdata_item_new(tic_tdeleg);
			tdeleg = tdata_deleg_new();
			titem->u.tdeleg = tdeleg;
			tdeleg->deleg = deleg;
			tdeleg->tsig = NULL;

			deleg->titem = titem;
		} else {
			titem = deleg->titem;
		}
		break;
	case sc_enum:
		/* Fetch stored enum type. */
		enum_d = symbol_to_enum(sym);
		assert(enum_d != NULL);
		if (enum_d->titem == NULL) {
			/*
			 * Prepare a partial enum whic will be completed
			 * later.
			 */
			titem = tdata_item_new(tic_tenum);
			tenum = tdata_enum_new();
			titem->u.tenum = tenum;
			tenum->enum_d = enum_d;
		} else {
			titem = enum_d->titem;
		}
		break;
	case sc_fun:
	case sc_var:
	case sc_prop:
		cspan_print(taccess->member_name->cspan);
		printf(" Error: Symbol '");
		symbol_print_fqn(sym);
		printf("' is not a type.\n");
		titem = tdata_item_new(tic_ignore);
		break;
	}

	*res = titem;
}

/** Evaluate type indexing expression.
 *
 * Evaluate operation per the type indexing ('[', ']') operator.
 * A type indexing operation may have extents specified or only rank
 * specified.
 *
 * @param prog		Program
 * @param ctx		Current CSI (context)
 * @param tindex	Type indexing expression to evaluate
 * @param res		Place to store type result
 */
static void run_tindex(stree_program_t *prog, stree_csi_t *ctx,
    stree_tindex_t *tindex, tdata_item_t **res)
{
	tdata_item_t *base_ti = NULL;
	tdata_item_t *titem;
	tdata_array_t *tarray;
	stree_expr_t *arg_expr;
	list_node_t *arg_node;

#ifdef DEBUG_RUN_TRACE
	printf("Evaluating type index operation.\n");
#endif
	/* Evaluate base type. */
	run_texpr(prog, ctx, tindex->base_type, &base_ti);

	if (base_ti->tic == tic_ignore) {
		*res = tdata_item_new(tic_ignore);
		return;
	}

	/* Construct type item. */
	titem = tdata_item_new(tic_tarray);
	tarray = tdata_array_new();
	titem->u.tarray = tarray;

	tarray->base_ti = base_ti;
	tarray->rank = tindex->n_args;

	/* Copy extents. */
	list_init(&tarray->extents);
	arg_node = list_first(&tindex->args);

	while (arg_node != NULL) {
		arg_expr = list_node_data(arg_node, stree_expr_t *);
		list_append(&tarray->extents, arg_expr);
		arg_node = list_next(&tindex->args, arg_node);
	}

	*res = titem;
}

/** Evaluate type literal expression.
 *
 * @param prog		Program
 * @param ctx		Current CSI (context)
 * @param tliteral	Type literal
 * @param res		Place to store type result
 */
static void run_tliteral(stree_program_t *prog, stree_csi_t *ctx,
    stree_tliteral_t *tliteral, tdata_item_t **res)
{
	tdata_item_t *titem;
	tdata_primitive_t *tprimitive;
	tprimitive_class_t tpc;

#ifdef DEBUG_RUN_TRACE
	printf("Evaluating type literal.\n");
#endif
	(void) prog;
	(void) ctx;
	(void) tliteral;

	/* Make compiler happy. */
	tpc = 0;

	switch (tliteral->tlc) {
	case tlc_bool:
		tpc = tpc_bool;
		break;
	case tlc_char:
		tpc = tpc_char;
		break;
	case tlc_int:
		tpc = tpc_int;
		break;
	case tlc_string:
		tpc = tpc_string;
		break;
	case tlc_resource:
		tpc = tpc_resource;
		break;
	}

	/* Construct type item. */
	titem = tdata_item_new(tic_tprimitive);
	tprimitive = tdata_primitive_new(tpc);
	titem->u.tprimitive = tprimitive;

	*res = titem;
}

static void run_tnameref(stree_program_t *prog, stree_csi_t *ctx,
    stree_tnameref_t *tnameref, tdata_item_t **res)
{
	stree_symbol_t *sym;
	tdata_item_t *titem;
	tdata_object_t *tobject;
	stree_targ_t *targ;
	tdata_vref_t *tvref;
	stree_deleg_t *deleg;
	tdata_deleg_t *tdeleg;
	stree_enum_t *enum_d;
	tdata_enum_t *tenum;

#ifdef DEBUG_RUN_TRACE
	printf("Evaluating type name reference.\n");
	printf("'%s'\n", strtab_get_str(tnameref->name->sid));
#endif
	/* In interactive mode we are not in a class */
	if (ctx != NULL) {
		/* Look for type argument */
		targ = stree_csi_find_targ(ctx, tnameref->name);

		if (targ != NULL) {
			/* Found type argument */
#ifdef DEBUG_RUN_TRACE
			printf("Found type argument '%s'.\n",
			    strtab_get_str(tnameref->name->sid));
#endif
			titem = tdata_item_new(tic_tvref);
			tvref = tdata_vref_new();
			titem->u.tvref = tvref;
			tvref->targ = targ;

			*res = titem;
			return;
		}
	}

	/* Look for symbol */
	sym = symbol_lookup_in_csi(prog, ctx, tnameref->name);
	if (sym == NULL) {
		cspan_print(tnameref->texpr->cspan);
		printf(" Error: Symbol '%s' not found.\n",
		    strtab_get_str(tnameref->name->sid));
		*res = tdata_item_new(tic_ignore);
		return;
	}

	/* Make compiler happy. */
	titem = NULL;

	switch (sym->sc) {
	case sc_csi:
		/* Construct type item. */
		titem = tdata_item_new(tic_tobject);
		tobject = tdata_object_new();
		titem->u.tobject = tobject;

		tobject->static_ref = sn_nonstatic;
		tobject->csi = sym->u.csi;
		list_init(&tobject->targs);
		break;
	case sc_ctor:
		/* It is not possible to reference a constructor explicitly. */
		assert(b_false);
		/* Fallthrough */
	case sc_deleg:
		/* Fetch stored delegate type. */
		deleg = symbol_to_deleg(sym);
		assert(deleg != NULL);
		if (deleg->titem == NULL) {
			/*
			 * Prepare a partial delegate which will be completed
			 * later.
			 */
			titem = tdata_item_new(tic_tdeleg);
			tdeleg = tdata_deleg_new();
			titem->u.tdeleg = tdeleg;
			tdeleg->deleg = deleg;
			tdeleg->tsig = NULL;

			deleg->titem = titem;
		} else {
			titem = deleg->titem;
		}
		break;
	case sc_enum:
		/* Fetch stored enum type. */
		enum_d = symbol_to_enum(sym);
		assert(enum_d != NULL);
		if (enum_d->titem == NULL) {
			/*
			 * Prepare a partial enum whic will be completed
			 * later.
			 */
			titem = tdata_item_new(tic_tenum);
			tenum = tdata_enum_new();
			titem->u.tenum = tenum;
			tenum->enum_d = enum_d;
		} else {
			titem = enum_d->titem;
		}
		break;
	case sc_fun:
	case sc_var:
	case sc_prop:
		cspan_print(tnameref->texpr->cspan);
		printf(" Error: Symbol '");
		symbol_print_fqn(sym);
		printf("' is not a type.\n");
		titem = tdata_item_new(tic_ignore);
		break;
	}

	*res = titem;
}

/** Evaluate type application expression.
 *
 * In a type application expression type arguments are applied to a generic
 * CSI.
 *
 * @param prog		Program
 * @param ctx		Current CSI (context)
 * @param tapply	Type application expression
 * @param res		Place to store type result
 */
static void run_tapply(stree_program_t *prog, stree_csi_t *ctx,
    stree_tapply_t *tapply, tdata_item_t **res)
{
	tdata_item_t *base_ti;
	tdata_item_t *arg_ti;
	tdata_item_t *titem;
	tdata_object_t *tobject;

	list_node_t *arg_n;
	stree_texpr_t *arg;

	list_node_t *farg_n;

#ifdef DEBUG_RUN_TRACE
	printf("Evaluating type apply operation.\n");
#endif
	/* Construct type item. */
	titem = tdata_item_new(tic_tobject);
	tobject = tdata_object_new();
	titem->u.tobject = tobject;

	/* Evaluate base (generic) type. */
	run_texpr(prog, ctx, tapply->gtype, &base_ti);

	if (base_ti->tic != tic_tobject) {
		cspan_print(tapply->gtype->cspan);
		printf(" Error: Base type of generic application is not "
		    "a CSI.\n");
		*res = tdata_item_new(tic_ignore);
		return;
	}

	tobject->static_ref = sn_nonstatic;
	tobject->csi = base_ti->u.tobject->csi;
	list_init(&tobject->targs);

	/* Evaluate type arguments. */
	farg_n = list_first(&tobject->csi->targ);
	arg_n = list_first(&tapply->targs);
	while (farg_n != NULL && arg_n != NULL) {
		arg = list_node_data(arg_n, stree_texpr_t *);

		run_texpr(prog, ctx, arg, &arg_ti);

		if (arg_ti->tic == tic_ignore) {
			*res = tdata_item_new(tic_ignore);
			return;
		}

		list_append(&tobject->targs, arg_ti);

		farg_n = list_next(&tobject->csi->targ, farg_n);
		arg_n = list_next(&tapply->targs, arg_n);
	}

	if (farg_n != NULL || arg_n != NULL) {
		cspan_print(tapply->texpr->cspan);
		printf(" Error: Incorrect number of type arguments.\n");
		*res = tdata_item_new(tic_ignore);
		return;
	}

	*res = titem;
}
