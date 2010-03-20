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

/** @file Evaluates type expressions. */

#include <assert.h>
#include <stdlib.h>
#include "list.h"
#include "mytypes.h"
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
		printf("Unimplemented: Evaluate type expression type %d.\n",
		    texpr->tc);
		exit(1);
	}
}

static void run_taccess(stree_program_t *prog, stree_csi_t *ctx,
    stree_taccess_t *taccess, tdata_item_t **res)
{
	stree_symbol_t *sym;
	tdata_item_t *targ_i;
	tdata_item_t *titem;
	tdata_object_t *tobject;
	stree_csi_t *base_csi;

#ifdef DEBUG_RUN_TRACE
	printf("Evaluating type access operation.\n");
#endif
	/* Evaluate base type. */
	run_texpr(prog, ctx, taccess->arg, &targ_i);

	if (targ_i->tic != tic_tobject) {
		printf("Error: Using '.' with type which is not an object.\n");
		exit(1);
	}

	/* Get base CSI. */
	base_csi = targ_i->u.tobject->csi;

	sym = symbol_lookup_in_csi(prog, base_csi, taccess->member_name);
	if (sym->sc != sc_csi) {
		printf("Error: Symbol '");
		symbol_print_fqn(sym);
		printf("' is not a CSI.\n");
		exit(1);
	}

	/* Construct type item. */
	titem = tdata_item_new(tic_tobject);
	tobject = tdata_object_new();
	titem->u.tobject = tobject;

	tobject->static_ref = b_false;
	tobject->csi = sym->u.csi;

	*res = titem;
}

static void run_tindex(stree_program_t *prog, stree_csi_t *ctx,
    stree_tindex_t *tindex, tdata_item_t **res)
{
	tdata_item_t *base_ti;
	tdata_item_t *titem;
	tdata_array_t *tarray;
	stree_expr_t *arg_expr;
	list_node_t *arg_node;

#ifdef DEBUG_RUN_TRACE
	printf("Evaluating type index operation.\n");
#endif
	/* Evaluate base type. */
	run_texpr(prog, ctx, tindex->base_type, &base_ti);

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

	switch (tliteral->tlc) {
	case tlc_int: tpc = tpc_int; break;
	case tlc_string: tpc = tpc_string; break;
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

#ifdef DEBUG_RUN_TRACE
	printf("Evaluating type name reference.\n");
#endif
	sym = symbol_lookup_in_csi(prog, ctx, tnameref->name);

	if (sym->sc != sc_csi) {
		printf("Error: Symbol '");
		symbol_print_fqn(sym);
		printf("' is not a CSI.\n");
		exit(1);
	}

	/* Construct type item. */
	titem = tdata_item_new(tic_tobject);
	tobject = tdata_object_new();
	titem->u.tobject = tobject;

	tobject->static_ref = b_false;
	tobject->csi = sym->u.csi;

	*res = titem;
}
