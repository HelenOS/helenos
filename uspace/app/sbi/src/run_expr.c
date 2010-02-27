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
#include "debug.h"
#include "intmap.h"
#include "list.h"
#include "mytypes.h"
#include "rdata.h"
#include "run.h"
#include "symbol.h"
#include "strtab.h"

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
static void run_binop_ref(run_t *run, stree_binop_t *binop, rdata_value_t *v1,
    rdata_value_t *v2, rdata_item_t **res);

static void run_unop(run_t *run, stree_unop_t *unop, rdata_item_t **res);
static void run_new(run_t *run, stree_new_t *new_op, rdata_item_t **res);

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
static void run_assign(run_t *run, stree_assign_t *assign, rdata_item_t **res);

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
	case ec_assign:
		run_assign(run, expr->u.assign, res);
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
	rdata_value_t *value;
	rdata_var_t *var;
	rdata_deleg_t *deleg_v;

	run_fun_ar_t *fun_ar;
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
		address = rdata_address_new();

		item->u.address = address;
		address->vref = var;

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
	fun_ar = run_get_current_fun_ar(run);
	if (fun_ar->obj != NULL) {
		assert(fun_ar->obj->vc == vc_object);
		obj = fun_ar->obj->u.object_v;
		csi_sym = obj->class_sym;
		csi = symbol_to_csi(csi_sym);
		assert(csi != NULL);
	} else {
		csi = fun_ar->fun_sym->outer_csi;
		obj = NULL;
	}

	sym = symbol_lookup_in_csi(run->program, csi, nameref->name);

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
			symbol_print_fqn(run->program, sym);
			printf("' from nested CSI '");
			symbol_print_fqn(run->program, csi_sym);
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

		deleg_v->obj = fun_ar->obj;
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
			symbol_print_fqn(run->program, sym);
			printf("' from nested CSI '");
			symbol_print_fqn(run->program, csi_sym);
			printf("'.\n");
			exit(1);
		}

		/* Find member variable in object. */
		member_var = intmap_get(&obj->fields, nameref->name->sid);
		assert(member_var != NULL);

		/* Return address of the variable. */
		item = rdata_item_new(ic_address);
		address = rdata_address_new();

		item->u.address = address;
		address->vref = member_var;

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
	int_v->value = lit_int->value;

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
	run_fun_ar_t *fun_ar;

#ifdef DEBUG_RUN_TRACE
	printf("Run self reference.\n");
#endif
	(void) self_ref;
	fun_ar = run_get_current_fun_ar(run);

	/* Return reference to the currently active object. */
	rdata_reference(fun_ar->obj, res);
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
	run_expr(run, binop->arg2, &rarg2_i);

	switch (binop->bc) {
	case bo_plus:
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

	rdata_cvt_value_item(rarg1_i, &rarg1_vi);
	rdata_cvt_value_item(rarg2_i, &rarg2_vi);

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

	int i1, i2;

	(void) run;

	item = rdata_item_new(ic_value);
	value = rdata_value_new();
	var = rdata_var_new(vc_int);
	int_v = rdata_int_new();

	item->u.value = value;
	value->var = var;
	var->u.int_v = int_v;

	i1 = v1->var->u.int_v->value;
	i2 = v2->var->u.int_v->value;

	switch (binop->bc) {
	case bo_plus:
		int_v->value = i1 + i2;
		break;

	/* XXX We should have a real boolean type. */
	case bo_equal:
		int_v->value = (i1 == i2) ? 1 : 0;
		break;
	case bo_notequal:
		int_v->value = (i1 != i2) ? 1 : 0;
		break;
	case bo_lt:
		int_v->value = (i1 < i2) ? 1 : 0;
		break;
	case bo_gt:
		int_v->value = (i1 > i2) ? 1 : 0;
		break;
	case bo_lt_equal:
		int_v->value = (i1 <= i2) ? 1 : 0;
		break;
	case bo_gt_equal:
		int_v->value = (i1 >= i2) ? 1 : 0;
		break;
	default:
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
		int_v->value = (ref1 == ref2) ? 1 : 0;
		break;
	case bo_notequal:
		int_v->value = (ref1 != ref2) ? 1 : 0;
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
	rdata_item_t *rarg;

#ifdef DEBUG_RUN_TRACE
	printf("Run unary operation.\n");
#endif
	run_expr(run, unop->arg, &rarg);
	*res = NULL;
}

/** Evaluate @c new operation. */
static void run_new(run_t *run, stree_new_t *new_op, rdata_item_t **res)
{
	rdata_object_t *obj;
	rdata_var_t *obj_var;

	stree_symbol_t *csi_sym;
	stree_csi_t *csi;
	stree_csimbr_t *csimbr;

	rdata_var_t *mbr_var;

	list_node_t *node;

#ifdef DEBUG_RUN_TRACE
	printf("Run 'new' operation.\n");
#endif
	/* Lookup object CSI. */
	/* XXX Should start in the current CSI. */
	csi_sym = symbol_xlookup_in_csi(run->program, NULL, new_op->texpr);
	csi = symbol_to_csi(csi_sym);
	if (csi == NULL) {
		printf("Error: Symbol '");
		symbol_print_fqn(run->program, csi_sym);
		printf("' is not a CSI. CSI required for 'new' operator.\n");
		exit(1);
	}

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
			mbr_var->u.int_v->value = 0;

			intmap_set(&obj->fields, csimbr->u.var->name->sid,
			    mbr_var);
		}

		node = list_next(&csi->members, node);
	}

	/* Create reference to the new object. */
	rdata_reference(obj_var, res);
}

/** Evaluate member acccess. */
static void run_access(run_t *run, stree_access_t *access, rdata_item_t **res)
{
	rdata_item_t *rarg;

#ifdef DEBUG_RUN_TRACE
	printf("Run access operation.\n");
#endif
	run_expr(run, access->arg, &rarg);
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
	switch (arg->ic) {
	case ic_value:
		vc = arg->u.value->var->vc;
		break;
	case ic_address:
		vc = arg->u.address->vref->vc;
		break;
	default:
		/* Silence warning. */
		abort();
	}

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
	rdata_dereference(arg, &darg);

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
	rdata_cvt_value_item(arg, &arg_vi);
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
		symbol_print_fqn(run->program, deleg_v->sym);
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
	rdata_object_t *object;
	rdata_item_t *ritem;
	rdata_address_t *address;

	rdata_value_t *value;
	rdata_deleg_t *deleg_v;
	rdata_var_t *var;

#ifdef DEBUG_RUN_TRACE
	printf("Run object access operation.\n");
#endif
	assert(arg->ic == ic_address);
	assert(arg->u.value->var->vc == vc_object);

	object = arg->u.value->var->u.object_v;

	member = symbol_search_csi(run->program, object->class_sym->u.csi,
	    access->member_name);

	if (member == NULL) {
		printf("Error: Object of class '");
		symbol_print_fqn(run->program, object->class_sym);
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

		deleg_v->obj = arg->u.value->var;
		deleg_v->sym = member;
		break;
	case sc_var:
		/* Construct variable address item. */
		ritem = rdata_item_new(ic_address);
		address = rdata_address_new();
		ritem->u.address = address;

		address->vref = intmap_get(&object->fields,
		    access->member_name->sid);
		assert(address->vref != NULL);
		break;
	case sc_prop:
		printf("Unimplemented: Accessing object property.\n");
		exit(1);
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
	run_fun_ar_t *fun_ar;

#ifdef DEBUG_RUN_TRACE
	printf("Run call operation.\n");
#endif
	run_expr(run, call->fun, &rfun);

	if (rfun->ic != ic_value || rfun->u.value->var->vc != vc_deleg) {
		printf("Unimplemented: Call expression of this type.\n");
		*res = NULL;
		return;
	}

	deleg_v = rfun->u.value->var->u.deleg_v;

	if (deleg_v->sym->sc != sc_fun) {
		printf("Error: Called symbol is not a function.\n");
		exit(1);
	}

#ifdef DEBUG_RUN_TRACE
	printf("Call function '");
	symbol_print_fqn(run->program, deleg_v->sym);
	printf("'\n");
#endif
	/* Evaluate function arguments. */
	list_init(&arg_vals);
	node = list_first(&call->args);

	while (node != NULL) {
		arg = list_node_data(node, stree_expr_t *);
		run_expr(run, arg, &rarg_i);
		rdata_cvt_value_item(rarg_i, &rarg_vi);

		list_append(&arg_vals, rarg_vi);
		node = list_next(&call->args, node);
	}

	fun = symbol_to_fun(deleg_v->sym);
	assert(fun != NULL);

	/* Create function activation record. */
	run_fun_ar_create(run, deleg_v->obj, fun, &fun_ar);

	/* Fill in argument values. */
	run_fun_ar_set_args(run, fun_ar, &arg_vals);

	/* Run the function. */
	run_fun(run, fun_ar, res);

#ifdef DEBUG_RUN_TRACE
	printf("Returned from function call.\n");
#endif
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
	run_expr(run, assign->src, &rsrc_i);

	rdata_cvt_value_item(rsrc_i, &rsrc_vi);
	src_val = rsrc_vi->u.value;

	if (rdest_i->ic != ic_address) {
		printf("Error: Address expression required on left side of "
		    "assignment operator.\n");
		exit(1);
	}

	rdata_address_write(rdest_i->u.address, rsrc_vi->u.value);

	*res = NULL;
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
	rdata_cvt_value_item(item, &vitem);

	assert(vitem->ic == ic_value);
	var = vitem->u.value->var;

	if (var->vc != vc_int) {
		printf("Error: Boolean (int) expression expected.\n");
		exit(1);
	}

	return (var->u.int_v->value != 0);
}
