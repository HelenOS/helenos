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

#include <stdlib.h>
#include "list.h"
#include "mytypes.h"
#include "rdata.h"
#include "run.h"
#include "symbol.h"

#include "run_texpr.h"

static void run_taccess(run_t *run, stree_taccess_t *taccess,
    rdata_titem_t **res);
static void run_tindex(run_t *run, stree_tindex_t *tindex,
    rdata_titem_t **res);
static void run_tliteral(run_t *run, stree_tliteral_t *tliteral,
    rdata_titem_t **res);
static void run_tnameref(run_t *run, stree_tnameref_t *tnameref,
    rdata_titem_t **res);

void run_texpr(run_t *run, stree_texpr_t *texpr, rdata_titem_t **res)
{
	switch (texpr->tc) {
	case tc_taccess:
		run_taccess(run, texpr->u.taccess, res);
		break;
	case tc_tindex:
		run_tindex(run, texpr->u.tindex, res);
		break;
	case tc_tliteral:
		run_tliteral(run, texpr->u.tliteral, res);
		break;
	case tc_tnameref:
		run_tnameref(run, texpr->u.tnameref, res);
		break;
	case tc_tapply:
		printf("Unimplemented: Evaluate type expression type %d.\n",
		    texpr->tc);
		exit(1);
	}
}

static void run_taccess(run_t *run, stree_taccess_t *taccess,
    rdata_titem_t **res)
{
	stree_symbol_t *sym;
	rdata_titem_t *targ_i;
	rdata_titem_t *titem;
	rdata_tcsi_t *tcsi;
	stree_csi_t *base_csi;

#ifdef DEBUG_RUN_TRACE
	printf("Evaluating type access operation.\n");
#endif
	/* Evaluate base type. */
	run_texpr(run, taccess->arg, &targ_i);

	if (targ_i->tic != tic_tcsi) {
		printf("Error: Using '.' with type which is not CSI.\n");
		exit(1);
	}

	/* Get base CSI. */
	base_csi = targ_i->u.tcsi->csi;

	sym = symbol_lookup_in_csi(run->program, base_csi,
	    taccess->member_name);
	if (sym->sc != sc_csi) {
		printf("Error: Symbol '");
		symbol_print_fqn(run->program, sym);
		printf("' is not a CSI.\n");
		exit(1);
	}

	/* Construct type item. */
	titem = rdata_titem_new(tic_tcsi);
	tcsi = rdata_tcsi_new();
	titem->u.tcsi = tcsi;

	tcsi->csi = sym->u.csi;

	*res = titem;
}

static void run_tindex(run_t *run, stree_tindex_t *tindex, rdata_titem_t **res)
{
	rdata_titem_t *base_ti;
	rdata_titem_t *titem;
	rdata_tarray_t *tarray;
	stree_expr_t *arg_expr;
	list_node_t *arg_node;

#ifdef DEBUG_RUN_TRACE
	printf("Evaluating type index operation.\n");
#endif
	/* Evaluate base type. */
	run_texpr(run, tindex->base_type, &base_ti);

	/* Construct type item. */
	titem = rdata_titem_new(tic_tarray);
	tarray = rdata_tarray_new();
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

static void run_tliteral(run_t *run, stree_tliteral_t *tliteral,
    rdata_titem_t **res)
{
	rdata_titem_t *titem;
	rdata_tprimitive_t *tprimitive;

#ifdef DEBUG_RUN_TRACE
	printf("Evaluating type literal.\n");
#endif

	(void) run;
	(void) tliteral;

	/* Construct type item. */
	titem = rdata_titem_new(tic_tprimitive);
	tprimitive = rdata_tprimitive_new();
	titem->u.tprimitive = tprimitive;

	*res = titem;
}

static void run_tnameref(run_t *run, stree_tnameref_t *tnameref,
    rdata_titem_t **res)
{
	stree_csi_t *current_csi;
	stree_symbol_t *sym;
	rdata_titem_t *titem;
	rdata_tcsi_t *tcsi;

#ifdef DEBUG_RUN_TRACE
	printf("Evaluating type name reference.\n");
#endif
	current_csi = run_get_current_csi(run);
	sym = symbol_lookup_in_csi(run->program, current_csi, tnameref->name);

	if (sym->sc != sc_csi) {
		printf("Error: Symbol '");
		symbol_print_fqn(run->program, sym);
		printf("' is not a CSI.\n");
		exit(1);
	}

	/* Construct type item. */
	titem = rdata_titem_new(tic_tcsi);
	tcsi = rdata_tcsi_new();
	titem->u.tcsi = tcsi;

	tcsi->csi = sym->u.csi;

	*res = titem;
}
