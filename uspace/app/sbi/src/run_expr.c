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

/** @file Runner (executes the code). */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "bigint.h"
#include "debug.h"
#include "intmap.h"
#include "list.h"
#include "mytypes.h"
#include "os/os.h"
#include "rdata.h"
#include "run.h"
#include "run_texpr.h"
#include "symbol.h"
#include "stree.h"
#include "strtab.h"
#include "tdata.h"

#include "run_expr.h"

static void run_nameref(run_t *run, stree_nameref_t *nameref,
    rdata_item_t **res);

static void run_literal(run_t *run, stree_literal_t *literal,
    rdata_item_t **res);
static void run_lit_int(run_t *run, stree_lit_int_t *lit_int,
    rdata_item_t **res);
static void run_lit_ref(run_t *run, stree_lit_ref_t *lit_ref,
    rdata_item_t **res);
static void run_lit_string(run_t *run, stree_lit_string_t *lit_string,
    rdata_item_t **res);

static void run_self_ref(run_t *run, stree_self_ref_t *self_ref,
    rdata_item_t **res);

static void run_binop(run_t *run, stree_binop_t *binop, rdata_item_t **res);
static void run_binop_int(run_t *run, stree_binop_t *binop, rdata_value_t *v1,
    rdata_value_t *v2, rdata_item_t **res);
static void run_binop_string(run_t *run, stree_binop_t *binop, rdata_value_t *v1,
    rdata_value_t *v2, rdata_item_t **res);
static void run_binop_ref(run_t *run, stree_binop_t *binop, rdata_value_t *v1,
    rdata_value_t *v2, rdata_item_t **res);

static void run_unop(run_t *run, stree_unop_t *unop, rdata_item_t **res);
static void run_unop_int(run_t *run, stree_unop_t *unop, rdata_value_t *val,
    rdata_item_t **res);

static void run_new(run_t *run, stree_new_t *new_op, rdata_item_t **res);
static void run_new_array(run_t *run, stree_new_t *new_op,
    tdata_item_t *titem, rdata_item_t **res);
static void run_new_object(run_t *run, stree_new_t *new_op,
    tdata_item_t *titem, rdata_item_t **res);

static void run_access(run_t *run, stree_access_t *access, rdata_item_t **res);
static void run_access_item(run_t *run, stree_access_t *access,
    rdata_item_t *arg, rdata_item_t **res);
static void run_access_ref(run_t *run, stree_access_t *access,
    rdata_item_t *arg, rdata_item_t **res);
static void run_access_deleg(run_t *run, stree_access_t *access,
    rdata_item_t *arg, rdata_item_t **res);
static void run_access_object(run_t *run, stree_access_t *access,
    rdata_item_t *arg, rdata_item_t **res);

static void run_call(run_t *run, stree_call_t *call, rdata_item_t **res);
static void run_index(run_t *run, stree_index_t *index, rdata_item_t **res);
static void run_index_array(run_t *run, stree_index_t *index,
    rdata_item_t *base, list_t *args, rdata_item_t **res);
static void run_index_object(run_t *run, stree_index_t *index,
    rdata_item_t *base, list_t *args, rdata_item_t **res);
static void run_index_string(run_t *run, stree_index_t *index,
    rdata_item_t *base, list_t *args, rdata_item_t **res);
static void run_assign(run_t *run, stree_assign_t *assign, rdata_item_t **res);
static void run_as(run_t *run, stree_as_t *as_op, rdata_item_t **res);

/** Evaluate expression. */
void run_expr(run_t *run, stree_expr_t *expr, rdata_item_t **res)
{
#ifdef DEBUG_RUN_TRACE
	printf("Executing expression.\n");
#endif

	switch (expr->ec) {
	case ec_nameref:
		run_nameref(run, expr->u.nameref, res);
		break;
	case ec_literal:
		run_literal(run, expr->u.literal, res);
		break;
	case ec_self_ref:
		run_self_ref(run, expr->u.self_ref, res);
		break;
	case ec_binop:
		run_binop(run, expr->u.binop, res);
		break;
	case ec_unop:
		run_unop(run, expr->u.unop, res);
		break;
	case ec_new:
		run_new(run, expr->u.new_op, res);
		break;
	case ec_access:
		run_access(run, expr->u.access, res);
		break;
	case ec_call:
		run_call(run, expr->u.call, res);
		break;
	case ec_index:
		run_index(run, expr->u.index, res);
		break;
	case ec_assign:
		run_assign(run, expr->u.assign, res);
		break;
	case ec_as:
		run_as(run, expr->u.as_op, res);
		break;
	}

#ifdef DEBUG_RUN_TRACE
	printf("Expression result: ");
	rdata_item_print(*res);
	printf(".\n");
#endif
}

/** Evaluate name reference expression. */
static void run_nameref(run_t *run, stree_nameref_t *nameref,
    rdata_item_t **res)
{
	stree_symbol_t *sym;
	rdata_item_t *item;
	rdata_address_t *address;
	rdata_addr_var_t *addr_var;
	rdata_value_t *value;
	rdata_var_t *var;
	rdata_deleg_t *deleg_v;

	run_proc_ar_t *proc_ar;
	stree_symbol_t *csi_sym;
	stree_csi_t *csi;
	rdata_object_t *obj;
	rdata_var_t *member_var;

#ifdef DEBUG_RUN_TRACE
	printf("Run nameref.\n");
#endif

	/*
	 * Look for a local variable.
	 */
	var = run_local_vars_lookup(run, nameref->name->sid);
	if (var != NULL) {
		/* Found a local variable. */
		item = rdata_item_new(ic_address);
		address = rdata_address_new(ac_var);
		addr_var = rdata_addr_var_new();

		item->u.address = address;
		address->u.var_a = addr_var;
		addr_var->vref = var;

		*res = item;
#ifdef DEBUG_RUN_TRACE
		printf("Found local variable.\n");
#endif
		return;
	}

	/*
	 * Look for a class-wide or global symbol.
	 */

	/* Determine currently active object or CSI. */
	proc_ar = run_get_current_proc_ar(run);
	if (proc_ar->obj != NULL) {
		assert(proc_ar->obj->vc == vc_object);
		obj = proc_ar->obj->u.object_v;
		csi_sym = obj->class_sym;
		csi = symbol_to_csi(csi_sym);
		assert(csi != NULL);
	} else {
		csi = proc_ar->proc->outer_symbol->outer_csi;
		obj = NULL;
	}

	sym = symbol_lookup_in_csi(run->program, csi, nameref->name);

	/* Existence should have been verified in type checking phase. */
	assert(sym != NULL);

	switch (sym->sc) {
	case sc_csi:
#ifdef DEBUG_RUN_TRACE
		printf("Referencing class.\n");
#endif
		item = rdata_item_new(ic_value);
		value = rdata_value_new();
		var = rdata_var_new(vc_deleg);
		deleg_v = rdata_deleg_new();

		item->u.value = value;
		value->var = var;
		var->u.deleg_v = deleg_v;

		deleg_v->obj = NULL;
		deleg_v->sym = sym;
		*res = item;
		break;
	case sc_fun:
		/* There should be no global functions. */
		assert(csi != NULL);

		if (sym->outer_csi != csi) {
			/* Function is not in the current object. */
			printf("Error: Cannot access non-static member "
			    "function '");
			symbol_print_fqn(sym);
			printf("' from nested CSI '");
			symbol_print_fqn(csi_sym);
			printf("'.\n");
			exit(1);
		}

		/* Construct delegate. */
		item = rdata_item_new(ic_value);
		value = rdata_value_new();
		item->u.value = value;

		var = rdata_var_new(vc_deleg);
		deleg_v = rdata_deleg_new();
		value->var = var;
		var->u.deleg_v = deleg_v;

		deleg_v->obj = proc_ar->obj;
		deleg_v->sym = sym;

		*res = item;
		break;
	case sc_var:
#ifdef DEBUG_RUN_TRACE
		printf("Referencing member variable.\n");
#endif
		/* There should be no global variables. */
		assert(csi != NULL);

		/* XXX Assume variable is not static for now. */
		assert(obj != NULL);

		if (sym->outer_csi != csi) {
			/* Variable is not in the current object. */
			printf("Error: Cannot access non-static member "
			    "variable '");
			symbol_print_fqn(sym);
			printf("' from nested CSI '");
			symbol_print_fqn(csi_sym);
			printf("'.\n");
			exit(1);
		}

		/* Find member variable in object. */
		member_var = intmap_get(&obj->fields, nameref->name->sid);
		assert(member_var != NULL);

		/* Return address of the variable. */
		item = rdata_item_new(ic_address);
		address = rdata_address_new(ac_var);
		addr_var = rdata_addr_var_new();

		item->u.address = address;
		address->u.var_a = addr_var;
		addr_var->vref = member_var;

		*res = item;
		break;
	default:
		printf("Referencing symbol class %d unimplemented.\n", sym->sc);
		*res = NULL;
		break;
	}
}

/** Evaluate literal. */
static void run_literal(run_t *run, stree_literal_t *literal,
    rdata_item_t **res)
{
#ifdef DEBUG_RUN_TRACE
	printf("Run literal.\n");
#endif

	switch (literal->ltc) {
	case ltc_int:
		run_lit_int(run, &literal->u.lit_int, res);
		break;
	case ltc_ref:
		run_lit_ref(run, &literal->u.lit_ref, res);
		break;
	case ltc_string:
		run_lit_string(run, &literal->u.lit_string, res);
		break;
	default:
		assert(b_false);
	}
}

/** Evaluate integer literal. */
static void run_lit_int(run_t *run, stree_lit_int_t *lit_int,
    rdata_item_t **res)
{
	rdata_item_t *item;
	rdata_value_t *value;
	rdata_var_t *var;
	rdata_int_t *int_v;

#ifdef DEBUG_RUN_TRACE
	printf("Run integer literal.\n");
#endif
	(void) run;

	item = rdata_item_new(ic_value);
	value = rdata_value_new();
	var = rdata_var_new(vc_int);
	int_v = rdata_int_new();

	item->u.value = value;
	value->var = var;
	var->u.int_v = int_v;
	bigint_clone(&lit_int->value, &int_v->value);

	*res = item;
}

/** Evaluate reference literal (@c nil). */
static void run_lit_ref(run_t *run, stree_lit_ref_t *lit_ref,
    rdata_item_t **res)
{
	rdata_item_t *item;
	rdata_value_t *value;
	rdata_var_t *var;
	rdata_ref_t *ref_v;

#ifdef DEBUG_RUN_TRACE
	printf("Run reference literal (nil).\n");
#endif
	(void) run;
	(void) lit_ref;

	item = rdata_item_new(ic_value);
	value = rdata_value_new();
	var = rdata_var_new(vc_ref);
	ref_v = rdata_ref_new();

	item->u.value = value;
	value->var = var;
	var->u.ref_v = ref_v;
	ref_v->vref = NULL;

	*res = item;
}

/** Evaluate string literal. */
static void run_lit_string(run_t *run, stree_lit_string_t *lit_string,
    rdata_item_t **res)
{
	rdata_item_t *item;
	rdata_value_t *value;
	rdata_var_t *var;
	rdata_string_t *string_v;

#ifdef DEBUG_RUN_TRACE
	printf("Run integer literal.\n");
#endif
	(void) run;

	item = rdata_item_new(ic_value);
	value = rdata_value_new();
	var = rdata_var_new(vc_string);
	string_v = rdata_string_new();

	item->u.value = value;
	value->var = var;
	var->u.string_v = string_v;
	string_v->value = lit_string->value;

	*res = item;
}

/** Evaluate @c self reference. */
static void run_self_ref(run_t *run, stree_self_ref_t *self_ref,
    rdata_item_t **res)
{
	run_proc_ar_t *proc_ar;

#ifdef DEBUG_RUN_TRACE
	printf("Run self reference.\n");
#endif
	(void) self_ref;
	proc_ar = run_get_current_proc_ar(run);

	/* Return reference to the currently active object. */
	run_reference(run, proc_ar->obj, res);
}

/** Evaluate binary operation. */
static void run_binop(run_t *run, stree_binop_t *binop, rdata_item_t **res)
{
	rdata_item_t *rarg1_i, *rarg2_i;
	rdata_item_t *rarg1_vi, *rarg2_vi;
	rdata_value_t *v1, *v2;

#ifdef DEBUG_RUN_TRACE
	printf("Run binary operation.\n");
#endif
	run_expr(run, binop->arg1, &rarg1_i);
	if (run_is_bo(run)) {
		*res = NULL;
		return;
	}

	run_expr(run, binop->arg2, &rarg2_i);
	if (run_is_bo(run)) {
		*res = NULL;
		return;
	}

	switch (binop->bc) {
	case bo_plus:
	case bo_minus:
	case bo_mult:
	case bo_equal:
	case bo_notequal:
	case bo_lt:
	case bo_gt:
	case bo_lt_equal:
	case bo_gt_equal:
		/* These are implemented so far. */
		break;
	default:
		printf("Unimplemented: Binary operation type %d.\n",
		    binop->bc);
		exit(1);
	}

#ifdef DEBUG_RUN_TRACE
	printf("Check binop argument results.\n");
#endif

	run_cvt_value_item(run, rarg1_i, &rarg1_vi);
	run_cvt_value_item(run, rarg2_i, &rarg2_vi);

	v1 = rarg1_vi->u.value;
	v2 = rarg2_vi->u.value;

	if (v1->var->vc != v2->var->vc) {
		printf("Unimplemented: Binary operation arguments have "
		    "different type.\n");
		exit(1);
	}

	switch (v1->var->vc) {
	case vc_int:
		run_binop_int(run, binop, v1, v2, res);
		break;
	case vc_string:
		run_binop_string(run, binop, v1, v2, res);
		break;
	case vc_ref:
		run_binop_ref(run, binop, v1, v2, res);
		break;
	default:
		printf("Unimplemented: Binary operation arguments of "
		    "type %d.\n", v1->var->vc);
		exit(1);
	}
}

/** Evaluate binary operation on int arguments. */
static void run_binop_int(run_t *run, stree_binop_t *binop, rdata_value_t *v1,
    rdata_value_t *v2, rdata_item_t **res)
{
	rdata_item_t *item;
	rdata_value_t *value;
	rdata_var_t *var;
	rdata_int_t *int_v;

	bigint_t *i1, *i2;
	bigint_t diff;
	bool_t done;
	bool_t zf, nf;

	(void) run;

	item = rdata_item_new(ic_value);
	value = rdata_value_new();
	var = rdata_var_new(vc_int);
	int_v = rdata_int_new();

	item->u.value = value;
	value->var = var;
	var->u.int_v = int_v;

	i1 = &v1->var->u.int_v->value;
	i2 = &v2->var->u.int_v->value;

	done = b_true;

	switch (binop->bc) {
	case bo_plus:
		bigint_add(i1, i2, &int_v->value);
		break;
	case bo_minus:
		bigint_sub(i1, i2, &int_v->value);
		break;
	case bo_mult:
		bigint_mul(i1, i2, &int_v->value);
		break;
	default:
		done = b_false;
		break;
	}

	if (done) {
		*res = item;
		return;
	}

	/* Relational operation. */

	bigint_sub(i1, i2, &diff);
	zf = bigint_is_zero(&diff);
	nf = bigint_is_negative(&diff);

	/* XXX We should have a real boolean type. */
	switch (binop->bc) {
	case bo_equal:
		bigint_init(&int_v->value, zf ? 1 : 0);
		break;
	case bo_notequal:
		bigint_init(&int_v->value, !zf ? 1 : 0);
		break;
	case bo_lt:
		bigint_init(&int_v->value, (!zf && nf) ? 1 : 0);
		break;
	case bo_gt:
		bigint_init(&int_v->value, (!zf && !nf) ? 1 : 0);
		break;
	case bo_lt_equal:
		bigint_init(&int_v->value, (zf || nf) ? 1 : 0);
		break;
	case bo_gt_equal:
		bigint_init(&int_v->value, !nf ? 1 : 0);
		break;
	default:
		assert(b_false);
	}

	*res = item;
}

/** Evaluate binary operation on string arguments. */
static void run_binop_string(run_t *run, stree_binop_t *binop, rdata_value_t *v1,
    rdata_value_t *v2, rdata_item_t **res)
{
	rdata_item_t *item;
	rdata_value_t *value;
	rdata_var_t *var;
	rdata_string_t *string_v;

	char *s1, *s2;

	(void) run;

	item = rdata_item_new(ic_value);
	value = rdata_value_new();
	var = rdata_var_new(vc_string);
	string_v = rdata_string_new();

	item->u.value = value;
	value->var = var;
	var->u.string_v = string_v;

	s1 = v1->var->u.string_v->value;
	s2 = v2->var->u.string_v->value;

	switch (binop->bc) {
	case bo_plus:
		/* Concatenate strings. */
		string_v->value = os_str_acat(s1, s2);
		break;
	default:
		printf("Error: Invalid binary operation on string "
		    "arguments (%d).\n", binop->bc);
		assert(b_false);
	}

	*res = item;
}

/** Evaluate binary operation on ref arguments. */
static void run_binop_ref(run_t *run, stree_binop_t *binop, rdata_value_t *v1,
    rdata_value_t *v2, rdata_item_t **res)
{
	rdata_item_t *item;
	rdata_value_t *value;
	rdata_var_t *var;
	rdata_int_t *int_v;

	rdata_var_t *ref1, *ref2;

	(void) run;

	item = rdata_item_new(ic_value);
	value = rdata_value_new();
	var = rdata_var_new(vc_int);
	int_v = rdata_int_new();

	item->u.value = value;
	value->var = var;
	var->u.int_v = int_v;

	ref1 = v1->var->u.ref_v->vref;
	ref2 = v2->var->u.ref_v->vref;

	switch (binop->bc) {
	/* XXX We should have a real boolean type. */
	case bo_equal:
		bigint_init(&int_v->value, (ref1 == ref2) ? 1 : 0);
		break;
	case bo_notequal:
		bigint_init(&int_v->value, (ref1 != ref2) ? 1 : 0);
		break;
	default:
		printf("Error: Invalid binary operation on reference "
		    "arguments (%d).\n", binop->bc);
		assert(b_false);
	}

	*res = item;
}


/** Evaluate unary operation. */
static void run_unop(run_t *run, stree_unop_t *unop, rdata_item_t **res)
{
	rdata_item_t *rarg_i;
	rdata_item_t *rarg_vi;
	rdata_value_t *val;

#ifdef DEBUG_RUN_TRACE
	printf("Run unary operation.\n");
#endif
	run_expr(run, unop->arg, &rarg_i);
	if (run_is_bo(run)) {
		*res = NULL;
		return;
	}

#ifdef DEBUG_RUN_TRACE
	printf("Check unop argument result.\n");
#endif
	run_cvt_value_item(run, rarg_i, &rarg_vi);

	val = rarg_vi->u.value;

	switch (val->var->vc) {
	case vc_int:
		run_unop_int(run, unop, val, res);
		break;
	default:
		printf("Unimplemented: Unrary operation argument of "
		    "type %d.\n", val->var->vc);
		run_raise_error(run);
		*res = NULL;
		break;
	}
}

/** Evaluate unary operation on int argument. */
static void run_unop_int(run_t *run, stree_unop_t *unop, rdata_value_t *val,
    rdata_item_t **res)
{
	rdata_item_t *item;
	rdata_value_t *value;
	rdata_var_t *var;
	rdata_int_t *int_v;

	(void) run;

	item = rdata_item_new(ic_value);
	value = rdata_value_new();
	var = rdata_var_new(vc_int);
	int_v = rdata_int_new();

	item->u.value = value;
	value->var = var;
	var->u.int_v = int_v;

	switch (unop->uc) {
	case uo_plus:
	        bigint_clone(&val->var->u.int_v->value, &int_v->value);
		break;
	case uo_minus:
		bigint_reverse_sign(&val->var->u.int_v->value,
		    &int_v->value);
		break;
	}

	*res = item;
}


/** Evaluate @c new operation. */
static void run_new(run_t *run, stree_new_t *new_op, rdata_item_t **res)
{
	tdata_item_t *titem;

#ifdef DEBUG_RUN_TRACE
	printf("Run 'new' operation.\n");
#endif
	/* Evaluate type expression */
	run_texpr(run->program, run_get_current_csi(run), new_op->texpr,
	    &titem);

	switch (titem->tic) {
	case tic_tarray:
		run_new_array(run, new_op, titem, res);
		break;
	case tic_tobject:
		run_new_object(run, new_op, titem, res);
		break;
	default:
		printf("Error: Invalid argument to operator 'new', "
		    "expected object.\n");
		exit(1);
	}
}

/** Create new array. */
static void run_new_array(run_t *run, stree_new_t *new_op,
    tdata_item_t *titem, rdata_item_t **res)
{
	tdata_array_t *tarray;
	rdata_array_t *array;
	rdata_var_t *array_var;
	rdata_var_t *elem_var;

	rdata_item_t *rexpr, *rexpr_vi;
	rdata_var_t *rexpr_var;

	stree_expr_t *expr;

	list_node_t *node;
	int length;
	int i;
	int rc;
	int iextent;

#ifdef DEBUG_RUN_TRACE
	printf("Create new array.\n");
#endif
	(void) run;
	(void) new_op;

	assert(titem->tic == tic_tarray);
	tarray = titem->u.tarray;

	/* Create the array. */
	assert(titem->u.tarray->rank > 0);
	array = rdata_array_new(titem->u.tarray->rank);

	/* Compute extents. */
	node = list_first(&tarray->extents);
	if (node == NULL) {
		printf("Error: Extents must be specified when constructing "
		    "an array with 'new'.\n");
		exit(1);
	}

	i = 0; length = 1;
	while (node != NULL) {
		expr = list_node_data(node, stree_expr_t *);

		/* Evaluate extent argument. */
		run_expr(run, expr, &rexpr);
		if (run_is_bo(run)) {
			*res = NULL;
			return;
		}

		run_cvt_value_item(run, rexpr, &rexpr_vi);
		assert(rexpr_vi->ic == ic_value);
		rexpr_var = rexpr_vi->u.value->var;

		if (rexpr_var->vc != vc_int) {
			printf("Error: Array extent must be an integer.\n");
			exit(1);
		}

#ifdef DEBUG_RUN_TRACE
		printf("Array extent: ");
		bigint_print(&rexpr_var->u.int_v->value);
		printf(".\n");
#endif
		rc = bigint_get_value_int(&rexpr_var->u.int_v->value,
		    &iextent);
		if (rc != EOK) {
			printf("Memory allocation failed (big int used).\n");
			exit(1);
		}

		array->extent[i] = iextent;
		length = length * array->extent[i];

		node = list_next(&tarray->extents, node);
		i += 1;
	}

	array->element = calloc(length, sizeof(rdata_var_t *));
	if (array->element == NULL) {
		printf("Memory allocation failed.\n");
		exit(1);
	}

	/* Create member variables */
	for (i = 0; i < length; ++i) {
		/* XXX Depends on member variable type. */
		elem_var = rdata_var_new(vc_int);
		elem_var->u.int_v = rdata_int_new();
		bigint_init(&elem_var->u.int_v->value, 0);

		array->element[i] = elem_var;
	}

	/* Create array variable. */
	array_var = rdata_var_new(vc_array);
	array_var->u.array_v = array;

	/* Create reference to the new array. */
	run_reference(run, array_var, res);
}

/** Create new object. */
static void run_new_object(run_t *run, stree_new_t *new_op,
    tdata_item_t *titem, rdata_item_t **res)
{
	stree_csi_t *csi;

#ifdef DEBUG_RUN_TRACE
	printf("Create new object.\n");
#endif
	(void) new_op;

	/* Lookup object CSI. */
	assert(titem->tic == tic_tobject);
	csi = titem->u.tobject->csi;

	/* Create CSI instance. */
	run_new_csi_inst(run, csi, res);
}

/** Evaluate member acccess. */
static void run_access(run_t *run, stree_access_t *access, rdata_item_t **res)
{
	rdata_item_t *rarg;

#ifdef DEBUG_RUN_TRACE
	printf("Run access operation.\n");
#endif
	run_expr(run, access->arg, &rarg);
	if (run_is_bo(run)) {
		*res = NULL;
		return;
    	}

	if (rarg == NULL) {
		printf("Error: Sub-expression has no value.\n");
		exit(1);
	}

	run_access_item(run, access, rarg, res);
}

/** Evaluate member acccess (with base already evaluated). */
static void run_access_item(run_t *run, stree_access_t *access,
    rdata_item_t *arg, rdata_item_t **res)
{
	var_class_t vc;

#ifdef DEBUG_RUN_TRACE
	printf("Run access operation on pre-evaluated base.\n");
#endif
	vc = run_item_get_vc(run, arg);

	switch (vc) {
	case vc_ref:
		run_access_ref(run, access, arg, res);
		break;
	case vc_deleg:
		run_access_deleg(run, access, arg, res);
		break;
	case vc_object:
		run_access_object(run, access, arg, res);
		break;
	default:
		printf("Unimplemented: Using access operator ('.') "
		    "with unsupported data type (value/%d).\n", vc);
		exit(1);
	}
}

/** Evaluate reference acccess. */
static void run_access_ref(run_t *run, stree_access_t *access,
    rdata_item_t *arg, rdata_item_t **res)
{
	rdata_item_t *darg;

	/* Implicitly dereference. */
	run_dereference(run, arg, &darg);

	if (run->thread_ar->bo_mode != bm_none) {
		*res = run_recovery_item(run);
		return;
	}

	/* Try again. */
	run_access_item(run, access, darg, res);
}

/** Evaluate delegate-member acccess. */
static void run_access_deleg(run_t *run, stree_access_t *access,
    rdata_item_t *arg, rdata_item_t **res)
{
	rdata_item_t *arg_vi;
	rdata_value_t *arg_val;
	rdata_deleg_t *deleg_v;
	stree_symbol_t *member;

#ifdef DEBUG_RUN_TRACE
	printf("Run delegate access operation.\n");
#endif
	run_cvt_value_item(run, arg, &arg_vi);
	arg_val = arg_vi->u.value;
	assert(arg_val->var->vc == vc_deleg);

	deleg_v = arg_val->var->u.deleg_v;
	if (deleg_v->obj != NULL || deleg_v->sym->sc != sc_csi) {
		printf("Error: Using '.' with delegate to different object "
		    "than a CSI (%d).\n", deleg_v->sym->sc);
		exit(1);
	}

	member = symbol_search_csi(run->program, deleg_v->sym->u.csi,
	    access->member_name);

	if (member == NULL) {
		printf("Error: CSI '");
		symbol_print_fqn(deleg_v->sym);
		printf("' has no member named '%s'.\n",
		    strtab_get_str(access->member_name->sid));
		exit(1);
	}

#ifdef DEBUG_RUN_TRACE
	printf("Found member '%s'.\n",
	    strtab_get_str(access->member_name->sid));
#endif

	/*
	 * Reuse existing item, value, var, deleg.
	 * XXX This is maybe not a good idea because it complicates memory
	 * management as there is not a single owner 
	 */
	deleg_v->sym = member;
	*res = arg;
}

/** Evaluate object member acccess. */
static void run_access_object(run_t *run, stree_access_t *access,
    rdata_item_t *arg, rdata_item_t **res)
{
	stree_symbol_t *member;
	rdata_var_t *object_var;
	rdata_object_t *object;
	rdata_item_t *ritem;
	rdata_address_t *address;
	rdata_addr_var_t *addr_var;
	rdata_addr_prop_t *addr_prop;
	rdata_aprop_named_t *aprop_named;
	rdata_deleg_t *deleg_p;

	rdata_value_t *value;
	rdata_deleg_t *deleg_v;
	rdata_var_t *var;

#ifdef DEBUG_RUN_TRACE
	printf("Run object access operation.\n");
#endif
	assert(arg->ic == ic_address);
	assert(arg->u.address->ac == ac_var);
	assert(arg->u.address->u.var_a->vref->vc == vc_object);

	object_var = arg->u.address->u.var_a->vref;
	object = object_var->u.object_v;

	member = symbol_search_csi(run->program, object->class_sym->u.csi,
	    access->member_name);

	if (member == NULL) {
		printf("Error: Object of class '");
		symbol_print_fqn(object->class_sym);
		printf("' has no member named '%s'.\n",
		    strtab_get_str(access->member_name->sid));
		exit(1);
	}

#ifdef DEBUG_RUN_TRACE
	printf("Found member '%s'.\n",
	    strtab_get_str(access->member_name->sid));
#endif

	switch (member->sc) {
	case sc_csi:
		printf("Error: Accessing object member which is nested CSI.\n");
		exit(1);
	case sc_fun:
		/* Construct delegate. */
		ritem = rdata_item_new(ic_value);
		value = rdata_value_new();
		ritem->u.value = value;

		var = rdata_var_new(vc_deleg);
		value->var = var;
		deleg_v = rdata_deleg_new();
		var->u.deleg_v = deleg_v;

		deleg_v->obj = arg->u.address->u.var_a->vref;
		deleg_v->sym = member;
		break;
	case sc_var:
		/* Construct variable address item. */
		ritem = rdata_item_new(ic_address);
		address = rdata_address_new(ac_var);
		addr_var = rdata_addr_var_new();
		ritem->u.address = address;
		address->u.var_a = addr_var;

		addr_var->vref = intmap_get(&object->fields,
		    access->member_name->sid);
		assert(addr_var->vref != NULL);
		break;
	case sc_prop:
		/* Construct named property address. */
		ritem = rdata_item_new(ic_address);
		address = rdata_address_new(ac_prop);
		addr_prop = rdata_addr_prop_new(apc_named);
		aprop_named = rdata_aprop_named_new();
		ritem->u.address = address;
		address->u.prop_a = addr_prop;
		addr_prop->u.named = aprop_named;

		deleg_p = rdata_deleg_new();
		deleg_p->obj = object_var;
		deleg_p->sym = member;
		addr_prop->u.named->prop_d = deleg_p;
		break;
	}

	*res = ritem;
}

/** Call a function. */
static void run_call(run_t *run, stree_call_t *call, rdata_item_t **res)
{
	rdata_item_t *rfun;
	rdata_deleg_t *deleg_v;
	list_t arg_vals;
	list_node_t *node;
	stree_expr_t *arg;
	rdata_item_t *rarg_i, *rarg_vi;

	stree_fun_t *fun;
	run_proc_ar_t *proc_ar;

#ifdef DEBUG_RUN_TRACE
	printf("Run call operation.\n");
#endif
	run_expr(run, call->fun, &rfun);
	if (run_is_bo(run)) {
		*res = NULL;
		return;
	}

	if (run->thread_ar->bo_mode != bm_none) {
		*res = run_recovery_item(run);
		return;
	}

	if (rfun->ic != ic_value || rfun->u.value->var->vc != vc_deleg) {
		printf("Unimplemented: Call expression of this type.\n");
		exit(1);
	}

	deleg_v = rfun->u.value->var->u.deleg_v;

	if (deleg_v->sym->sc != sc_fun) {
		printf("Error: Called symbol is not a function.\n");
		exit(1);
	}

#ifdef DEBUG_RUN_TRACE
	printf("Call function '");
	symbol_print_fqn(deleg_v->sym);
	printf("'\n");
#endif
	/* Evaluate function arguments. */
	list_init(&arg_vals);
	node = list_first(&call->args);

	while (node != NULL) {
		arg = list_node_data(node, stree_expr_t *);
		run_expr(run, arg, &rarg_i);
		if (run_is_bo(run)) {
			*res = NULL;
			return;
		}

		run_cvt_value_item(run, rarg_i, &rarg_vi);

		list_append(&arg_vals, rarg_vi);
		node = list_next(&call->args, node);
	}

	fun = symbol_to_fun(deleg_v->sym);
	assert(fun != NULL);

	/* Create procedure activation record. */
	run_proc_ar_create(run, deleg_v->obj, fun->proc, &proc_ar);

	/* Fill in argument values. */
	run_proc_ar_set_args(run, proc_ar, &arg_vals);

	/* Run the function. */
	run_proc(run, proc_ar, res);

#ifdef DEBUG_RUN_TRACE
	printf("Returned from function call.\n");
#endif
}

/** Run index operation. */
static void run_index(run_t *run, stree_index_t *index, rdata_item_t **res)
{
	rdata_item_t *rbase;
	rdata_item_t *base_i;
	list_node_t *node;
	stree_expr_t *arg;
	rdata_item_t *rarg_i, *rarg_vi;
	var_class_t vc;
	list_t arg_vals;

#ifdef DEBUG_RUN_TRACE
	printf("Run index operation.\n");
#endif
	run_expr(run, index->base, &rbase);
	if (run_is_bo(run)) {
		*res = NULL;
		return;
	}

	vc = run_item_get_vc(run, rbase);

	/* Implicitly dereference. */
	if (vc == vc_ref) {
		run_dereference(run, rbase, &base_i);
	} else {
		base_i = rbase;
	}

	vc = run_item_get_vc(run, base_i);

	/* Evaluate arguments (indices). */
	node = list_first(&index->args);
	list_init(&arg_vals);

	while (node != NULL) {
		arg = list_node_data(node, stree_expr_t *);
		run_expr(run, arg, &rarg_i);
		if (run_is_bo(run)) {
			*res = NULL;
			return;
		}

		run_cvt_value_item(run, rarg_i, &rarg_vi);

		list_append(&arg_vals, rarg_vi);

		node = list_next(&index->args, node);
	}

	switch (vc) {
	case vc_array:
		run_index_array(run, index, base_i, &arg_vals, res);
		break;
	case vc_object:
		run_index_object(run, index, base_i, &arg_vals, res);
		break;
	case vc_string:
		run_index_string(run, index, base_i, &arg_vals, res);
		break;
	default:
		printf("Error: Indexing object of bad type (%d).\n", vc);
		exit(1);
	}
}

/** Run index operation on array. */
static void run_index_array(run_t *run, stree_index_t *index,
    rdata_item_t *base, list_t *args, rdata_item_t **res)
{
	list_node_t *node;
	rdata_array_t *array;
	rdata_item_t *arg;

	int i;
	int elem_index;
	int arg_val;
	int rc;

	rdata_item_t *ritem;
	rdata_address_t *address;
	rdata_addr_var_t *addr_var;

#ifdef DEBUG_RUN_TRACE
	printf("Run array index operation.\n");
#endif
	(void) run;
	(void) index;

	assert(base->ic == ic_address);
	assert(base->u.address->ac == ac_var);
	assert(base->u.address->u.var_a->vref->vc == vc_array);
	array = base->u.address->u.var_a->vref->u.array_v;

	/*
	 * Linear index of the desired element. Elements are stored in
	 * lexicographic order with the last index changing the fastest.
	 */
	elem_index = 0;

	node = list_first(args);
	i = 0;

	while (node != NULL) {
		if (i >= array->rank) {
			printf("Error: Too many indices for array of rank %d",
			    array->rank);
			exit(1);
		}

		arg = list_node_data(node, rdata_item_t *);
		assert(arg->ic == ic_value);

		if (arg->u.value->var->vc != vc_int) {
			printf("Error: Array index is not an integer.\n");
			exit(1);
		}

		rc = bigint_get_value_int(
		    &arg->u.value->var->u.int_v->value,
		    &arg_val);

		if (rc != EOK || arg_val < 0 || arg_val >= array->extent[i]) {
#ifdef DEBUG_RUN_TRACE
			printf("Error: Array index (value: %d) is out of range.\n",
			    arg_val);
#endif
			/* Raise Error.OutOfBounds */
			run_raise_exc(run,
			    run->program->builtin->error_outofbounds);
			*res = run_recovery_item(run);
			return;
		}

		elem_index = elem_index * array->extent[i] + arg_val;

		node = list_next(args, node);
		i += 1;
	}

	if (i < array->rank) {
		printf("Error: Too few indices for array of rank %d",
		    array->rank);
		exit(1);
	}

	/* Construct variable address item. */
	ritem = rdata_item_new(ic_address);
	address = rdata_address_new(ac_var);
	addr_var = rdata_addr_var_new();
	ritem->u.address = address;
	address->u.var_a = addr_var;

	addr_var->vref = array->element[elem_index];

	*res = ritem;
}

/** Index an object (via its indexer). */
static void run_index_object(run_t *run, stree_index_t *index,
    rdata_item_t *base, list_t *args, rdata_item_t **res)
{
	rdata_item_t *ritem;
	rdata_address_t *address;
	rdata_addr_prop_t *addr_prop;
	rdata_aprop_indexed_t *aprop_indexed;
	rdata_var_t *obj_var;
	stree_csi_t *obj_csi;
	rdata_deleg_t *object_d;
	stree_symbol_t *indexer_sym;
	stree_ident_t *indexer_ident;

	list_node_t *node;
	rdata_item_t *arg;

#ifdef DEBUG_RUN_TRACE
	printf("Run object index operation.\n");
#endif
	(void) index;

	/* Construct property address item. */
	ritem = rdata_item_new(ic_address);
	address = rdata_address_new(ac_prop);
	addr_prop = rdata_addr_prop_new(apc_indexed);
	aprop_indexed = rdata_aprop_indexed_new();
	ritem->u.address = address;
	address->u.prop_a = addr_prop;
	addr_prop->u.indexed = aprop_indexed;

	if (base->ic != ic_address || base->u.address->ac != ac_var) {
		/* XXX Several other cases can occur. */
		printf("Unimplemented: Indexing object varclass via something "
		    "which is not a simple variable reference.\n");
		exit(1);
	}

	/* Find indexer symbol. */
	obj_var = base->u.address->u.var_a->vref;
	assert(obj_var->vc == vc_object);
	indexer_ident = stree_ident_new();
	indexer_ident->sid = strtab_get_sid(INDEXER_IDENT);
	obj_csi = symbol_to_csi(obj_var->u.object_v->class_sym);
	assert(obj_csi != NULL);
	indexer_sym = symbol_search_csi(run->program, obj_csi, indexer_ident);

	if (indexer_sym == NULL) {
		printf("Error: Accessing object which does not have an "
		    "indexer.\n");
		exit(1);
	}

	/* Construct delegate. */
	object_d = rdata_deleg_new();
	object_d->obj = obj_var;
	object_d->sym = indexer_sym;
	aprop_indexed->object_d = object_d;

	/* Copy list of argument values. */
	list_init(&aprop_indexed->args);

	node = list_first(args);
	while (node != NULL) {
		arg = list_node_data(node, rdata_item_t *);
		list_append(&aprop_indexed->args, arg);
		node = list_next(args, node);
	}

	*res = ritem;
}

/** Run index operation on string. */
static void run_index_string(run_t *run, stree_index_t *index,
    rdata_item_t *base, list_t *args, rdata_item_t **res)
{
	list_node_t *node;
	rdata_string_t *string;
	rdata_item_t *base_vi;
	rdata_item_t *arg;

	int i;
	int elem_index;
	int arg_val;
	int rc1, rc2;

	rdata_value_t *value;
	rdata_var_t *cvar;
	rdata_item_t *ritem;
	int cval;

#ifdef DEBUG_RUN_TRACE
	printf("Run string index operation.\n");
#endif
	(void) run;
	(void) index;

	run_cvt_value_item(run, base, &base_vi);
	assert(base_vi->u.value->var->vc == vc_string);
	string = base_vi->u.value->var->u.string_v;

	/*
	 * Linear index of the desired element. Elements are stored in
	 * lexicographic order with the last index changing the fastest.
	 */
	node = list_first(args);
	elem_index = 0;

	i = 0;
	while (node != NULL) {
		if (i >= 1) {
			printf("Error: Too many indices string.\n");
			exit(1);
		}

		arg = list_node_data(node, rdata_item_t *);
		assert(arg->ic == ic_value);

		if (arg->u.value->var->vc != vc_int) {
			printf("Error: String index is not an integer.\n");
			exit(1);
		}

		rc1 = bigint_get_value_int(
		    &arg->u.value->var->u.int_v->value,
		    &arg_val);

		elem_index = arg_val;

		node = list_next(args, node);
		i += 1;
	}

	if (i < 1) {
		printf("Error: Too few indices for string.\n");
		exit(1);
	}

	if (rc1 == EOK)
		rc2 = os_str_get_char(string->value, elem_index, &cval);

	if (rc1 != EOK || rc2 != EOK) {
		printf("Error: String index (value: %d) is out of range.\n",
		    arg_val);
		exit(1);
	}

	/* Construct character value. */
	ritem = rdata_item_new(ic_value);
	value = rdata_value_new();
	ritem->u.value = value;

	cvar = rdata_var_new(vc_int);
	cvar->u.int_v = rdata_int_new();
	bigint_init(&cvar->u.int_v->value, cval);
	value->var = cvar;

	*res = ritem;
}

/** Execute assignment. */
static void run_assign(run_t *run, stree_assign_t *assign, rdata_item_t **res)
{
	rdata_item_t *rdest_i, *rsrc_i;
	rdata_item_t *rsrc_vi;
	rdata_value_t *src_val;

#ifdef DEBUG_RUN_TRACE
	printf("Run assign operation.\n");
#endif
	run_expr(run, assign->dest, &rdest_i);
	if (run_is_bo(run)) {
		*res = NULL;
		return;
	}

	run_expr(run, assign->src, &rsrc_i);
	if (run_is_bo(run)) {
		*res = NULL;
		return;
	}

	run_cvt_value_item(run, rsrc_i, &rsrc_vi);
	assert(rsrc_vi->ic == ic_value);
	src_val = rsrc_vi->u.value;

	if (rdest_i->ic != ic_address) {
		printf("Error: Address expression required on left side of "
		    "assignment operator.\n");
		exit(1);
	}

	run_address_write(run, rdest_i->u.address, rsrc_vi->u.value);

	*res = NULL;
}

/** Execute @c as conversion. */
static void run_as(run_t *run, stree_as_t *as_op, rdata_item_t **res)
{
	rdata_item_t *rarg_i;
	rdata_item_t *rarg_vi;
	rdata_item_t *rarg_di;
	rdata_var_t *arg_vref;
	tdata_item_t *dtype;
	run_proc_ar_t *proc_ar;

	stree_symbol_t *obj_csi_sym;
	stree_csi_t *obj_csi;

#ifdef DEBUG_RUN_TRACE
	printf("Run @c as conversion operation.\n");
#endif
	run_expr(run, as_op->arg, &rarg_i);
	if (run_is_bo(run)) {
		*res = NULL;
		return;
	}

	/*
	 * This should always be a reference if the argument is indeed
	 * a class instance.
	 */
	assert(run_item_get_vc(run, rarg_i) == vc_ref);
	run_cvt_value_item(run, rarg_i, &rarg_vi);
	assert(rarg_vi->ic == ic_value);

	if (rarg_vi->u.value->var->u.ref_v->vref == NULL) {
		/* Nil reference is always okay. */
		*res = rarg_vi;
		return;
	}

	run_dereference(run, rarg_vi, &rarg_di);

	/* Now we should have a variable address. */
	assert(rarg_di->ic == ic_address);
	assert(rarg_di->u.address->ac == ac_var);

	arg_vref = rarg_di->u.address->u.var_a->vref;

	proc_ar = run_get_current_proc_ar(run);
	/* XXX Memoize to avoid recomputing. */
	run_texpr(run->program, proc_ar->proc->outer_symbol->outer_csi,
	    as_op->dtype, &dtype);

	assert(arg_vref->vc == vc_object);
	obj_csi_sym = arg_vref->u.object_v->class_sym;
	obj_csi = symbol_to_csi(obj_csi_sym);
	assert(obj_csi != NULL);

	if (tdata_is_csi_derived_from_ti(obj_csi, dtype) != b_true) {
		printf("Error: Run-time type conversion error. Object is "
		    "of type '");
		symbol_print_fqn(obj_csi_sym);
		printf("' which is not derived from '");
		tdata_item_print(dtype);
		printf("'.\n");
		exit(1);
	}

	*res = rarg_vi;
}

/** Create new CSI instance. */
void run_new_csi_inst(run_t *run, stree_csi_t *csi, rdata_item_t **res)
{
	rdata_object_t *obj;
	rdata_var_t *obj_var;

	stree_symbol_t *csi_sym;
	stree_csimbr_t *csimbr;

	rdata_var_t *mbr_var;

	list_node_t *node;

	csi_sym = csi_to_symbol(csi);

#ifdef DEBUG_RUN_TRACE
	printf("Create new instance of CSI '");
	symbol_print_fqn(csi_sym);
	printf("'.\n");
#endif

	/* Create the object. */
	obj = rdata_object_new();
	obj->class_sym = csi_sym;
	intmap_init(&obj->fields);

	obj_var = rdata_var_new(vc_object);
	obj_var->u.object_v = obj;

	/* Create object fields. */
	node = list_first(&csi->members);
	while (node != NULL) {
		csimbr = list_node_data(node, stree_csimbr_t *);
		if (csimbr->cc == csimbr_var) {
			/* XXX Depends on member variable type. */
			mbr_var = rdata_var_new(vc_int);
			mbr_var->u.int_v = rdata_int_new();
			bigint_init(&mbr_var->u.int_v->value, 0);

			intmap_set(&obj->fields, csimbr->u.var->name->sid,
			    mbr_var);
		}

		node = list_next(&csi->members, node);
	}

	/* Create reference to the new object. */
	run_reference(run, obj_var, res);
}

/** Return boolean value of an item.
 *
 * Tries to interpret @a item as a boolean value. If it is not a boolean
 * value, this generates an error.
 *
 * XXX Currently int supplants the role of a true boolean type.
 */
bool_t run_item_boolean_value(run_t *run, rdata_item_t *item)
{
	rdata_item_t *vitem;
	rdata_var_t *var;

	(void) run;
	run_cvt_value_item(run, item, &vitem);

	assert(vitem->ic == ic_value);
	var = vitem->u.value->var;

	if (var->vc != vc_int) {
		printf("Error: Boolean (int) expression expected.\n");
		exit(1);
	}

	return !bigint_is_zero(&var->u.int_v->value);
}
