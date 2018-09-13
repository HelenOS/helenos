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

/** @file Typing of expressions.
 *
 * This module types (data) expressions -- not to be confused with evaluating
 * type expressions! Thus the type of each (sub-)expression is determined
 * and stored in its @c titem field.
 *
 * It can also happen that, due to implicit conversions, the expression
 * needs to be patched to insert these conversions.
 *
 * If a type error occurs within an expression, @c stype->error is set
 * and the type of the expression will be @c tic_ignore. This type item
 * is propagated upwards and causes further typing errors to be ignored
 * (this prevents a type error avalanche). Type checking is thus resumed
 * at the next expression.
 */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "cspan.h"
#include "debug.h"
#include "list.h"
#include "mytypes.h"
#include "run_texpr.h"
#include "stree.h"
#include "strtab.h"
#include "stype.h"
#include "symbol.h"
#include "tdata.h"

#include "stype_expr.h"

static void stype_nameref(stype_t *stype, stree_nameref_t *nameref,
    tdata_item_t **rtitem);
static void stype_literal(stype_t *stype, stree_literal_t *literal,
    tdata_item_t **rtitem);
static void stype_self_ref(stype_t *stype, stree_self_ref_t *self_ref,
    tdata_item_t **rtitem);

static void stype_binop(stype_t *stype, stree_binop_t *binop,
    tdata_item_t **rtitem);

static void stype_binop_tprimitive(stype_t *stype, stree_binop_t *binop,
    tdata_item_t *ta, tdata_item_t *tb, tdata_item_t **rtitem);
static void stype_binop_bool(stype_t *stype, stree_binop_t *binop,
    tdata_item_t **rtitem);
static void stype_binop_char(stype_t *stype, stree_binop_t *binop,
    tdata_item_t **rtitem);
static void stype_binop_int(stype_t *stype, stree_binop_t *binop,
    tdata_item_t **rtitem);
static void stype_binop_nil(stype_t *stype, stree_binop_t *binop,
    tdata_item_t **rtitem);
static void stype_binop_string(stype_t *stype, stree_binop_t *binop,
    tdata_item_t **rtitem);
static void stype_binop_resource(stype_t *stype, stree_binop_t *binop,
    tdata_item_t **rtitem);

static void stype_binop_tobject(stype_t *stype, stree_binop_t *binop,
    tdata_item_t *ta, tdata_item_t *tb, tdata_item_t **rtitem);
static void stype_binop_tenum(stype_t *stype, stree_binop_t *binop,
    tdata_item_t *ta, tdata_item_t *tb, tdata_item_t **rtitem);
static void stype_binop_tvref(stype_t *stype, stree_binop_t *binop,
    tdata_item_t *ta, tdata_item_t *tb, tdata_item_t **rtitem);

static void stype_unop(stype_t *stype, stree_unop_t *unop,
    tdata_item_t **rtitem);
static void stype_unop_tprimitive(stype_t *stype, stree_unop_t *unop,
    tdata_item_t *ta, tdata_item_t **rtitem);
static void stype_new(stype_t *stype, stree_new_t *new,
    tdata_item_t **rtitem);
static void stype_new_object(stype_t *stype, stree_new_t *new_op,
    tdata_item_t *obj_ti);

static void stype_access(stype_t *stype, stree_access_t *access,
    tdata_item_t **rtitem);
static void stype_access_tprimitive(stype_t *stype, stree_access_t *access,
    tdata_item_t *arg_ti, tdata_item_t **rtitem);
static void stype_access_tobject(stype_t *stype, stree_access_t *access,
    tdata_item_t *arg_ti, tdata_item_t **rtitem);
static void stype_access_tarray(stype_t *stype, stree_access_t *access,
    tdata_item_t *arg_ti, tdata_item_t **rtitem);
static void stype_access_tebase(stype_t *stype, stree_access_t *access,
    tdata_item_t *arg_ti, tdata_item_t **rtitem);

static void stype_call(stype_t *stype, stree_call_t *call,
    tdata_item_t **rtitem);
static void stype_call_args(stype_t *stype, cspan_t *cspan, list_t *farg_tis,
    tdata_item_t *fvarg_ti, list_t *args);

static void stype_index(stype_t *stype, stree_index_t *index,
    tdata_item_t **rtitem);
static void stype_index_tprimitive(stype_t *stype, stree_index_t *index,
    tdata_item_t *base_ti, tdata_item_t **rtitem);
static void stype_index_tobject(stype_t *stype, stree_index_t *index,
    tdata_item_t *base_ti, tdata_item_t **rtitem);
static void stype_index_tarray(stype_t *stype, stree_index_t *index,
    tdata_item_t *base_ti, tdata_item_t **rtitem);

static void stype_assign(stype_t *stype, stree_assign_t *assign,
    tdata_item_t **rtitem);
static void stype_as(stype_t *stype, stree_as_t *as_op, tdata_item_t **rtitem);
static void stype_box(stype_t *stype, stree_box_t *box, tdata_item_t **rtitem);

/** Type expression
 *
 * The type is stored in @a expr->titem. If the express contains a type error,
 * @a stype->error will be set when this function returns.
 *
 * @param stype		Static typing object
 * @param expr		Expression
 */
void stype_expr(stype_t *stype, stree_expr_t *expr)
{
	tdata_item_t *et;

#ifdef DEBUG_TYPE_TRACE
	cspan_print(expr->cspan);
	printf(" Type expression.\n");
#endif
	/* Silence warning. */
	et = NULL;

	switch (expr->ec) {
	case ec_nameref:
		stype_nameref(stype, expr->u.nameref, &et);
		break;
	case ec_literal:
		stype_literal(stype, expr->u.literal, &et);
		break;
	case ec_self_ref:
		stype_self_ref(stype, expr->u.self_ref, &et);
		break;
	case ec_binop:
		stype_binop(stype, expr->u.binop, &et);
		break;
	case ec_unop:
		stype_unop(stype, expr->u.unop, &et);
		break;
	case ec_new:
		stype_new(stype, expr->u.new_op, &et);
		break;
	case ec_access:
		stype_access(stype, expr->u.access, &et);
		break;
	case ec_call:
		stype_call(stype, expr->u.call, &et);
		break;
	case ec_index:
		stype_index(stype, expr->u.index, &et);
		break;
	case ec_assign:
		stype_assign(stype, expr->u.assign, &et);
		break;
	case ec_as:
		stype_as(stype, expr->u.as_op, &et);
		break;
	case ec_box:
		stype_box(stype, expr->u.box, &et);
		break;
	}

	expr->titem = et;

#ifdef DEBUG_TYPE_TRACE
	cspan_print(expr->cspan);
	printf(" Expression type is '");
	tdata_item_print(et);
	printf("'.\n");
#endif
}

/** Type name reference.
 *
 * @param stype		Static typing object
 * @param nameref	Name reference
 * @param rtitem	Place to store result type
 */
static void stype_nameref(stype_t *stype, stree_nameref_t *nameref,
    tdata_item_t **rtitem)
{
	stree_symbol_t *sym;
	stree_vdecl_t *vdecl;
	stree_proc_arg_t *proc_arg;
	tdata_item_t *titem;
	tdata_object_t *tobject;
	stree_csi_t *csi;
	stree_deleg_t *deleg;
	stree_enum_t *enum_d;
	tdata_ebase_t *tebase;
	stree_fun_t *fun;
	bool_t static_ctx;

#ifdef DEBUG_TYPE_TRACE
	cspan_print(nameref->expr->cspan);
	printf(" Evaluate type of name reference '%s'.\n",
	    strtab_get_str(nameref->name->sid));
#endif
	/*
	 * Look for a local variable declaration.
	 */

	vdecl = stype_local_vars_lookup(stype, nameref->name->sid);
	if (vdecl != NULL) {
		/* Found a local variable declaration. */
#ifdef DEBUG_RUN_TRACE
		printf("Found local variable declaration.\n");
#endif
		run_texpr(stype->program, stype->current_csi, vdecl->type,
		    &titem);
		*rtitem = titem;
		return;
	}

	/*
	 * Look for a procedure argument.
	 */

	proc_arg = stype_proc_args_lookup(stype, nameref->name->sid);
	if (proc_arg != NULL) {
		/* Found a procedure argument. */
#ifdef DEBUG_RUN_TRACE
		printf("Found procedure argument.\n");
#endif
		run_texpr(stype->program, stype->current_csi, proc_arg->type,
		    &titem);
		*rtitem = titem;
		return;
	}

	/*
	 * Look for a class-wide or global symbol.
	 */

	sym = symbol_lookup_in_csi(stype->program, stype->current_csi,
	    nameref->name);

	if (sym == NULL) {
		/* Not found. */
		if (stype->current_csi != NULL) {
			cspan_print(nameref->expr->cspan);
			printf(" Error: Symbol '%s' not found in '",
			    strtab_get_str(nameref->name->sid));
			symbol_print_fqn(csi_to_symbol(stype->current_csi));
			printf("'.\n");
		} else {
			cspan_print(nameref->expr->cspan);
			printf(" Error: Symbol '%s' not found.\n",
			    strtab_get_str(nameref->name->sid));
		}
		stype_note_error(stype);
		*rtitem = stype_recovery_titem(stype);
		return;
	}

	/* Determine if current procedure is static. */
	static_ctx = stree_symbol_is_static(stype->proc_vr->proc->outer_symbol);

	/*
	 * If the symbol is not found in current CSI, then we access it
	 * in a static context. (Context of current object cannot be used.)
	 */
	if (sym->outer_csi != stype->current_csi)
		static_ctx = b_true;

	/* Check for referencing non-static symbol in static context. */
	if (static_ctx && !stree_symbol_is_static(sym)) {
		cspan_print(nameref->expr->cspan);
		printf(" Error: Referencing non-static symbol '");
		symbol_print_fqn(sym);
		printf("' in static context.\n");
		stype_note_error(stype);
		*rtitem = stype_recovery_titem(stype);
		return;
	}

	/* Referencing static member in non-static context is allowed. */

	/* Make compiler happy. */
	titem = NULL;

	switch (sym->sc) {
	case sc_var:
		run_texpr(stype->program, stype->current_csi,
		    sym->u.var->type, &titem);
		break;
	case sc_prop:
		/* Type property header if it has not been typed yet. */
		stype_prop_header(stype, sym->u.prop);
		titem = sym->u.prop->titem;
		break;
	case sc_csi:
		csi = symbol_to_csi(sym);
		assert(csi != NULL);

		titem = tdata_item_new(tic_tobject);
		tobject = tdata_object_new();
		titem->u.tobject = tobject;

		tobject->static_ref = sn_static;
		tobject->csi = csi;
		break;
	case sc_ctor:
		/* It is not possible to reference a constructor explicitly. */
		assert(b_false);
		/* Fallthrough */
	case sc_deleg:
		deleg = symbol_to_deleg(sym);
		assert(deleg != NULL);
		/* Type delegate if it has not been typed yet. */
		stype_deleg(stype, deleg);
		titem = deleg->titem;
		break;
	case sc_enum:
		enum_d = symbol_to_enum(sym);
		assert(enum_d != NULL);

		titem = tdata_item_new(tic_tebase);
		tebase = tdata_ebase_new();
		titem->u.tebase = tebase;

		/* This is an enum base reference. */
		tebase->enum_d = enum_d;
		break;
	case sc_fun:
		fun = symbol_to_fun(sym);
		assert(fun != NULL);
		/* Type function header if it has not been typed yet. */
		stype_fun_header(stype, fun);
		titem = fun->titem;
		break;
	}

	*rtitem = titem;
}

/** Type a literal.
 *
 * @param stype		Static typing object
 * @param literal	Literal
 * @param rtitem	Place to store result type
 */
static void stype_literal(stype_t *stype, stree_literal_t *literal,
    tdata_item_t **rtitem)
{
	tdata_item_t *titem;
	tdata_primitive_t *tprimitive;
	tprimitive_class_t tpc;

#ifdef DEBUG_TYPE_TRACE
	cspan_print(literal->expr->cspan);
	printf(" Evaluate type of literal.\n");
#endif
	(void) stype;

	/* Make compiler happy. */
	tpc = 0;

	switch (literal->ltc) {
	case ltc_bool:
		tpc = tpc_bool;
		break;
	case ltc_char:
		tpc = tpc_char;
		break;
	case ltc_int:
		tpc = tpc_int;
		break;
	case ltc_ref:
		tpc = tpc_nil;
		break;
	case ltc_string:
		tpc = tpc_string;
		break;
	}

	titem = tdata_item_new(tic_tprimitive);
	tprimitive = tdata_primitive_new(tpc);
	titem->u.tprimitive = tprimitive;

	*rtitem = titem;
}

/** Type @c self reference.
 *
 * @param stype		Static typing object
 * @param self_ref	@c self reference
 * @param rtitem	Place to store result type
 */
static void stype_self_ref(stype_t *stype, stree_self_ref_t *self_ref,
    tdata_item_t **rtitem)
{
	stree_csi_t *cur_csi;
	tdata_item_t *titem;
	tdata_object_t *tobject;

#ifdef DEBUG_TYPE_TRACE
	cspan_print(self_ref->expr->cspan);
	printf(" Evaluate type of self reference.\n");
#endif
	(void) stype;
	(void) self_ref;

	cur_csi = stype->proc_vr->proc->outer_symbol->outer_csi;

	/* No global symbols should have procedures. */
	assert(cur_csi != NULL);

	/* Construct type item. */
	titem = tdata_item_new(tic_tobject);
	tobject = tdata_object_new();
	titem->u.tobject = tobject;

	tobject->static_ref = sn_nonstatic;
	tobject->csi = cur_csi;
	list_init(&tobject->targs);

	*rtitem = titem;
}

/** Type a binary operation.
 *
 * @param stype		Static typing object
 * @param binop		Binary operation
 * @param rtitem	Place to store result type
 */
static void stype_binop(stype_t *stype, stree_binop_t *binop,
    tdata_item_t **rtitem)
{
	bool_t equal;
	tdata_item_t *titem1, *titem2;

#ifdef DEBUG_TYPE_TRACE
	cspan_print(binop->expr->cspan);
	printf(" Evaluate type of binary operation.\n");
#endif
	stype_expr(stype, binop->arg1);
	stype_expr(stype, binop->arg2);

	titem1 = binop->arg1->titem;
	titem2 = binop->arg2->titem;

	if (titem1 == NULL) {
		cspan_print(binop->arg1->cspan);
		printf(" Error: Binary operand has no value.\n");
		stype_note_error(stype);
		*rtitem = stype_recovery_titem(stype);
		return;
	}

	if (titem2 == NULL) {
		cspan_print(binop->arg2->cspan);
		printf(" Error: Binary operand has no value.\n");
		stype_note_error(stype);
		*rtitem = stype_recovery_titem(stype);
		return;
	}

	if (titem1->tic == tic_ignore || titem2->tic == tic_ignore) {
		*rtitem = stype_recovery_titem(stype);
		return;
	}

	equal = tdata_item_equal(titem1, titem2);
	if (equal != b_true) {
		cspan_print(binop->expr->cspan);
		printf(" Error: Binary operation arguments "
		    "have different types ('");
		tdata_item_print(titem1);
		printf("' and '");
		tdata_item_print(titem2);
		printf("').\n");
		stype_note_error(stype);
		*rtitem = stype_recovery_titem(stype);
		return;
	}

	switch (titem1->tic) {
	case tic_tprimitive:
		stype_binop_tprimitive(stype, binop, titem1, titem2, rtitem);
		break;
	case tic_tobject:
		stype_binop_tobject(stype, binop, titem1, titem2, rtitem);
		break;
	case tic_tenum:
		stype_binop_tenum(stype, binop, titem1, titem2, rtitem);
		break;
	case tic_tvref:
		stype_binop_tvref(stype, binop, titem1, titem2, rtitem);
		break;
	default:
		cspan_print(binop->expr->cspan);
		printf(" Error: Binary operation on value which is not of a "
		    "supported type (found '");
		tdata_item_print(titem1);
		printf("').\n");
		stype_note_error(stype);
		*rtitem = stype_recovery_titem(stype);
		break;
	}

}

/** Type a binary operation with arguments of primitive type.
 *
 * @param stype		Static typing object
 * @param binop		Binary operation
 * @param ta		Type of first argument
 * @param tb		Type of second argument
 * @param rtitem	Place to store result type
 */
static void stype_binop_tprimitive(stype_t *stype, stree_binop_t *binop,
    tdata_item_t *ta, tdata_item_t *tb, tdata_item_t **rtitem)
{
	assert(ta->tic == tic_tprimitive);
	assert(tb->tic == tic_tprimitive);

	switch (ta->u.tprimitive->tpc) {
	case tpc_bool:
		stype_binop_bool(stype, binop, rtitem);
		break;
	case tpc_char:
		stype_binop_char(stype, binop, rtitem);
		break;
	case tpc_int:
		stype_binop_int(stype, binop, rtitem);
		break;
	case tpc_nil:
		stype_binop_nil(stype, binop, rtitem);
		break;
	case tpc_string:
		stype_binop_string(stype, binop, rtitem);
		break;
	case tpc_resource:
		stype_binop_resource(stype, binop, rtitem);
		break;
	}
}

/** Type a binary operation with @c bool arguments.
 *
 * @param stype		Static typing object
 * @param binop		Binary operation
 * @param rtitem	Place to store result type
 */
static void stype_binop_bool(stype_t *stype, stree_binop_t *binop,
    tdata_item_t **rtitem)
{
	tprimitive_class_t rtpc;
	tdata_item_t *res_ti;

	/* Make compiler happy. */
	rtpc = 0;

	switch (binop->bc) {
	case bo_equal:
	case bo_notequal:
	case bo_lt:
	case bo_gt:
	case bo_lt_equal:
	case bo_gt_equal:
		/* Comparison -> boolean type */
		rtpc = tpc_bool;
		break;
	case bo_plus:
	case bo_minus:
	case bo_mult:
		/* Arithmetic -> error */
		cspan_print(binop->expr->cspan);
		printf(" Error: Binary operation (%d) on booleans.\n",
		    binop->bc);
		stype_note_error(stype);
		*rtitem = stype_recovery_titem(stype);
		return;
	case bo_and:
	case bo_or:
		/* Boolean -> boolean type */
		rtpc = tpc_bool;
		break;
	}

	res_ti = tdata_item_new(tic_tprimitive);
	res_ti->u.tprimitive = tdata_primitive_new(rtpc);

	*rtitem = res_ti;
}

/** Type a binary operation with @c char arguments.
 *
 * @param stype		Static typing object
 * @param binop		Binary operation
 * @param rtitem	Place to store result type
 */
static void stype_binop_char(stype_t *stype, stree_binop_t *binop,
    tdata_item_t **rtitem)
{
	tprimitive_class_t rtpc;
	tdata_item_t *res_ti;

	(void) stype;

	/* Make compiler happy. */
	rtpc = 0;

	switch (binop->bc) {
	case bo_equal:
	case bo_notequal:
	case bo_lt:
	case bo_gt:
	case bo_lt_equal:
	case bo_gt_equal:
		/* Comparison -> boolean type */
		rtpc = tpc_bool;
		break;
	case bo_plus:
	case bo_minus:
	case bo_mult:
	case bo_and:
	case bo_or:
		/* Arithmetic, boolean -> error */
		cspan_print(binop->expr->cspan);
		printf(" Error: Binary operation (%d) on characters.\n",
		    binop->bc);
		stype_note_error(stype);
		rtpc = tpc_char;
		break;
	}

	res_ti = tdata_item_new(tic_tprimitive);
	res_ti->u.tprimitive = tdata_primitive_new(rtpc);

	*rtitem = res_ti;
}

/** Type a binary operation with @c int arguments.
 *
 * @param stype		Static typing object
 * @param binop		Binary operation
 * @param rtitem	Place to store result type
 */
static void stype_binop_int(stype_t *stype, stree_binop_t *binop,
    tdata_item_t **rtitem)
{
	tprimitive_class_t rtpc;
	tdata_item_t *res_ti;

	(void) stype;

	/* Make compiler happy. */
	rtpc = 0;

	switch (binop->bc) {
	case bo_equal:
	case bo_notequal:
	case bo_lt:
	case bo_gt:
	case bo_lt_equal:
	case bo_gt_equal:
		/* Comparison -> boolean type */
		rtpc = tpc_bool;
		break;
	case bo_plus:
	case bo_minus:
	case bo_mult:
		/* Arithmetic -> int type */
		rtpc = tpc_int;
		break;
	case bo_and:
	case bo_or:
		/* Boolean -> error */
		cspan_print(binop->expr->cspan);
		printf(" Error: Binary operation (%d) on integers.\n",
		    binop->bc);
		stype_note_error(stype);
		rtpc = tpc_char;
		break;
	}

	res_ti = tdata_item_new(tic_tprimitive);
	res_ti->u.tprimitive = tdata_primitive_new(rtpc);

	*rtitem = res_ti;
}

/** Type a binary operation with @c nil arguments.
 *
 * @param stype		Static typing object
 * @param binop		Binary operation
 * @param rtitem	Place to store result type
 */
static void stype_binop_nil(stype_t *stype, stree_binop_t *binop,
    tdata_item_t **rtitem)
{
	(void) binop;

	cspan_print(binop->expr->cspan);
	printf(" Unimplemented: Binary operation on nil.\n");
	stype_note_error(stype);
	*rtitem = stype_recovery_titem(stype);
}

/** Type a binary operation with @c string arguments.
 *
 * @param stype		Static typing object
 * @param binop		Binary operation
 * @param rtitem	Place to store result type
 */
static void stype_binop_string(stype_t *stype, stree_binop_t *binop,
    tdata_item_t **rtitem)
{
	tprimitive_class_t rtpc;
	tdata_item_t *res_ti;

	/* Make compiler happy. */
	rtpc = 0;

	switch (binop->bc) {
	case bo_equal:
	case bo_notequal:
		/* Comparison -> boolean type */
		rtpc = tpc_bool;
		break;
	case bo_plus:
		/* Concatenation -> string type */
		rtpc = tpc_string;
		break;

	case bo_lt:
	case bo_gt:
	case bo_lt_equal:
	case bo_gt_equal:

	case bo_minus:
	case bo_mult:
	case bo_and:
	case bo_or:
		/* Ordering, arithmetic, boolean -> error */
		cspan_print(binop->expr->cspan);
		printf(" Error: Binary operation (%d) on strings.\n",
		    binop->bc);
		stype_note_error(stype);
		rtpc = tpc_char;
		break;
	}

	res_ti = tdata_item_new(tic_tprimitive);
	res_ti->u.tprimitive = tdata_primitive_new(rtpc);

	*rtitem = res_ti;
}

/** Type a binary operation with resource arguments.
 *
 * @param stype		Static typing object
 * @param binop		Binary operation
 * @param rtitem	Place to store result type
 */
static void stype_binop_resource(stype_t *stype, stree_binop_t *binop,
    tdata_item_t **rtitem)
{
	tprimitive_class_t rtpc;
	tdata_item_t *res_ti;

	(void) binop;

	cspan_print(binop->expr->cspan);
	printf(" Error: Cannot apply operator to resource type.\n");
	stype_note_error(stype);
	rtpc = tpc_resource;

	res_ti = tdata_item_new(tic_tprimitive);
	res_ti->u.tprimitive = tdata_primitive_new(rtpc);

	*rtitem = res_ti;
}

/** Type a binary operation with arguments of an object type.
 *
 * @param stype		Static typing object
 * @param binop		Binary operation
 * @param ta		Type of first argument
 * @param tb		Type of second argument
 * @param rtitem	Place to store result type
 */
static void stype_binop_tobject(stype_t *stype, stree_binop_t *binop,
    tdata_item_t *ta, tdata_item_t *tb, tdata_item_t **rtitem)
{
	tdata_item_t *res_ti;

	(void) stype;

	assert(ta->tic == tic_tobject || (ta->tic == tic_tprimitive &&
	    ta->u.tprimitive->tpc == tpc_nil));
	assert(tb->tic == tic_tobject || (tb->tic == tic_tprimitive &&
	    tb->u.tprimitive->tpc == tpc_nil));

	switch (binop->bc) {
	case bo_equal:
	case bo_notequal:
		/* Comparing objects -> boolean type */
		res_ti = stype_boolean_titem(stype);
		break;
	default:
		cspan_print(binop->expr->cspan);
		printf(" Error: Binary operation (%d) on objects.\n",
		    binop->bc);
		stype_note_error(stype);
		*rtitem = stype_recovery_titem(stype);
		return;
	}

	*rtitem = res_ti;
}

/** Type a binary operation with arguments of an enum type.
 *
 * @param stype		Static typing object
 * @param binop		Binary operation
 * @param ta		Type of first argument
 * @param tb		Type of second argument
 * @param rtitem	Place to store result type
 */
static void stype_binop_tenum(stype_t *stype, stree_binop_t *binop,
    tdata_item_t *ta, tdata_item_t *tb, tdata_item_t **rtitem)
{
	tdata_item_t *res_ti;

	assert(ta->tic == tic_tenum);
	assert(tb->tic == tic_tenum);

	switch (binop->bc) {
	case bo_equal:
	case bo_notequal:
		/* Comparison -> boolean type */
		res_ti = stype_boolean_titem(stype);
		break;
	default:
		cspan_print(binop->expr->cspan);
		printf(" Error: Binary operation (%d) on values of enum "
		    "type.\n", binop->bc);
		stype_note_error(stype);
		*rtitem = stype_recovery_titem(stype);
		return;
	}

	*rtitem = res_ti;
}

/** Type a binary operation with arguments of a variable type.
 *
 * @param stype		Static typing object
 * @param binop		Binary operation
 * @param ta		Type of first argument
 * @param tb		Type of second argument
 * @param rtitem	Place to store result type
 */
static void stype_binop_tvref(stype_t *stype, stree_binop_t *binop,
    tdata_item_t *ta, tdata_item_t *tb, tdata_item_t **rtitem)
{
	tdata_item_t *res_ti;

	assert(ta->tic == tic_tvref || (ta->tic == tic_tprimitive &&
	    ta->u.tprimitive->tpc == tpc_nil));
	assert(tb->tic == tic_tvref || (tb->tic == tic_tprimitive &&
	    tb->u.tprimitive->tpc == tpc_nil));

	switch (binop->bc) {
	case bo_equal:
	case bo_notequal:
		/* Comparison -> boolean type */
		res_ti = stype_boolean_titem(stype);
		break;
	default:
		cspan_print(binop->expr->cspan);
		printf(" Error: Binary operation (%d) on variable types.\n",
		    binop->bc);
		stype_note_error(stype);
		*rtitem = stype_recovery_titem(stype);
		return;
	}

	*rtitem = res_ti;
}

/** Type a unary operation.
 *
 * @param stype		Static typing object
 * @param unop		Unary operation
 * @param rtitem	Place to store result type
 */
static void stype_unop(stype_t *stype, stree_unop_t *unop,
    tdata_item_t **rtitem)
{
	tdata_item_t *titem;

#ifdef DEBUG_TYPE_TRACE
	cspan_print(unop->expr->cspan);
	printf(" Evaluate type of unary operation.\n");
#endif
	stype_expr(stype, unop->arg);

	titem = unop->arg->titem;
	if (titem == NULL) {
		cspan_print(unop->arg->cspan);
		printf(" Error: Argument of unary operation has no value.\n");
		stype_note_error(stype);
		*rtitem = stype_recovery_titem(stype);
		return;
	}

	if (titem->tic == tic_ignore) {
		*rtitem = stype_recovery_titem(stype);
		return;
	}

	switch (titem->tic) {
	case tic_tprimitive:
		stype_unop_tprimitive(stype, unop, titem, rtitem);
		break;
	default:
		cspan_print(unop->arg->cspan);
		printf(" Error: Unary operation on value which is not of a "
		    "supported type (found '");
		tdata_item_print(titem);
		printf("').\n");
		stype_note_error(stype);
		*rtitem = stype_recovery_titem(stype);
		break;
	}
}

/** Type a binary operation arguments of primitive type.
 *
 * @param stype		Static typing object
 * @param unop		Binary operation
 * @param ta		Type of argument
 * @param rtitem	Place to store result type
 */
static void stype_unop_tprimitive(stype_t *stype, stree_unop_t *unop,
    tdata_item_t *ta, tdata_item_t **rtitem)
{
	tprimitive_class_t rtpc;
	tdata_item_t *res_ti;

	(void) stype;
	(void) unop;

	assert(ta->tic == tic_tprimitive);

	switch (ta->u.tprimitive->tpc) {
	case tpc_bool:
		rtpc = tpc_bool;
		break;
	case tpc_int:
		rtpc = tpc_int;
		break;
	default:
		cspan_print(unop->arg->cspan);
		printf(" Error: Unary operator applied on unsupported "
		    "primitive type %d.\n", ta->u.tprimitive->tpc);
		stype_note_error(stype);
		*rtitem = stype_recovery_titem(stype);
		return;
	}

	res_ti = tdata_item_new(tic_tprimitive);
	res_ti->u.tprimitive = tdata_primitive_new(rtpc);

	*rtitem = res_ti;
}

/** Type a @c new operation.
 *
 * @param stype		Static typing object
 * @param new_op	@c new operation
 * @param rtitem	Place to store result type
 */
static void stype_new(stype_t *stype, stree_new_t *new_op,
    tdata_item_t **rtitem)
{
#ifdef DEBUG_TYPE_TRACE
	cspan_print(new_op->expr->cspan);
	printf("Evaluate type of 'new' operation.\n");
#endif
	/*
	 * Type of @c new expression is exactly the type supplied as parameter
	 * to the @c new operator.
	 */
	run_texpr(stype->program, stype->current_csi, new_op->texpr, rtitem);

	if ((*rtitem)->tic == tic_ignore) {
		/* An error occured when evaluating the type expression. */
		stype_note_error(stype);
		*rtitem = stype_recovery_titem(stype);
		return;
	}

	if ((*rtitem)->tic == tic_tobject)
		stype_new_object(stype, new_op, *rtitem);
}

/** Type a new object operation arguments.
 *
 * @param stype		Static typing object
 * @param new_op	@c new operation
 */
static void stype_new_object(stype_t *stype, stree_new_t *new_op,
    tdata_item_t *obj_ti)
{
	stree_csi_t *csi;
	stree_ctor_t *ctor;
	stree_symbol_t *ctor_sym;
	stree_ident_t *ctor_ident;
	tdata_fun_sig_t *tsig;
	tdata_tvv_t *obj_tvv;
	tdata_item_t *ctor_sti;

	assert(obj_ti->tic == tic_tobject);
	csi = obj_ti->u.tobject->csi;

	if (csi->cc == csi_interface) {
		cspan_print(new_op->expr->cspan);
		printf(" Error: Cannot instantiate an interface.\n");
		stype_note_error(stype);
		return;
	}

	ctor_ident = stree_ident_new();
	ctor_ident->sid = strtab_get_sid(CTOR_IDENT);

	/* Find constructor. */
	ctor_sym = symbol_search_csi_no_base(stype->program, csi,
	    ctor_ident);

	if (ctor_sym == NULL && !list_is_empty(&new_op->ctor_args)) {
		cspan_print(new_op->expr->cspan);
		printf(" Error: Passing arguments to 'new' but no "
		    "constructor found.\n");
		stype_note_error(stype);
		return;
	}

	if (ctor_sym == NULL)
		return;

	ctor = symbol_to_ctor(ctor_sym);
	assert(ctor != NULL);

	/* Type constructor header if it has not been typed yet. */
	stype_ctor_header(stype, ctor);
	if (ctor->titem->tic == tic_ignore)
		return;

	/* Substitute type arguments in constructor type. */
	stype_titem_to_tvv(stype, obj_ti, &obj_tvv);
	tdata_item_subst(ctor->titem, obj_tvv, &ctor_sti);
	/* XXX Free obj_tvv */

	assert(ctor_sti->tic == tic_tfun);
	tsig = ctor_sti->u.tfun->tsig;

	stype_call_args(stype, new_op->expr->cspan, &tsig->arg_ti,
	    tsig->varg_ti, &new_op->ctor_args);
}

/** Type a member access operation.
 *
 * @param stype		Static typing object
 * @param access	Member access operation
 * @param rtitem	Place to store result type
 */
static void stype_access(stype_t *stype, stree_access_t *access,
    tdata_item_t **rtitem)
{
	tdata_item_t *arg_ti;

#ifdef DEBUG_TYPE_TRACE
	cspan_print(access->expr->cspan);
	printf(" Evaluate type of access operation.\n");
#endif
	stype_expr(stype, access->arg);
	arg_ti = access->arg->titem;

	if (arg_ti == NULL) {
		cspan_print(access->arg->cspan);
		printf(" Error: Argument of access operation has no value.\n");
		stype_note_error(stype);
		*rtitem = stype_recovery_titem(stype);
		return;
	}

	switch (arg_ti->tic) {
	case tic_tprimitive:
		stype_access_tprimitive(stype, access, arg_ti, rtitem);
		break;
	case tic_tobject:
		stype_access_tobject(stype, access, arg_ti, rtitem);
		break;
	case tic_tarray:
		stype_access_tarray(stype, access, arg_ti, rtitem);
		break;
	case tic_tdeleg:
		cspan_print(access->arg->cspan);
		printf(" Error: Using '.' operator on a delegate.\n");
		stype_note_error(stype);
		*rtitem = stype_recovery_titem(stype);
		break;
	case tic_tebase:
		stype_access_tebase(stype, access, arg_ti, rtitem);
		break;
	case tic_tenum:
		cspan_print(access->arg->cspan);
		printf(" Error: Using '.' operator on expression of enum "
		    "type.\n");
		stype_note_error(stype);
		*rtitem = stype_recovery_titem(stype);
		break;
	case tic_tfun:
		cspan_print(access->arg->cspan);
		printf(" Error: Using '.' operator on a function.\n");
		stype_note_error(stype);
		*rtitem = stype_recovery_titem(stype);
		break;
	case tic_tvref:
		/* Cannot allow this without some constraint. */
		cspan_print(access->arg->cspan);
		printf(" Error: Using '.' operator on generic data.\n");
		*rtitem = stype_recovery_titem(stype);
		break;
	case tic_ignore:
		*rtitem = stype_recovery_titem(stype);
		break;
	}
}

/** Type a primitive type access operation.
 *
 * @param stype		Static typing object
 * @param access	Member access operation
 * @param arg_ti	Base type
 * @param rtitem	Place to store result type
 */
static void stype_access_tprimitive(stype_t *stype, stree_access_t *access,
    tdata_item_t *arg_ti, tdata_item_t **rtitem)
{
	(void) arg_ti;

	/* Box the value. */
	access->arg = stype_box_expr(stype, access->arg);
	if (access->arg->titem->tic == tic_ignore) {
		*rtitem = stype_recovery_titem(stype);
		return;
	}

	/* Access the boxed object. */
	stype_access_tobject(stype, access, access->arg->titem, rtitem);
}

/** Type an object access operation.
 *
 * @param stype		Static typing object
 * @param access	Member access operation
 * @param arg_ti	Base type
 * @param rtitem	Place to store result type
 */
static void stype_access_tobject(stype_t *stype, stree_access_t *access,
    tdata_item_t *arg_ti, tdata_item_t **rtitem)
{
	stree_symbol_t *member_sym;
	stree_var_t *var;
	stree_enum_t *enum_d;
	stree_fun_t *fun;
	stree_prop_t *prop;
	tdata_object_t *tobject;
	tdata_item_t *mtitem;
	tdata_tvv_t *tvv;
	stree_csi_t *member_csi;

#ifdef DEBUG_TYPE_TRACE
	printf("Type a CSI access operation.\n");
#endif
	assert(arg_ti->tic == tic_tobject);
	tobject = arg_ti->u.tobject;

	/* Look for a member with the specified name. */
	member_sym = symbol_search_csi(stype->program, tobject->csi,
	    access->member_name);

	if (member_sym == NULL) {
		/* No such member found. */
		cspan_print(access->member_name->cspan);
		printf(" Error: CSI '");
		symbol_print_fqn(csi_to_symbol(tobject->csi));
		printf("' has no member named '%s'.\n",
		    strtab_get_str(access->member_name->sid));
		stype_note_error(stype);
		*rtitem = stype_recovery_titem(stype);
		return;
	}

#ifdef DEBUG_RUN_TRACE
	printf("Found member '%s'.\n",
	    strtab_get_str(access->member_name->sid));
#endif
	/* Check for accessing non-static member in static context. */
	if (tobject->static_ref == sn_static &&
	    !stree_symbol_is_static(member_sym)) {
		cspan_print(access->member_name->cspan);
		printf(" Error: Accessing non-static member '");
		symbol_print_fqn(member_sym);
		printf("' in static context.\n");
		stype_note_error(stype);
		*rtitem = stype_recovery_titem(stype);
		return;
	}

	/* Check for accessing static member in non-static context. */
	if (tobject->static_ref != sn_static &&
	    stree_symbol_is_static(member_sym)) {
		cspan_print(access->member_name->cspan);
		printf(" Error: Accessing static member '");
		symbol_print_fqn(member_sym);
		printf("' in non-static context.\n");
		stype_note_error(stype);
		*rtitem = stype_recovery_titem(stype);
		return;
	}

	/* Make compiler happy. */
	mtitem = NULL;

	switch (member_sym->sc) {
	case sc_csi:
		member_csi = symbol_to_csi(member_sym);
		assert(member_csi != NULL);

		mtitem = tdata_item_new(tic_tobject);
		tobject = tdata_object_new();
		mtitem->u.tobject = tobject;

		tobject->static_ref = sn_static;
		tobject->csi = member_csi;
		break;
	case sc_ctor:
		/* It is not possible to reference a constructor explicitly. */
		assert(b_false);
		/* Fallthrough */
	case sc_deleg:
		cspan_print(access->member_name->cspan);
		printf(" Error: Accessing object member which is a "
		    "delegate.\n");
		stype_note_error(stype);
		*rtitem = stype_recovery_titem(stype);
		return;
	case sc_enum:
		enum_d = symbol_to_enum(member_sym);
		assert(enum_d != NULL);
		/* Type enum if it has not been typed yet. */
		stype_enum(stype, enum_d);
		mtitem = enum_d->titem;
		break;
	case sc_fun:
		fun = symbol_to_fun(member_sym);
		assert(fun != NULL);
		/* Type function header now */
		stype_fun_header(stype, fun);
		mtitem = fun->titem;
		break;
	case sc_var:
		var = symbol_to_var(member_sym);
		assert(var != NULL);
		run_texpr(stype->program, member_sym->outer_csi,
		    var->type, &mtitem);
		break;
	case sc_prop:
		prop = symbol_to_prop(member_sym);
		assert(prop != NULL);
		run_texpr(stype->program, member_sym->outer_csi,
		    prop->type, &mtitem);
		break;
	}

	/*
	 * Substitute type arguments in member titem.
	 *
	 * Since the CSI can be generic the actual type of the member
	 * is obtained by substituting our type arguments into the
	 * (generic) type of the member.
	 */

	stype_titem_to_tvv(stype, arg_ti, &tvv);
	tdata_item_subst(mtitem, tvv, rtitem);
}

/** Type an array access operation.
 *
 * @param stype		Static typing object
 * @param access	Member access operation
 * @param arg_ti	Base type
 * @param rtitem	Place to store result type
 */
static void stype_access_tarray(stype_t *stype, stree_access_t *access,
    tdata_item_t *arg_ti, tdata_item_t **rtitem)
{
	(void) stype;
	(void) access;
	(void) rtitem;

	cspan_print(access->arg->cspan);
	printf(" Error: Unimplemented: Accessing array type '");
	tdata_item_print(arg_ti);
	printf("'.\n");
	stype_note_error(stype);
	*rtitem = stype_recovery_titem(stype);
}

/** Type an enum access operation.
 *
 * @param stype		Static typing object
 * @param access	Member access operation
 * @param arg_ti	Base type
 * @param rtitem	Place to store result type
 */
static void stype_access_tebase(stype_t *stype, stree_access_t *access,
    tdata_item_t *arg_ti, tdata_item_t **rtitem)
{
	tdata_ebase_t *tebase;
	tdata_enum_t *tenum;
	tdata_item_t *mtitem;
	stree_embr_t *embr;

#ifdef DEBUG_TYPE_TRACE
	printf("Type an ebase access operation.\n");
#endif
	assert(arg_ti->tic == tic_tebase);
	tebase = arg_ti->u.tebase;

	/* Look for a member with the specified name. */
	embr = stree_enum_find_mbr(tebase->enum_d, access->member_name);

	if (embr == NULL) {
		/* No such member found. */
		cspan_print(access->member_name->cspan);
		printf(" Error: Enum type '");
		symbol_print_fqn(enum_to_symbol(tebase->enum_d));
		printf("' has no member named '%s'.\n",
		    strtab_get_str(access->member_name->sid));
		stype_note_error(stype);
		*rtitem = stype_recovery_titem(stype);
		return;
	}

#ifdef DEBUG_RUN_TRACE
	printf("Found member '%s'.\n",
	    strtab_get_str(access->member_name->sid));
#endif

	mtitem = tdata_item_new(tic_tenum);
	tenum = tdata_enum_new();
	mtitem->u.tenum = tenum;
	tenum->enum_d = tebase->enum_d;

	*rtitem = mtitem;
}

/** Type a call operation.
 *
 * @param stype		Static typing object
 * @param call		Call operation
 * @param rtitem	Place to store result type
 */
static void stype_call(stype_t *stype, stree_call_t *call,
    tdata_item_t **rtitem)
{
	tdata_item_t *fun_ti;
	tdata_fun_sig_t *tsig;

#ifdef DEBUG_TYPE_TRACE
	cspan_print(call->expr->cspan);
	printf(" Evaluate type of call operation.\n");
#endif
	/* Type the function */
	stype_expr(stype, call->fun);

	/* Check type item class */
	fun_ti = call->fun->titem;
	switch (fun_ti->tic) {
	case tic_tdeleg:
		tsig = stype_deleg_get_sig(stype, fun_ti->u.tdeleg);
		assert(tsig != NULL);
		break;
	case tic_tfun:
		tsig = fun_ti->u.tfun->tsig;
		break;
	case tic_ignore:
		*rtitem = stype_recovery_titem(stype);
		return;
	default:
		cspan_print(call->fun->cspan);
		printf(" Error: Calling something which is not a function ");
		printf("(found '");
		tdata_item_print(fun_ti);
		printf("').\n");
		stype_note_error(stype);
		*rtitem = stype_recovery_titem(stype);
		return;
	}

	/* Type call arguments. */
	stype_call_args(stype, call->expr->cspan, &tsig->arg_ti, tsig->varg_ti,
	    &call->args);

	if (tsig->rtype != NULL) {
		/* XXX Might be better to clone here. */
		*rtitem = tsig->rtype;
	} else {
		*rtitem = NULL;
	}
}

/** Type call arguments.
 *
 * Type arguments in call to a function or constructor.
 *
 * @param stype		Static typing object
 * @param cpsan		Cspan to print in case of error.
 * @param farg_tis	Formal argument types (list of tdata_item_t)
 * @param args		Real arguments (list of stree_expr_t)
 */
static void stype_call_args(stype_t *stype, cspan_t *cspan, list_t *farg_tis,
    tdata_item_t *fvarg_ti, list_t *args)
{
	list_node_t *fargt_n;
	tdata_item_t *farg_ti;
	tdata_item_t *varg_ti;

	list_node_t *arg_n;
	stree_expr_t *arg;
	stree_expr_t *carg;

	/* Type and check regular arguments. */
	fargt_n = list_first(farg_tis);
	arg_n = list_first(args);

	while (fargt_n != NULL && arg_n != NULL) {
		farg_ti = list_node_data(fargt_n, tdata_item_t *);
		arg = list_node_data(arg_n, stree_expr_t *);
		stype_expr(stype, arg);

		/* XXX Because of overloaded bultin WriteLine */
		if (farg_ti == NULL) {
			/* Skip the check */
			fargt_n = list_next(farg_tis, fargt_n);
			arg_n = list_next(args, arg_n);
			continue;
		}

		/* Convert expression to type of formal argument. */
		carg = stype_convert(stype, arg, farg_ti);

		/* Patch code with augmented expression. */
		list_node_setdata(arg_n, carg);

		fargt_n = list_next(farg_tis, fargt_n);
		arg_n = list_next(args, arg_n);
	}

	/* Type and check variadic arguments. */
	if (fvarg_ti != NULL) {
		/* Obtain type of packed argument. */
		farg_ti = fvarg_ti;

		/* Get array element type */
		assert(farg_ti->tic == tic_tarray);
		varg_ti = farg_ti->u.tarray->base_ti;

		while (arg_n != NULL) {
			arg = list_node_data(arg_n, stree_expr_t *);
			stype_expr(stype, arg);

			/* Convert expression to type of formal argument. */
			carg = stype_convert(stype, arg, varg_ti);

			/* Patch code with augmented expression. */
			list_node_setdata(arg_n, carg);

			arg_n = list_next(args, arg_n);
		}
	}

	if (fargt_n != NULL) {
		cspan_print(cspan);
		printf(" Error: Too few arguments.\n");
		stype_note_error(stype);
	}

	if (arg_n != NULL) {
		cspan_print(cspan);
		printf(" Error: Too many arguments.\n");
		stype_note_error(stype);
	}
}

/** Type an indexing operation.
 *
 * @param stype		Static typing object
 * @param index		Indexing operation
 * @param rtitem	Place to store result type
 */
static void stype_index(stype_t *stype, stree_index_t *index,
    tdata_item_t **rtitem)
{
	tdata_item_t *base_ti;
	list_node_t *arg_n;
	stree_expr_t *arg;

#ifdef DEBUG_TYPE_TRACE
	cspan_print(index->expr->cspan);
	printf(" Evaluate type of index operation.\n");
#endif
	stype_expr(stype, index->base);
	base_ti = index->base->titem;

	/* Type the arguments (indices). */
	arg_n = list_first(&index->args);
	while (arg_n != NULL) {
		arg = list_node_data(arg_n, stree_expr_t *);
		stype_expr(stype, arg);

		arg_n = list_next(&index->args, arg_n);
	}

	switch (base_ti->tic) {
	case tic_tprimitive:
		stype_index_tprimitive(stype, index, base_ti, rtitem);
		break;
	case tic_tobject:
		stype_index_tobject(stype, index, base_ti, rtitem);
		break;
	case tic_tarray:
		stype_index_tarray(stype, index, base_ti, rtitem);
		break;
	case tic_tdeleg:
		cspan_print(index->base->cspan);
		printf(" Error: Indexing a delegate.\n");
		stype_note_error(stype);
		*rtitem = stype_recovery_titem(stype);
		break;
	case tic_tebase:
		cspan_print(index->base->cspan);
		printf(" Error: Indexing an enum declaration.\n");
		stype_note_error(stype);
		*rtitem = stype_recovery_titem(stype);
		break;
	case tic_tenum:
		cspan_print(index->base->cspan);
		printf(" Error: Indexing an enum value.\n");
		stype_note_error(stype);
		*rtitem = stype_recovery_titem(stype);
		break;
	case tic_tfun:
		cspan_print(index->base->cspan);
		printf(" Error: Indexing a function.\n");
		stype_note_error(stype);
		*rtitem = stype_recovery_titem(stype);
		break;
	case tic_tvref:
		/* Cannot allow this without some constraint. */
		cspan_print(index->base->cspan);
		printf(" Error: Indexing generic data.\n");
		*rtitem = stype_recovery_titem(stype);
		break;
	case tic_ignore:
		*rtitem = stype_recovery_titem(stype);
		break;
	}
}

/** Type a primitive indexing operation.
 *
 * @param stype		Static typing object
 * @param index		Indexing operation
 * @param base_ti	Base type (primitive being indexed)
 * @param rtitem	Place to store result type
 */
static void stype_index_tprimitive(stype_t *stype, stree_index_t *index,
    tdata_item_t *base_ti, tdata_item_t **rtitem)
{
	tdata_primitive_t *tprimitive;
	tdata_item_t *titem;

	(void) stype;
	(void) index;

	assert(base_ti->tic == tic_tprimitive);
	tprimitive = base_ti->u.tprimitive;

	if (tprimitive->tpc == tpc_string) {
		titem = tdata_item_new(tic_tprimitive);
		titem->u.tprimitive = tdata_primitive_new(tpc_char);
		*rtitem = titem;
		return;
	}

	cspan_print(index->base->cspan);
	printf(" Error: Indexing primitive type '");
	tdata_item_print(base_ti);
	printf("'.\n");
	stype_note_error(stype);
	*rtitem = stype_recovery_titem(stype);
}

/** Type an object indexing operation.
 *
 * @param stype		Static typing object
 * @param index		Indexing operation
 * @param base_ti	Base type (object being indexed)
 * @param rtitem	Place to store result type
 */
static void stype_index_tobject(stype_t *stype, stree_index_t *index,
    tdata_item_t *base_ti, tdata_item_t **rtitem)
{
	tdata_object_t *tobject;
	stree_symbol_t *idx_sym;
	stree_prop_t *idx;
	stree_ident_t *idx_ident;
	tdata_item_t *mtitem;
	tdata_tvv_t *tvv;

	(void) index;

#ifdef DEBUG_TYPE_TRACE
	cspan_print(index->expr->cspan);
	printf(" Indexing object type '");
	tdata_item_print(base_ti);
	printf("'.\n");
#endif

	assert(base_ti->tic == tic_tobject);
	tobject = base_ti->u.tobject;

	/* Find indexer symbol. */
	idx_ident = stree_ident_new();
	idx_ident->sid = strtab_get_sid(INDEXER_IDENT);
	idx_sym = symbol_search_csi(stype->program, tobject->csi, idx_ident);

	if (idx_sym == NULL) {
		cspan_print(index->base->cspan);
		printf(" Error: Indexing object of type '");
		tdata_item_print(base_ti);
		printf("' which does not have an indexer.\n");
		stype_note_error(stype);
		*rtitem = stype_recovery_titem(stype);
		return;
	}

	idx = symbol_to_prop(idx_sym);
	assert(idx != NULL);

	/* XXX Memoize to avoid recomputing every time. */
	run_texpr(stype->program, idx_sym->outer_csi, idx->type, &mtitem);

	/*
	 * Substitute type arguments in member titem.
	 *
	 * Since the CSI can be generic the actual type of the member
	 * is obtained by substituting our type arguments into the
	 * (generic) type of the member.
	 */

	stype_titem_to_tvv(stype, base_ti, &tvv);
	tdata_item_subst(mtitem, tvv, rtitem);
}

/** Type an array indexing operation.
 *
 * @param stype		Static typing object
 * @param index		Indexing operation
 * @param base_ti	Base type (array being indexed)
 * @param rtitem	Place to store result type
 */
static void stype_index_tarray(stype_t *stype, stree_index_t *index,
    tdata_item_t *base_ti, tdata_item_t **rtitem)
{
	list_node_t *arg_n;
	stree_expr_t *arg;
	int arg_count;

	(void) stype;
	assert(base_ti->tic == tic_tarray);

	/*
	 * Check that type of all indices is @c int and that the number of
	 * indices matches array rank.
	 */
	arg_count = 0;
	arg_n = list_first(&index->args);
	while (arg_n != NULL) {
		++arg_count;

		arg = list_node_data(arg_n, stree_expr_t *);
		if (arg->titem->tic != tic_tprimitive ||
		    arg->titem->u.tprimitive->tpc != tpc_int) {

			cspan_print(arg->cspan);
			printf(" Error: Array index is not an integer.\n");
			stype_note_error(stype);
		}

		arg_n = list_next(&index->args, arg_n);
	}

	if (arg_count != base_ti->u.tarray->rank) {
		cspan_print(index->expr->cspan);
		printf(" Error: Using %d indices with array of rank %d.\n",
		    arg_count, base_ti->u.tarray->rank);
		stype_note_error(stype);
	}

	*rtitem = base_ti->u.tarray->base_ti;
}

/** Type an assignment.
 *
 * @param stype		Static typing object
 * @param assign	Assignment operation
 * @param rtitem	Place to store result type
 */
static void stype_assign(stype_t *stype, stree_assign_t *assign,
    tdata_item_t **rtitem)
{
	stree_expr_t *csrc;

#ifdef DEBUG_TYPE_TRACE
	cspan_print(assign->expr->cspan);
	printf(" Evaluate type of assignment.\n");
#endif
	stype_expr(stype, assign->dest);
	stype_expr(stype, assign->src);

	csrc = stype_convert(stype, assign->src, assign->dest->titem);

	/* Patch code with the augmented expression. */
	assign->src = csrc;
	*rtitem = NULL;
}

/** Type @c as conversion.
 *
 * @param stype		Static typing object
 * @param as_op		@c as conversion operation
 * @param rtitem	Place to store result type
 */
static void stype_as(stype_t *stype, stree_as_t *as_op, tdata_item_t **rtitem)
{
	tdata_item_t *titem;
	tdata_item_t *pred_ti;

#ifdef DEBUG_TYPE_TRACE
	cspan_print(as_op->expr->cspan);
	printf(" Evaluate type of @c as conversion.\n");
#endif
	stype_expr(stype, as_op->arg);
	run_texpr(stype->program, stype->current_csi, as_op->dtype, &titem);

	pred_ti = stype_tobject_find_pred(stype, titem, as_op->arg->titem);
	if (pred_ti == NULL) {
		/* No CSI match. */
		stype_convert_failure(stype, convc_as, as_op->arg, titem);
		*rtitem = titem;
		return;
	}

	/*
	 * Verify that type arguments match with those specified for
	 * conversion destination.
	 */
	if (stype_targs_check_equal(stype, pred_ti, as_op->arg->titem) !=
	    EOK) {
		stype_convert_failure(stype, convc_as, as_op->arg, titem);
		*rtitem = titem;
		return;
	}

	*rtitem = titem;
}

/** Type boxing operation.
 *
 * While there is no boxing operation on the first typing pass, we do want
 * to allow potential re-evaluation (with same results).
 *
 * @param stype		Static typing object
 * @param box		Boxing operation
 * @param rtitem	Place to store result type
 */
static void stype_box(stype_t *stype, stree_box_t *box, tdata_item_t **rtitem)
{
	tdata_item_t *ptitem, *btitem;
	tdata_object_t *tobject;
	stree_symbol_t *csi_sym;
	builtin_t *bi;

#ifdef DEBUG_TYPE_TRACE
	cspan_print(box->expr->cspan);
	printf(" Evaluate type of boxing operation.\n");
#endif
	bi = stype->program->builtin;

	stype_expr(stype, box->arg);
	ptitem = box->arg->titem;

	/* Make compiler happy. */
	csi_sym = NULL;

	assert(ptitem->tic == tic_tprimitive);
	switch (ptitem->u.tprimitive->tpc) {
	case tpc_bool:
		csi_sym = bi->boxed_bool;
		break;
	case tpc_char:
		csi_sym = bi->boxed_char;
		break;
	case tpc_int:
		csi_sym = bi->boxed_int;
		break;
	case tpc_nil:
		assert(b_false);
		/* Fallthrough */
	case tpc_string:
		csi_sym = bi->boxed_string;
		break;
	case tpc_resource:
		assert(b_false);
	}

	btitem = tdata_item_new(tic_tobject);
	tobject = tdata_object_new();

	btitem->u.tobject = tobject;
	tobject->static_ref = sn_nonstatic;
	tobject->csi = symbol_to_csi(csi_sym);
	assert(tobject->csi != NULL);
	list_init(&tobject->targs);

	*rtitem = btitem;
}
