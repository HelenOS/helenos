/*
 * Copyright (c) 2011 Jiri Svoboda
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

/** @file Run expressions. */

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
static void run_lit_bool(run_t *run, stree_lit_bool_t *lit_bool,
    rdata_item_t **res);
static void run_lit_char(run_t *run, stree_lit_char_t *lit_char,
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
static void run_binop_bool(run_t *run, stree_binop_t *binop, rdata_value_t *v1,
    rdata_value_t *v2, rdata_item_t **res);
static void run_binop_char(run_t *run, stree_binop_t *binop, rdata_value_t *v1,
    rdata_value_t *v2, rdata_item_t **res);
static void run_binop_int(run_t *run, stree_binop_t *binop, rdata_value_t *v1,
    rdata_value_t *v2, rdata_item_t **res);
static void run_binop_string(run_t *run, stree_binop_t *binop, rdata_value_t *v1,
    rdata_value_t *v2, rdata_item_t **res);
static void run_binop_ref(run_t *run, stree_binop_t *binop, rdata_value_t *v1,
    rdata_value_t *v2, rdata_item_t **res);
static void run_binop_enum(run_t *run, stree_binop_t *binop, rdata_value_t *v1,
    rdata_value_t *v2, rdata_item_t **res);

static void run_unop(run_t *run, stree_unop_t *unop, rdata_item_t **res);
static void run_unop_bool(run_t *run, stree_unop_t *unop, rdata_value_t *val,
    rdata_item_t **res);
static void run_unop_int(run_t *run, stree_unop_t *unop, rdata_value_t *val,
    rdata_item_t **res);

static void run_new(run_t *run, stree_new_t *new_op, rdata_item_t **res);
static void run_new_array(run_t *run, stree_new_t *new_op,
    tdata_item_t *titem, rdata_item_t **res);
static void run_new_object(run_t *run, stree_new_t *new_op,
    tdata_item_t *titem, rdata_item_t **res);

static void run_object_ctor(run_t *run, rdata_var_t *obj, list_t *arg_vals);

static void run_access(run_t *run, stree_access_t *access, rdata_item_t **res);
static void run_access_item(run_t *run, stree_access_t *access,
    rdata_item_t *arg, rdata_item_t **res);
static void run_access_ref(run_t *run, stree_access_t *access,
    rdata_item_t *arg, rdata_item_t **res);
static void run_access_deleg(run_t *run, stree_access_t *access,
    rdata_item_t *arg, rdata_item_t **res);
static void run_access_object(run_t *run, stree_access_t *access,
    rdata_item_t *arg, rdata_item_t **res);
static void run_access_object_static(run_t *run, stree_access_t *access,
    rdata_var_t *obj_var, rdata_item_t **res);
static void run_access_object_nonstatic(run_t *run, stree_access_t *access,
    rdata_var_t *obj_var, rdata_item_t **res);
static void run_access_symbol(run_t *run, stree_access_t *access,
    rdata_item_t *arg, rdata_item_t **res);

static void run_call(run_t *run, stree_call_t *call, rdata_item_t **res);
static void run_call_args(run_t *run, list_t *args, list_t *arg_vals);
static void run_destroy_arg_vals(list_t *arg_vals);

static void run_index(run_t *run, stree_index_t *index, rdata_item_t **res);
static void run_index_array(run_t *run, stree_index_t *index,
    rdata_item_t *base, list_t *args, rdata_item_t **res);
static void run_index_object(run_t *run, stree_index_t *index,
    rdata_item_t *base, list_t *args, rdata_item_t **res);
static void run_index_string(run_t *run, stree_index_t *index,
    rdata_item_t *base, list_t *args, rdata_item_t **res);
static void run_assign(run_t *run, stree_assign_t *assign, rdata_item_t **res);
static void run_as(run_t *run, stree_as_t *as_op, rdata_item_t **res);
static void run_box(run_t *run, stree_box_t *box, rdata_item_t **res);

/** Evaluate expression.
 *
 * Run the expression @a expr and store pointer to the result in *(@a res).
 * If the expression has on value (assignment) then @c NULL is returned.
 * @c NULL is also returned if an error or exception occurs.
 *
 * @param run		Runner object
 * @param expr		Expression to run
 * @param res		Place to store result
 */
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
	case ec_box:
		run_box(run, expr->u.box, res);
		break;
	}

#ifdef DEBUG_RUN_TRACE
	printf("Expression result: ");
	rdata_item_print(*res);
	printf(".\n");
#endif
}

/** Evaluate name reference expression.
 *
 * @param run		Runner object
 * @param nameref	Name reference
 * @param res		Place to store result
 */
static void run_nameref(run_t *run, stree_nameref_t *nameref,
    rdata_item_t **res)
{
	stree_symbol_t *sym;
	rdata_item_t *item;
	rdata_address_t *address;
	rdata_addr_var_t *addr_var;
	rdata_addr_prop_t *addr_prop;
	rdata_aprop_named_t *aprop_named;
	rdata_deleg_t *deleg_p;
	rdata_value_t *value;
	rdata_var_t *var;
	rdata_deleg_t *deleg_v;
	rdata_symbol_t *symbol_v;

	run_proc_ar_t *proc_ar;
	stree_symbol_t *csi_sym;
	stree_csi_t *csi;
	rdata_object_t *obj;
	rdata_var_t *member_var;

	rdata_var_t *psobj;
	rdata_var_t *sobj;
	rdata_object_t *aobj;

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

	assert (proc_ar->obj != NULL);
	assert(proc_ar->obj->vc == vc_object);
	obj = proc_ar->obj->u.object_v;
	csi_sym = obj->class_sym;

	if (csi_sym != NULL) {
		csi = symbol_to_csi(csi_sym);
		assert(csi != NULL);
	} else {
		/* This happens in interactive mode. */
		csi = NULL;
	}

	sym = symbol_lookup_in_csi(run->program, csi, nameref->name);

	/* Existence should have been verified in type checking phase. */
	assert(sym != NULL);

	switch (sym->sc) {
	case sc_csi:
#ifdef DEBUG_RUN_TRACE
		printf("Referencing CSI.\n");
#endif
		/* Obtain static object for the referenced CSI. */
		psobj = run->gdata; /* XXX */
		sobj = run_sobject_get(run, sym->u.csi, psobj,
		    nameref->name->sid);

		/* Return reference to the object. */
		run_reference(run, sobj, res);
		break;
	case sc_ctor:
		/* It is not possible to reference a constructor explicitly. */
		assert(b_false);
		/* Fallthrough */
	case sc_enum:
#ifdef DEBUG_RUN_TRACE
		printf("Referencing enum.\n");
#endif
		item = rdata_item_new(ic_value);
		value = rdata_value_new();
		var = rdata_var_new(vc_symbol);
		symbol_v = rdata_symbol_new();

		item->u.value = value;
		value->var = var;
		var->u.symbol_v = symbol_v;

		symbol_v->sym = sym;
		*res = item;
		break;
	case sc_deleg:
		/* XXX TODO */
		printf("Unimplemented: Delegate name reference.\n");
		abort();
		break;
	case sc_fun:
		/* There should be no global functions. */
		assert(csi != NULL);

		if (symbol_search_csi(run->program, csi, nameref->name)
		    == NULL) {
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
	case sc_prop:
#ifdef DEBUG_RUN_TRACE
		if (sym->sc == sc_var)
			printf("Referencing member variable.\n");
		else
			printf("Referencing unqualified property.\n");
#endif
		/* There should be no global variables or properties. */
		assert(csi != NULL);

		if (symbol_search_csi(run->program, csi, nameref->name)
		    == NULL && !stree_symbol_is_static(sym)) {
			/* Symbol is not in the current object. */
			printf("Error: Cannot access non-static member "
			    "variable '");
			symbol_print_fqn(sym);
			printf("' from nested CSI '");
			symbol_print_fqn(csi_sym);
			printf("'.\n");
			exit(1);
		}

		/*
		 * Determine object in which the symbol resides
		 */
		if (stree_symbol_is_static(sym)) {
			/*
			 * Class object
			 * XXX This is too slow!
			 *
			 * However fixing this is non-trivial. We would
			 * have to have pointer to static object available
			 * for each object (therefore also for each object
			 * type).
			 */
			sobj = run_sobject_find(run, sym->outer_csi);
			assert(sobj->vc == vc_object);
			aobj = sobj->u.object_v;
		} else {
			/*
			 * Instance object. Currently we don't support
			 * true inner classes, thus we know the symbol is
			 * in the active object (there is no dynamic parent).
			 */
			sobj = proc_ar->obj;
			aobj = sobj->u.object_v;
		}

		if (sym->sc == sc_var) {
			/* Find member variable in object. */
			member_var = intmap_get(&aobj->fields,
			    nameref->name->sid);
			assert(member_var != NULL);

			/* Return address of the variable. */
			item = rdata_item_new(ic_address);
			address = rdata_address_new(ac_var);
			addr_var = rdata_addr_var_new();

			item->u.address = address;
			address->u.var_a = addr_var;
			addr_var->vref = member_var;

			*res = item;
		} else {
			/* Construct named property address. */
			item = rdata_item_new(ic_address);
			address = rdata_address_new(ac_prop);
			addr_prop = rdata_addr_prop_new(apc_named);
			aprop_named = rdata_aprop_named_new();
			item->u.address = address;
			address->u.prop_a = addr_prop;
			addr_prop->u.named = aprop_named;

			deleg_p = rdata_deleg_new();
			deleg_p->obj = sobj;
			deleg_p->sym = sym;
			addr_prop->u.named->prop_d = deleg_p;

			*res = item;
		}
		break;
	}
}

/** Evaluate literal.
 *
 * @param run		Runner object
 * @param literal	Literal
 * @param res		Place to store result
 */
static void run_literal(run_t *run, stree_literal_t *literal,
    rdata_item_t **res)
{
#ifdef DEBUG_RUN_TRACE
	printf("Run literal.\n");
#endif
	switch (literal->ltc) {
	case ltc_bool:
		run_lit_bool(run, &literal->u.lit_bool, res);
		break;
	case ltc_char:
		run_lit_char(run, &literal->u.lit_char, res);
		break;
	case ltc_int:
		run_lit_int(run, &literal->u.lit_int, res);
		break;
	case ltc_ref:
		run_lit_ref(run, &literal->u.lit_ref, res);
		break;
	case ltc_string:
		run_lit_string(run, &literal->u.lit_string, res);
		break;
	}
}

/** Evaluate Boolean literal.
 *
 * @param run		Runner object
 * @param lit_bool	Boolean literal
 * @param res		Place to store result
 */
static void run_lit_bool(run_t *run, stree_lit_bool_t *lit_bool,
    rdata_item_t **res)
{
	rdata_item_t *item;
	rdata_value_t *value;
	rdata_var_t *var;
	rdata_bool_t *bool_v;

#ifdef DEBUG_RUN_TRACE
	printf("Run Boolean literal.\n");
#endif
	(void) run;

	item = rdata_item_new(ic_value);
	value = rdata_value_new();
	var = rdata_var_new(vc_bool);
	bool_v = rdata_bool_new();

	item->u.value = value;
	value->var = var;
	var->u.bool_v = bool_v;
	bool_v->value = lit_bool->value;

	*res = item;
}

/** Evaluate character literal. */
static void run_lit_char(run_t *run, stree_lit_char_t *lit_char,
    rdata_item_t **res)
{
	rdata_item_t *item;
	rdata_value_t *value;
	rdata_var_t *var;
	rdata_char_t *char_v;

#ifdef DEBUG_RUN_TRACE
	printf("Run character literal.\n");
#endif
	(void) run;

	item = rdata_item_new(ic_value);
	value = rdata_value_new();
	var = rdata_var_new(vc_char);
	char_v = rdata_char_new();

	item->u.value = value;
	value->var = var;
	var->u.char_v = char_v;
	bigint_clone(&lit_char->value, &char_v->value);

	*res = item;
}

/** Evaluate integer literal.
 *
 * @param run		Runner object
 * @param lit_int	Integer literal
 * @param res		Place to store result
 */
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

/** Evaluate reference literal (@c nil).
 *
 * @param run		Runner object
 * @param lit_ref	Reference literal
 * @param res		Place to store result
 */
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

/** Evaluate string literal.
 *
 * @param run		Runner object
 * @param lit_string	String literal
 * @param res		Place to store result
 */
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

/** Evaluate @c self reference.
 *
 * @param run		Runner object
 * @param self_ref	Self reference
 * @param res		Place to store result
 */
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

/** Evaluate binary operation.
 *
 * @param run		Runner object
 * @param binop		Binary operation
 * @param res		Place to store result
 */
static void run_binop(run_t *run, stree_binop_t *binop, rdata_item_t **res)
{
	rdata_item_t *rarg1_i, *rarg2_i;
	rdata_item_t *rarg1_vi, *rarg2_vi;
	rdata_value_t *v1, *v2;

	rarg1_i = NULL;
	rarg2_i = NULL;
	rarg1_vi = NULL;
	rarg2_vi = NULL;

#ifdef DEBUG_RUN_TRACE
	printf("Run binary operation.\n");
#endif
	run_expr(run, binop->arg1, &rarg1_i);
	if (run_is_bo(run)) {
		*res = run_recovery_item(run);
		goto cleanup;
	}

#ifdef DEBUG_RUN_TRACE
	printf("Check binop argument result.\n");
#endif
	run_cvt_value_item(run, rarg1_i, &rarg1_vi);
	if (run_is_bo(run)) {
		*res = run_recovery_item(run);
		goto cleanup;
	}

	run_expr(run, binop->arg2, &rarg2_i);
	if (run_is_bo(run)) {
		*res = run_recovery_item(run);
		goto cleanup;
	}

#ifdef DEBUG_RUN_TRACE
	printf("Check binop argument result.\n");
#endif
	run_cvt_value_item(run, rarg2_i, &rarg2_vi);
	if (run_is_bo(run)) {
		*res = run_recovery_item(run);
		goto cleanup;
	}

	v1 = rarg1_vi->u.value;
	v2 = rarg2_vi->u.value;

	if (v1->var->vc != v2->var->vc) {
		printf("Unimplemented: Binary operation arguments have "
		    "different type.\n");
		exit(1);
	}

	switch (v1->var->vc) {
	case vc_bool:
		run_binop_bool(run, binop, v1, v2, res);
		break;
	case vc_char:
		run_binop_char(run, binop, v1, v2, res);
		break;
	case vc_int:
		run_binop_int(run, binop, v1, v2, res);
		break;
	case vc_string:
		run_binop_string(run, binop, v1, v2, res);
		break;
	case vc_ref:
		run_binop_ref(run, binop, v1, v2, res);
		break;
	case vc_enum:
		run_binop_enum(run, binop, v1, v2, res);
		break;
	case vc_deleg:
	case vc_array:
	case vc_object:
	case vc_resource:
	case vc_symbol:
		assert(b_false);
	}

cleanup:
	if (rarg1_i != NULL)
		rdata_item_destroy(rarg1_i);
	if (rarg2_i != NULL)
		rdata_item_destroy(rarg2_i);
	if (rarg1_vi != NULL)
		rdata_item_destroy(rarg1_vi);
	if (rarg2_vi != NULL)
		rdata_item_destroy(rarg2_vi);
}

/** Evaluate binary operation on bool arguments.
 *
 * @param run		Runner object
 * @param binop		Binary operation
 * @param v1		Value of first argument
 * @param v2		Value of second argument
 * @param res		Place to store result
 */
static void run_binop_bool(run_t *run, stree_binop_t *binop, rdata_value_t *v1,
    rdata_value_t *v2, rdata_item_t **res)
{
	rdata_item_t *item;
	rdata_value_t *value;
	rdata_var_t *var;
	rdata_bool_t *bool_v;

	bool_t b1, b2;

	(void) run;

	item = rdata_item_new(ic_value);
	value = rdata_value_new();
	var = rdata_var_new(vc_bool);
	bool_v = rdata_bool_new();

	item->u.value = value;
	value->var = var;
	var->u.bool_v = bool_v;

	b1 = v1->var->u.bool_v->value;
	b2 = v2->var->u.bool_v->value;

	switch (binop->bc) {
	case bo_plus:
	case bo_minus:
	case bo_mult:
		assert(b_false);
		/* Fallthrough */

	case bo_equal:
		bool_v->value = (b1 == b2);
		break;
	case bo_notequal:
		bool_v->value = (b1 != b2);
		break;
	case bo_lt:
		bool_v->value = (b1 == b_false) && (b2 == b_true);
		break;
	case bo_gt:
		bool_v->value = (b1 == b_true) && (b2 == b_false);
		break;
	case bo_lt_equal:
		bool_v->value = (b1 == b_false) || (b2 == b_true);
		break;
	case bo_gt_equal:
		bool_v->value = (b1 == b_true) || (b2 == b_false);
		break;

	case bo_and:
		bool_v->value = (b1 == b_true) && (b2 == b_true);
		break;
	case bo_or:
		bool_v->value = (b1 == b_true) || (b2 == b_true);
		break;
	}

	*res = item;
}

/** Evaluate binary operation on char arguments.
 *
 * @param run		Runner object
 * @param binop		Binary operation
 * @param v1		Value of first argument
 * @param v2		Value of second argument
 * @param res		Place to store result
*/
static void run_binop_char(run_t *run, stree_binop_t *binop, rdata_value_t *v1,
    rdata_value_t *v2, rdata_item_t **res)
{
	rdata_item_t *item;
	rdata_value_t *value;
	rdata_var_t *var;
	rdata_bool_t *bool_v;

	bigint_t *c1, *c2;
	bigint_t diff;
	bool_t zf, nf;

	(void) run;

	item = rdata_item_new(ic_value);
	value = rdata_value_new();

	item->u.value = value;

	c1 = &v1->var->u.char_v->value;
	c2 = &v2->var->u.char_v->value;

	var = rdata_var_new(vc_bool);
	bool_v = rdata_bool_new();
	var->u.bool_v = bool_v;
	value->var = var;

	bigint_sub(c1, c2, &diff);
	zf = bigint_is_zero(&diff);
	nf = bigint_is_negative(&diff);

	switch (binop->bc) {
	case bo_plus:
	case bo_minus:
	case bo_mult:
		assert(b_false);
		/* Fallthrough */

	case bo_equal:
		bool_v->value = zf;
		break;
	case bo_notequal:
		bool_v->value = !zf;
		break;
	case bo_lt:
		bool_v->value = (!zf && nf);
		break;
	case bo_gt:
		bool_v->value = (!zf && !nf);
		break;
	case bo_lt_equal:
		bool_v->value = (zf || nf);
		break;
	case bo_gt_equal:
		bool_v->value = !nf;
		break;

	case bo_and:
	case bo_or:
		assert(b_false);
	}

	*res = item;
}

/** Evaluate binary operation on int arguments.
 *
 * @param run		Runner object
 * @param binop		Binary operation
 * @param v1		Value of first argument
 * @param v2		Value of second argument
 * @param res		Place to store result
*/
static void run_binop_int(run_t *run, stree_binop_t *binop, rdata_value_t *v1,
    rdata_value_t *v2, rdata_item_t **res)
{
	rdata_item_t *item;
	rdata_value_t *value;
	rdata_var_t *var;
	rdata_int_t *int_v;
	rdata_bool_t *bool_v;

	bigint_t *i1, *i2;
	bigint_t diff;
	bool_t done;
	bool_t zf, nf;

	(void) run;

	item = rdata_item_new(ic_value);
	value = rdata_value_new();

	item->u.value = value;

	i1 = &v1->var->u.int_v->value;
	i2 = &v2->var->u.int_v->value;

	done = b_true;

	switch (binop->bc) {
	case bo_plus:
		int_v = rdata_int_new();
		bigint_add(i1, i2, &int_v->value);
		break;
	case bo_minus:
		int_v = rdata_int_new();
		bigint_sub(i1, i2, &int_v->value);
		break;
	case bo_mult:
		int_v = rdata_int_new();
		bigint_mul(i1, i2, &int_v->value);
		break;
	default:
		done = b_false;
		break;
	}

	if (done) {
		var = rdata_var_new(vc_int);
		var->u.int_v = int_v;
		value->var = var;
		*res = item;
		return;
	}

	var = rdata_var_new(vc_bool);
	bool_v = rdata_bool_new();
	var->u.bool_v = bool_v;
	value->var = var;

	/* Relational operation. */

	bigint_sub(i1, i2, &diff);
	zf = bigint_is_zero(&diff);
	nf = bigint_is_negative(&diff);

	switch (binop->bc) {
	case bo_plus:
	case bo_minus:
	case bo_mult:
		assert(b_false);
		/* Fallthrough */

	case bo_equal:
		bool_v->value = zf;
		break;
	case bo_notequal:
		bool_v->value = !zf;
		break;
	case bo_lt:
		bool_v->value = (!zf && nf);
		break;
	case bo_gt:
		bool_v->value = (!zf && !nf);
		break;
	case bo_lt_equal:
		bool_v->value = (zf || nf);
		break;
	case bo_gt_equal:
		bool_v->value = !nf;
		break;
	case bo_and:
	case bo_or:
		assert(b_false);
	}

	*res = item;
}

/** Evaluate binary operation on string arguments.
 *
 * @param run		Runner object
 * @param binop		Binary operation
 * @param v1		Value of first argument
 * @param v2		Value of second argument
 * @param res		Place to store result
 */
static void run_binop_string(run_t *run, stree_binop_t *binop, rdata_value_t *v1,
    rdata_value_t *v2, rdata_item_t **res)
{
	rdata_item_t *item;
	rdata_value_t *value;
	rdata_var_t *var;
	rdata_string_t *string_v;
	rdata_bool_t *bool_v;
	bool_t done;
	bool_t zf;

	const char *s1, *s2;

	(void) run;

	item = rdata_item_new(ic_value);
	value = rdata_value_new();

	item->u.value = value;

	s1 = v1->var->u.string_v->value;
	s2 = v2->var->u.string_v->value;

	done = b_true;

	switch (binop->bc) {
	case bo_plus:
		/* Concatenate strings. */
		string_v = rdata_string_new();
		string_v->value = os_str_acat(s1, s2);
		break;
	default:
		done = b_false;
		break;
	}

	if (done) {
		var = rdata_var_new(vc_string);
		var->u.string_v = string_v;
		value->var = var;
		*res = item;
		return;
	}

	var = rdata_var_new(vc_bool);
	bool_v = rdata_bool_new();
	var->u.bool_v = bool_v;
	value->var = var;

	/* Relational operation. */

	zf = os_str_cmp(s1, s2) == 0;

	switch (binop->bc) {
	case bo_equal:
		bool_v->value = zf;
		break;
	case bo_notequal:
		bool_v->value = !zf;
		break;
	default:
		printf("Error: Invalid binary operation on string "
		    "arguments (%d).\n", binop->bc);
		assert(b_false);
	}

	*res = item;
}

/** Evaluate binary operation on ref arguments.
 *
 * @param run		Runner object
 * @param binop		Binary operation
 * @param v1		Value of first argument
 * @param v2		Value of second argument
 * @param res		Place to store result
 */
static void run_binop_ref(run_t *run, stree_binop_t *binop, rdata_value_t *v1,
    rdata_value_t *v2, rdata_item_t **res)
{
	rdata_item_t *item;
	rdata_value_t *value;
	rdata_var_t *var;
	rdata_bool_t *bool_v;

	rdata_var_t *ref1, *ref2;

	(void) run;

	item = rdata_item_new(ic_value);
	value = rdata_value_new();
	var = rdata_var_new(vc_bool);
	bool_v = rdata_bool_new();

	item->u.value = value;
	value->var = var;
	var->u.bool_v = bool_v;

	ref1 = v1->var->u.ref_v->vref;
	ref2 = v2->var->u.ref_v->vref;

	switch (binop->bc) {
	case bo_equal:
		bool_v->value = (ref1 == ref2);
		break;
	case bo_notequal:
		bool_v->value = (ref1 != ref2);
		break;
	default:
		printf("Error: Invalid binary operation on reference "
		    "arguments (%d).\n", binop->bc);
		assert(b_false);
	}

	*res = item;
}

/** Evaluate binary operation on enum arguments.
 *
 * @param run		Runner object
 * @param binop		Binary operation
 * @param v1		Value of first argument
 * @param v2		Value of second argument
 * @param res		Place to store result
 */
static void run_binop_enum(run_t *run, stree_binop_t *binop, rdata_value_t *v1,
    rdata_value_t *v2, rdata_item_t **res)
{
	rdata_item_t *item;
	rdata_value_t *value;
	rdata_var_t *var;
	rdata_bool_t *bool_v;

	stree_embr_t *e1, *e2;

	(void) run;

	item = rdata_item_new(ic_value);
	value = rdata_value_new();
	var = rdata_var_new(vc_bool);
	bool_v = rdata_bool_new();

	item->u.value = value;
	value->var = var;
	var->u.bool_v = bool_v;

	e1 = v1->var->u.enum_v->value;
	e2 = v2->var->u.enum_v->value;

	switch (binop->bc) {
	case bo_equal:
		bool_v->value = (e1 == e2);
		break;
	case bo_notequal:
		bool_v->value = (e1 != e2);
		break;
	default:
		/* Should have been caught by static typing. */
		assert(b_false);
	}

	*res = item;
}

/** Evaluate unary operation.
 *
 * @param run		Runner object
 * @param unop		Unary operation
 * @param res		Place to store result
 */
static void run_unop(run_t *run, stree_unop_t *unop, rdata_item_t **res)
{
	rdata_item_t *rarg_i;
	rdata_item_t *rarg_vi;
	rdata_value_t *val;

#ifdef DEBUG_RUN_TRACE
	printf("Run unary operation.\n");
#endif
	rarg_i = NULL;
	rarg_vi = NULL;

	run_expr(run, unop->arg, &rarg_i);
	if (run_is_bo(run)) {
		*res = run_recovery_item(run);
		goto cleanup;
	}

#ifdef DEBUG_RUN_TRACE
	printf("Check unop argument result.\n");
#endif
	run_cvt_value_item(run, rarg_i, &rarg_vi);
	if (run_is_bo(run)) {
		*res = run_recovery_item(run);
		goto cleanup;
	}

	val = rarg_vi->u.value;

	switch (val->var->vc) {
	case vc_bool:
		run_unop_bool(run, unop, val, res);
		break;
	case vc_int:
		run_unop_int(run, unop, val, res);
		break;
	default:
		printf("Unimplemented: Unrary operation argument of "
		    "type %d.\n", val->var->vc);
		run_raise_error(run);
		*res = run_recovery_item(run);
		break;
	}
cleanup:
	if (rarg_i != NULL)
		rdata_item_destroy(rarg_i);
	if (rarg_vi != NULL)
		rdata_item_destroy(rarg_vi);
}

/** Evaluate unary operation on bool argument.
 *
 * @param run		Runner object
 * @param unop		Unary operation
 * @param val		Value of argument
 * @param res		Place to store result
 */
static void run_unop_bool(run_t *run, stree_unop_t *unop, rdata_value_t *val,
    rdata_item_t **res)
{
	rdata_item_t *item;
	rdata_value_t *value;
	rdata_var_t *var;
	rdata_bool_t *bool_v;

	(void) run;

	item = rdata_item_new(ic_value);
	value = rdata_value_new();
	var = rdata_var_new(vc_bool);
	bool_v = rdata_bool_new();

	item->u.value = value;
	value->var = var;
	var->u.bool_v = bool_v;

	switch (unop->uc) {
	case uo_plus:
	case uo_minus:
		assert(b_false);
		/* Fallthrough */

	case uo_not:
		bool_v->value = !val->var->u.bool_v->value;
		break;
	}

	*res = item;
}

/** Evaluate unary operation on int argument.
 *
 * @param run		Runner object
 * @param unop		Unary operation
 * @param val		Value of argument
 * @param res		Place to store result
 */
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
	case uo_not:
		assert(b_false);
	}

	*res = item;
}

/** Run equality comparison of two values
 *
 * This should be equivalent to equality ('==') binary operation.
 * XXX Duplicating code of run_binop_xxx().
 *
 * @param run		Runner object
 * @param v1		Value of first argument
 * @param v2		Value of second argument
 * @param res		Place to store result (plain boolean value)
 */
void run_equal(run_t *run, rdata_value_t *v1, rdata_value_t *v2, bool_t *res)
{
	bool_t b1, b2;
	bigint_t *c1, *c2;
	bigint_t *i1, *i2;
	bigint_t diff;
	const char *s1, *s2;
	rdata_var_t *ref1, *ref2;
	stree_embr_t *e1, *e2;

	(void) run;
	assert(v1->var->vc == v2->var->vc);

	switch (v1->var->vc) {
	case vc_bool:
		b1 = v1->var->u.bool_v->value;
		b2 = v2->var->u.bool_v->value;

		*res = (b1 == b2);
		break;
	case vc_char:
		c1 = &v1->var->u.char_v->value;
		c2 = &v2->var->u.char_v->value;

    		bigint_sub(c1, c2, &diff);
		*res = bigint_is_zero(&diff);
		break;
	case vc_int:
		i1 = &v1->var->u.int_v->value;
		i2 = &v2->var->u.int_v->value;

		bigint_sub(i1, i2, &diff);
		*res = bigint_is_zero(&diff);
		break;
	case vc_string:
		s1 = v1->var->u.string_v->value;
		s2 = v2->var->u.string_v->value;

		*res = os_str_cmp(s1, s2) == 0;
		break;
	case vc_ref:
		ref1 = v1->var->u.ref_v->vref;
		ref2 = v2->var->u.ref_v->vref;

		*res = (ref1 == ref2);
		break;
	case vc_enum:
		e1 = v1->var->u.enum_v->value;
		e2 = v2->var->u.enum_v->value;

		*res = (e1 == e2);
		break;

	case vc_deleg:
	case vc_array:
	case vc_object:
	case vc_resource:
	case vc_symbol:
		assert(b_false);
	}
}


/** Evaluate @c new operation.
 *
 * Evaluates operation per the @c new operator that creates a new
 * instance of some type.
 *
 * @param run		Runner object
 * @param unop		Unary operation
 * @param res		Place to store result
 */
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

/** Create new array.
 *
 * @param run		Runner object
 * @param new_op	New operation
 * @param titem		Type of new var node (tic_tarray)
 * @param res		Place to store result
 */
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
	errno_t rc;
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
			*res = run_recovery_item(run);
			return;
		}

		run_cvt_value_item(run, rexpr, &rexpr_vi);
		if (run_is_bo(run)) {
			*res = run_recovery_item(run);
			return;
		}

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
		/* Create and initialize element. */
		run_var_new(run, tarray->base_ti, &elem_var);

		array->element[i] = elem_var;
	}

	/* Create array variable. */
	array_var = rdata_var_new(vc_array);
	array_var->u.array_v = array;

	/* Create reference to the new array. */
	run_reference(run, array_var, res);
}

/** Create new object.
 *
 * @param run		Runner object
 * @param new_op	New operation
 * @param titem		Type of new var node (tic_tobject)
 * @param res		Place to store result
 */
static void run_new_object(run_t *run, stree_new_t *new_op,
    tdata_item_t *titem, rdata_item_t **res)
{
	stree_csi_t *csi;
	rdata_item_t *obj_i;
	list_t arg_vals;

#ifdef DEBUG_RUN_TRACE
	printf("Create new object.\n");
#endif
	/* Lookup object CSI. */
	assert(titem->tic == tic_tobject);
	csi = titem->u.tobject->csi;

	/* Evaluate constructor arguments. */
	run_call_args(run, &new_op->ctor_args, &arg_vals);
	if (run_is_bo(run)) {
		*res = run_recovery_item(run);
		return;
	}

	/* Create CSI instance. */
	run_new_csi_inst_ref(run, csi, sn_nonstatic, res);

	/* Run the constructor. */
	run_dereference(run, *res, NULL, &obj_i);
	assert(obj_i->ic == ic_address);
	assert(obj_i->u.address->ac == ac_var);
	run_object_ctor(run, obj_i->u.address->u.var_a->vref, &arg_vals);
	rdata_item_destroy(obj_i);

	/* Destroy argument values */
	run_destroy_arg_vals(&arg_vals);
}

/** Evaluate member acccess.
 *
 * Evaluate operation per the member access ('.') operator.
 *
 * @param run		Runner object
 * @param access	Access operation
 * @param res		Place to store result
 */
static void run_access(run_t *run, stree_access_t *access, rdata_item_t **res)
{
	rdata_item_t *rarg;

#ifdef DEBUG_RUN_TRACE
	printf("Run access operation.\n");
#endif
	rarg = NULL;

	run_expr(run, access->arg, &rarg);
	if (run_is_bo(run)) {
		*res = run_recovery_item(run);
		goto cleanup;
    	}

	if (rarg == NULL) {
		printf("Error: Sub-expression has no value.\n");
		exit(1);
	}

	run_access_item(run, access, rarg, res);
cleanup:
	if (rarg != NULL)
		rdata_item_destroy(rarg);
}

/** Evaluate member acccess (with base already evaluated).
 *
 * @param run		Runner object
 * @param access	Access operation
 * @param arg		Evaluated base expression
 * @param res		Place to store result
 */
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
	case vc_symbol:
		run_access_symbol(run, access, arg, res);
		break;

	case vc_bool:
	case vc_char:
	case vc_enum:
	case vc_int:
	case vc_string:
	case vc_array:
	case vc_resource:
		printf("Unimplemented: Using access operator ('.') "
		    "with unsupported data type (value/%d).\n", vc);
		exit(1);
	}
}

/** Evaluate reference acccess.
 *
 * @param run		Runner object
 * @param access	Access operation
 * @param arg		Evaluated base expression
 * @param res		Place to store result
 */
static void run_access_ref(run_t *run, stree_access_t *access,
    rdata_item_t *arg, rdata_item_t **res)
{
	rdata_item_t *darg;

	/* Implicitly dereference. */
	run_dereference(run, arg, access->arg->cspan, &darg);

	if (run->thread_ar->bo_mode != bm_none) {
		*res = run_recovery_item(run);
		return;
	}

	/* Try again. */
	run_access_item(run, access, darg, res);

	/* Destroy temporary */
	rdata_item_destroy(darg);
}

/** Evaluate delegate member acccess.
 *
 * @param run		Runner object
 * @param access	Access operation
 * @param arg		Evaluated base expression
 * @param res		Place to store result
 */
static void run_access_deleg(run_t *run, stree_access_t *access,
    rdata_item_t *arg, rdata_item_t **res)
{
	(void) run;
	(void) access;
	(void) arg;
	(void) res;

	printf("Error: Using '.' with delegate.\n");
	exit(1);
}

/** Evaluate object member acccess.
 *
 * @param run		Runner object
 * @param access	Access operation
 * @param arg		Evaluated base expression
 * @param res		Place to store result
 */
static void run_access_object(run_t *run, stree_access_t *access,
    rdata_item_t *arg, rdata_item_t **res)
{
	rdata_var_t *obj_var;
	rdata_object_t *object;

#ifdef DEBUG_RUN_TRACE
	printf("Run object access operation.\n");
#endif
	assert(arg->ic == ic_address);
	assert(arg->u.address->ac == ac_var);

	obj_var = arg->u.address->u.var_a->vref;
	assert(obj_var->vc == vc_object);

	object = obj_var->u.object_v;

	if (object->static_obj == sn_static)
		run_access_object_static(run, access, obj_var, res);
	else
		run_access_object_nonstatic(run, access, obj_var, res);
}

/** Evaluate static object member acccess.
 *
 * @param run		Runner object
 * @param access	Access operation
 * @param arg		Evaluated base expression
 * @param res		Place to store result
 */
static void run_access_object_static(run_t *run, stree_access_t *access,
    rdata_var_t *obj_var, rdata_item_t **res)
{
	rdata_object_t *object;
	stree_symbol_t *member;
	stree_csi_t *member_csi;

	rdata_deleg_t *deleg_v;
	rdata_item_t *ritem;
	rdata_value_t *rvalue;
	rdata_var_t *rvar;
	rdata_address_t *address;
	rdata_addr_var_t *addr_var;
	rdata_addr_prop_t *addr_prop;
	rdata_aprop_named_t *aprop_named;
	rdata_deleg_t *deleg_p;
	rdata_var_t *mvar;

#ifdef DEBUG_RUN_TRACE
	printf("Run static object access operation.\n");
#endif
	assert(obj_var->vc == vc_object);
	object = obj_var->u.object_v;

	assert(object->static_obj == sn_static);

	member = symbol_search_csi(run->program, object->class_sym->u.csi,
	    access->member_name);

	/* Member existence should be ensured by static type checking. */
	assert(member != NULL);

#ifdef DEBUG_RUN_TRACE
	printf("Found member '%s'.\n",
	    strtab_get_str(access->member_name->sid));
#endif

	switch (member->sc) {
	case sc_csi:
		/* Get child static object. */
		member_csi = symbol_to_csi(member);
		assert(member_csi != NULL);

		mvar = run_sobject_get(run, member_csi, obj_var,
		    access->member_name->sid);

		ritem = rdata_item_new(ic_address);
		address = rdata_address_new(ac_var);
		ritem->u.address = address;

		addr_var = rdata_addr_var_new();
		address->u.var_a = addr_var;
		addr_var->vref = mvar;

		*res = ritem;
		break;
	case sc_ctor:
		/* It is not possible to reference a constructor explicitly. */
		assert(b_false);
		/* Fallthrough */
	case sc_deleg:
		printf("Error: Accessing object member which is a delegate.\n");
		exit(1);
	case sc_enum:
		printf("Error: Accessing object member which is an enum.\n");
		exit(1);
	case sc_fun:
		/* Construct anonymous delegate. */
		ritem = rdata_item_new(ic_value);
		rvalue = rdata_value_new();
		ritem->u.value = rvalue;

		rvar = rdata_var_new(vc_deleg);
		rvalue->var = rvar;

		deleg_v = rdata_deleg_new();
		rvar->u.deleg_v = deleg_v;

		deleg_v->obj = obj_var;
		deleg_v->sym = member;
		*res = ritem;
		break;
	case sc_var:
		/* Get static object member variable. */
		mvar = intmap_get(&object->fields, access->member_name->sid);

		ritem = rdata_item_new(ic_address);
		address = rdata_address_new(ac_var);
		ritem->u.address = address;

		addr_var = rdata_addr_var_new();
		address->u.var_a = addr_var;
		addr_var->vref = mvar;

		*res = ritem;
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
		deleg_p->obj = obj_var;
		deleg_p->sym = member;
		addr_prop->u.named->prop_d = deleg_p;

		*res = ritem;
		break;
	}
}

/** Evaluate object member acccess.
 *
 * @param run		Runner object
 * @param access	Access operation
 * @param arg		Evaluated base expression
 * @param res		Place to store result
 */
static void run_access_object_nonstatic(run_t *run, stree_access_t *access,
    rdata_var_t *obj_var, rdata_item_t **res)
{
	rdata_object_t *object;
	stree_symbol_t *member;
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
	printf("Run nonstatic object access operation.\n");
#endif
	assert(obj_var->vc == vc_object);
	object = obj_var->u.object_v;

	assert(object->static_obj == sn_nonstatic);

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

	/* Make compiler happy. */
	ritem = NULL;

	switch (member->sc) {
	case sc_csi:
		printf("Error: Accessing object member which is nested CSI.\n");
		exit(1);
	case sc_ctor:
		/* It is not possible to reference a constructor explicitly. */
		assert(b_false);
		/* Fallthrough */
	case sc_deleg:
		printf("Error: Accessing object member which is a delegate.\n");
		exit(1);
	case sc_enum:
		printf("Error: Accessing object member which is an enum.\n");
		exit(1);
	case sc_fun:
		/* Construct anonymous delegate. */
		ritem = rdata_item_new(ic_value);
		value = rdata_value_new();
		ritem->u.value = value;

		var = rdata_var_new(vc_deleg);
		value->var = var;
		deleg_v = rdata_deleg_new();
		var->u.deleg_v = deleg_v;

		deleg_v->obj = obj_var;
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
		deleg_p->obj = obj_var;
		deleg_p->sym = member;
		addr_prop->u.named->prop_d = deleg_p;
		break;
	}

	*res = ritem;
}

/** Evaluate symbol member acccess.
 *
 * @param run		Runner object
 * @param access	Access operation
 * @param arg		Evaluated base expression
 * @param res		Place to store result
 */
static void run_access_symbol(run_t *run, stree_access_t *access,
    rdata_item_t *arg, rdata_item_t **res)
{
	rdata_item_t *arg_vi;
	rdata_value_t *arg_val;
	rdata_symbol_t *symbol_v;
	stree_embr_t *embr;

	rdata_item_t *ritem;
	rdata_value_t *rvalue;
	rdata_var_t *rvar;
	rdata_enum_t *enum_v;

#ifdef DEBUG_RUN_TRACE
	printf("Run symbol access operation.\n");
#endif
	run_cvt_value_item(run, arg, &arg_vi);
	if (run_is_bo(run)) {
		*res = run_recovery_item(run);
		return;
	}

	arg_val = arg_vi->u.value;
	assert(arg_val->var->vc == vc_symbol);

	symbol_v = arg_val->var->u.symbol_v;

	/* XXX Port CSI symbol reference to using vc_symbol */
	assert(symbol_v->sym->sc == sc_enum);

	embr = stree_enum_find_mbr(symbol_v->sym->u.enum_d,
	    access->member_name);

	rdata_item_destroy(arg_vi);

	/* Member existence should be ensured by static type checking. */
	assert(embr != NULL);

#ifdef DEBUG_RUN_TRACE
	printf("Found enum member '%s'.\n",
	    strtab_get_str(access->member_name->sid));
#endif
	ritem = rdata_item_new(ic_value);
	rvalue = rdata_value_new();
	rvar = rdata_var_new(vc_enum);
	enum_v = rdata_enum_new();

	ritem->u.value = rvalue;
	rvalue->var = rvar;
	rvar->u.enum_v = enum_v;
	enum_v->value = embr;

	*res = ritem;
}

/** Call a function.
 *
 * Call a function and return the result in @a res.
 *
 * @param run		Runner object
 * @param call		Call operation
 * @param res		Place to store result
 */
static void run_call(run_t *run, stree_call_t *call, rdata_item_t **res)
{
	rdata_item_t *rdeleg, *rdeleg_vi;
	rdata_deleg_t *deleg_v;
	list_t arg_vals;

	stree_fun_t *fun;
	run_proc_ar_t *proc_ar;

#ifdef DEBUG_RUN_TRACE
	printf("Run call operation.\n");
#endif
	rdeleg = NULL;
	rdeleg_vi = NULL;

	run_expr(run, call->fun, &rdeleg);
	if (run_is_bo(run)) {
		*res = run_recovery_item(run);
		goto cleanup;
	}

	run_cvt_value_item(run, rdeleg, &rdeleg_vi);
	if (run_is_bo(run)) {
		*res = run_recovery_item(run);
		goto cleanup;
	}

	assert(rdeleg_vi->ic == ic_value);

	if (rdeleg_vi->u.value->var->vc != vc_deleg) {
		printf("Unimplemented: Call expression of this type (");
		rdata_item_print(rdeleg_vi);
		printf(").\n");
		exit(1);
	}

	deleg_v = rdeleg_vi->u.value->var->u.deleg_v;

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
	run_call_args(run, &call->args, &arg_vals);
	if (run_is_bo(run)) {
		*res = run_recovery_item(run);
		goto cleanup;
	}

	fun = symbol_to_fun(deleg_v->sym);
	assert(fun != NULL);

	/* Create procedure activation record. */
	run_proc_ar_create(run, deleg_v->obj, fun->proc, &proc_ar);

	/* Fill in argument values. */
	run_proc_ar_set_args(run, proc_ar, &arg_vals);

	/* Destroy arg_vals, they are no longer needed. */
	run_destroy_arg_vals(&arg_vals);

	/* Run the function. */
	run_proc(run, proc_ar, res);

	if (!run_is_bo(run) && fun->sig->rtype != NULL && *res == NULL) {
		printf("Error: Function '");
		symbol_print_fqn(deleg_v->sym);
		printf("' did not return a value.\n");
		exit(1);
	}

	/* Destroy procedure activation record. */
	run_proc_ar_destroy(run, proc_ar);

cleanup:
	if (rdeleg != NULL)
		rdata_item_destroy(rdeleg);
	if (rdeleg_vi != NULL)
		rdata_item_destroy(rdeleg_vi);

#ifdef DEBUG_RUN_TRACE
	printf("Returned from function call.\n");
#endif
}

/** Evaluate call arguments.
 *
 * Evaluate arguments to function or constructor.
 *
 * @param run		Runner object
 * @param args		Real arguments (list of stree_expr_t)
 * @param arg_vals	Address of uninitialized list to store argument values
 *			(list of rdata_item_t).
 */
static void run_call_args(run_t *run, list_t *args, list_t *arg_vals)
{
	list_node_t *arg_n;
	stree_expr_t *arg;
	rdata_item_t *rarg_i, *rarg_vi;

	/* Evaluate function arguments. */
	list_init(arg_vals);
	arg_n = list_first(args);

	while (arg_n != NULL) {
		arg = list_node_data(arg_n, stree_expr_t *);
		run_expr(run, arg, &rarg_i);
		if (run_is_bo(run))
			goto error;

		run_cvt_value_item(run, rarg_i, &rarg_vi);
		rdata_item_destroy(rarg_i);
		if (run_is_bo(run))
			goto error;

		list_append(arg_vals, rarg_vi);
		arg_n = list_next(args, arg_n);
	}
	return;

error:
	/*
	 * An exception or error occured while evaluating one of the
	 * arguments. Destroy already obtained argument values and
	 * dismantle the list.
	 */
	run_destroy_arg_vals(arg_vals);
}

/** Destroy list of evaluated arguments.
 *
 * Provided a list of evaluated arguments, destroy them, removing them
 * from the list and fini the list itself.
 *
 * @param arg_vals	List of evaluated arguments (value items,
 *			rdata_item_t).
 */
static void run_destroy_arg_vals(list_t *arg_vals)
{
	list_node_t *val_n;
	rdata_item_t *val_i;

	/*
	 * An exception or error occured while evaluating one of the
	 * arguments. Destroy already obtained argument values and
	 * dismantle the list.
	 */
	while (!list_is_empty(arg_vals)) {
		val_n = list_first(arg_vals);
		val_i = list_node_data(val_n, rdata_item_t *);

		rdata_item_destroy(val_i);
		list_remove(arg_vals, val_n);
	}
	list_fini(arg_vals);
}

/** Run index operation.
 *
 * Evaluate operation per the indexing ('[', ']') operator.
 *
 * @param run		Runner object
 * @param index		Index operation
 * @param res		Place to store result
 */
static void run_index(run_t *run, stree_index_t *index, rdata_item_t **res)
{
	rdata_item_t *rbase;
	rdata_item_t *base_i;
	list_node_t *node;
	stree_expr_t *arg;
	rdata_item_t *rarg_i, *rarg_vi;
	var_class_t vc;
	list_t arg_vals;
	list_node_t *val_n;
	rdata_item_t *val_i;

#ifdef DEBUG_RUN_TRACE
	printf("Run index operation.\n");
#endif
	run_expr(run, index->base, &rbase);
	if (run_is_bo(run)) {
		*res = run_recovery_item(run);
		return;
	}

	vc = run_item_get_vc(run, rbase);

	/* Implicitly dereference. */
	if (vc == vc_ref) {
		run_dereference(run, rbase, index->base->cspan, &base_i);
		rdata_item_destroy(rbase);
		if (run_is_bo(run)) {
			*res = run_recovery_item(run);
			return;
		}
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
			*res = run_recovery_item(run);
			goto cleanup;
		}

		run_cvt_value_item(run, rarg_i, &rarg_vi);
		rdata_item_destroy(rarg_i);
		if (run_is_bo(run)) {
			*res = run_recovery_item(run);
			goto cleanup;
		}

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

	/* Destroy the indexing base temporary */
	rdata_item_destroy(base_i);
cleanup:
	/*
	 * An exception or error occured while evaluating one of the
	 * arguments. Destroy already obtained argument values and
	 * dismantle the list.
	 */
	while (!list_is_empty(&arg_vals)) {
		val_n = list_first(&arg_vals);
		val_i = list_node_data(val_n, rdata_item_t *);

		rdata_item_destroy(val_i);
		list_remove(&arg_vals, val_n);
	}

	list_fini(&arg_vals);
}

/** Run index operation on array.
 *
 * @param run		Runner object
 * @param index		Index operation
 * @param base		Evaluated base expression
 * @param args		Evaluated indices (list of rdata_item_t)
 * @param res		Place to store result
 */
static void run_index_array(run_t *run, stree_index_t *index,
    rdata_item_t *base, list_t *args, rdata_item_t **res)
{
	list_node_t *node;
	rdata_array_t *array;
	rdata_item_t *arg;

	int i;
	int elem_index;
	int arg_val;
	errno_t rc;

	rdata_item_t *ritem;
	rdata_address_t *address;
	rdata_addr_var_t *addr_var;

#ifdef DEBUG_RUN_TRACE
	printf("Run array index operation.\n");
#endif
	(void) run;

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
			    run->program->builtin->error_outofbounds,
			    index->expr->cspan);
			/* XXX It should be cspan of the argument. */
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

/** Index an object (via its indexer).
 *
 * @param run		Runner object
 * @param index		Index operation
 * @param base		Evaluated base expression
 * @param args		Evaluated indices (list of rdata_item_t)
 * @param res		Place to store result
 */
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
	rdata_item_t *arg, *arg_copy;

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

		/*
		 * Clone argument so that original can
		 * be freed.
		 */
		assert(arg->ic == ic_value);
		arg_copy = rdata_item_new(ic_value);
		rdata_value_copy(arg->u.value, &arg_copy->u.value);

		list_append(&aprop_indexed->args, arg_copy);
		node = list_next(args, node);
	}

	*res = ritem;
}

/** Run index operation on string.
 *
 * @param run		Runner object
 * @param index		Index operation
 * @param base		Evaluated base expression
 * @param args		Evaluated indices (list of rdata_item_t)
 * @param res		Place to store result
 */
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
	errno_t rc1, rc2;

	rdata_value_t *value;
	rdata_var_t *cvar;
	rdata_item_t *ritem;
	int cval;

#ifdef DEBUG_RUN_TRACE
	printf("Run string index operation.\n");
#endif
	(void) run;

	run_cvt_value_item(run, base, &base_vi);
	if (run_is_bo(run)) {
		*res = run_recovery_item(run);
		return;
	}

	assert(base_vi->u.value->var->vc == vc_string);
	string = base_vi->u.value->var->u.string_v;

	/*
	 * Linear index of the desired element. Elements are stored in
	 * lexicographic order with the last index changing the fastest.
	 */
	node = list_first(args);
	elem_index = 0;

	assert(node != NULL);

	i = 0;
	do {
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
	} while (node != NULL);

	if (i < 1) {
		printf("Error: Too few indices for string.\n");
		exit(1);
	}

	if (rc1 == EOK)
		rc2 = os_str_get_char(string->value, elem_index, &cval);
	else
		rc2 = EOK;

	if (rc1 != EOK || rc2 != EOK) {
#ifdef DEBUG_RUN_TRACE
		printf("Error: String index (value: %d) is out of range.\n",
		    arg_val);
#endif
		/* Raise Error.OutOfBounds */
		run_raise_exc(run, run->program->builtin->error_outofbounds,
		    index->expr->cspan);
		*res = run_recovery_item(run);
		goto cleanup;
	}

	/* Construct character value. */
	ritem = rdata_item_new(ic_value);
	value = rdata_value_new();
	ritem->u.value = value;

	cvar = rdata_var_new(vc_char);
	cvar->u.char_v = rdata_char_new();
	bigint_init(&cvar->u.char_v->value, cval);
	value->var = cvar;

	*res = ritem;
cleanup:
	rdata_item_destroy(base_vi);
}

/** Run assignment.
 *
 * Executes an assignment. @c NULL is always stored to @a res because
 * an assignment does not have a value.
 *
 * @param run		Runner object
 * @param assign	Assignment expression
 * @param res		Place to store result
*/
static void run_assign(run_t *run, stree_assign_t *assign, rdata_item_t **res)
{
	rdata_item_t *rdest_i, *rsrc_i;
	rdata_item_t *rsrc_vi;

#ifdef DEBUG_RUN_TRACE
	printf("Run assign operation.\n");
#endif
	rdest_i = NULL;
	rsrc_i = NULL;
	rsrc_vi = NULL;

	run_expr(run, assign->dest, &rdest_i);
	if (run_is_bo(run)) {
		*res = run_recovery_item(run);
		goto cleanup;
	}

	run_expr(run, assign->src, &rsrc_i);
	if (run_is_bo(run)) {
		*res = run_recovery_item(run);
		goto cleanup;
	}

	run_cvt_value_item(run, rsrc_i, &rsrc_vi);
	if (run_is_bo(run)) {
		*res = run_recovery_item(run);
		goto cleanup;
	}

	assert(rsrc_vi->ic == ic_value);

	if (rdest_i->ic != ic_address) {
		printf("Error: Address expression required on left side of "
		    "assignment operator.\n");
		exit(1);
	}

	run_address_write(run, rdest_i->u.address, rsrc_vi->u.value);

	*res = NULL;
cleanup:
	if (rdest_i != NULL)
		rdata_item_destroy(rdest_i);
	if (rsrc_i != NULL)
		rdata_item_destroy(rsrc_i);
	if (rsrc_vi != NULL)
		rdata_item_destroy(rsrc_vi);
}

/** Execute @c as conversion.
 *
 * @param run		Runner object
 * @param as_op		@c as conversion expression
 * @param res		Place to store result
 */
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
		*res = run_recovery_item(run);
		return;
	}

	/*
	 * This should always be a reference if the argument is indeed
	 * a class instance.
	 */
	assert(run_item_get_vc(run, rarg_i) == vc_ref);
	run_cvt_value_item(run, rarg_i, &rarg_vi);
	rdata_item_destroy(rarg_i);

	if (run_is_bo(run)) {
		*res = run_recovery_item(run);
		return;
	}

	assert(rarg_vi->ic == ic_value);

	if (rarg_vi->u.value->var->u.ref_v->vref == NULL) {
		/* Nil reference is always okay. */
		*res = rarg_vi;
		return;
	}

	run_dereference(run, rarg_vi, NULL, &rarg_di);

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

	/* The dereferenced item is not used anymore. */
	rdata_item_destroy(rarg_di);

	*res = rarg_vi;
}

/** Execute boxing operation.
 *
 * XXX We can scrap this special operation once we have constructors.
 *
 * @param run		Runner object
 * @param box		Boxing operation
 * @param res		Place to store result
 */
static void run_box(run_t *run, stree_box_t *box, rdata_item_t **res)
{
	rdata_item_t *rarg_i;
	rdata_item_t *rarg_vi;

	stree_symbol_t *csi_sym;
	stree_csi_t *csi;
	builtin_t *bi;
	rdata_var_t *var;
	rdata_object_t *object;

	sid_t mbr_name_sid;
	rdata_var_t *mbr_var;

#ifdef DEBUG_RUN_TRACE
	printf("Run boxing operation.\n");
#endif
	run_expr(run, box->arg, &rarg_i);
	if (run_is_bo(run)) {
		*res = run_recovery_item(run);
		return;
	}

	run_cvt_value_item(run, rarg_i, &rarg_vi);
	rdata_item_destroy(rarg_i);
	if (run_is_bo(run)) {
		*res = run_recovery_item(run);
		return;
	}

	assert(rarg_vi->ic == ic_value);

	bi = run->program->builtin;

	/* Just to keep the compiler happy. */
	csi_sym = NULL;

	switch (rarg_vi->u.value->var->vc) {
	case vc_bool: csi_sym = bi->boxed_bool; break;
	case vc_char: csi_sym = bi->boxed_char; break;
	case vc_int: csi_sym = bi->boxed_int; break;
	case vc_string: csi_sym = bi->boxed_string; break;

	case vc_ref:
	case vc_deleg:
	case vc_enum:
	case vc_array:
	case vc_object:
	case vc_resource:
	case vc_symbol:
		assert(b_false);
	}

	csi = symbol_to_csi(csi_sym);
	assert(csi != NULL);

	/* Construct object of the relevant boxed type. */
	run_new_csi_inst_ref(run, csi, sn_nonstatic, res);

	/* Set the 'Value' field */

	assert((*res)->ic == ic_value);
	assert((*res)->u.value->var->vc == vc_ref);
	var = (*res)->u.value->var->u.ref_v->vref;
	assert(var->vc == vc_object);
	object = var->u.object_v;

	mbr_name_sid = strtab_get_sid("Value");
	mbr_var = intmap_get(&object->fields, mbr_name_sid);
	assert(mbr_var != NULL);

	rdata_var_write(mbr_var, rarg_vi->u.value);
	rdata_item_destroy(rarg_vi);
}

/** Create new CSI instance and return reference to it.
 *
 * Create a new object, instance of @a csi.
 * XXX This does not work with generics as @a csi cannot specify a generic
 * type.
 *
 * Initialize the fields with default values of their types, but do not
 * run any constructor.
 *
 * If @a sn is @c sn_nonstatic a regular object is created, containing all
 * non-static member variables. If @a sn is @c sn_static a static object
 * is created, containing all static member variables.
 *
 * @param run		Runner object
 * @param csi		CSI to create instance of
 * @param sn		@c sn_static to create a static (class) object,
 *			@c sn_nonstatic to create a regular object
 * @param res		Place to store result
 */
void run_new_csi_inst_ref(run_t *run, stree_csi_t *csi, statns_t sn,
    rdata_item_t **res)
{
	rdata_var_t *obj_var;

	/* Create object. */
	run_new_csi_inst(run, csi, sn, &obj_var);

	/* Create reference to the new object. */
	run_reference(run, obj_var, res);
}

/** Create new CSI instance.
 *
 * Create a new object, instance of @a csi.
 * XXX This does not work with generics as @a csi cannot specify a generic
 * type.
 *
 * Initialize the fields with default values of their types, but do not
 * run any constructor.
 *
 * If @a sn is @c sn_nonstatic a regular object is created, containing all
 * non-static member variables. If @a sn is @c sn_static a static object
 * is created, containing all static member variables.
 *
 * @param run		Runner object
 * @param csi		CSI to create instance of
 * @param sn		@c sn_static to create a static (class) object,
 *			@c sn_nonstatic to create a regular object
 * @param res		Place to store result
 */
void run_new_csi_inst(run_t *run, stree_csi_t *csi, statns_t sn,
    rdata_var_t **res)
{
	rdata_object_t *obj;
	rdata_var_t *obj_var;

	stree_symbol_t *csi_sym;
	stree_csimbr_t *csimbr;
	stree_var_t *var;
	statns_t var_sn;

	rdata_var_t *mbr_var;
	list_node_t *node;
	tdata_item_t *field_ti;

	csi_sym = csi_to_symbol(csi);

#ifdef DEBUG_RUN_TRACE
	printf("Create new instance of CSI '");
	symbol_print_fqn(csi_sym);
	printf("'.\n");
#endif

	/* Create the object. */
	obj = rdata_object_new();
	obj->class_sym = csi_sym;
	obj->static_obj = sn;
	intmap_init(&obj->fields);

	obj_var = rdata_var_new(vc_object);
	obj_var->u.object_v = obj;

	/* For this CSI and all base CSIs */
	while (csi != NULL) {

		/* For all members */
		node = list_first(&csi->members);
		while (node != NULL) {
			csimbr = list_node_data(node, stree_csimbr_t *);

			/* Is it a member variable? */
			if (csimbr->cc == csimbr_var) {
				var = csimbr->u.var;

				/* Is it static/nonstatic? */
				var_sn = stree_symbol_has_attr(
				    var_to_symbol(var), sac_static) ? sn_static : sn_nonstatic;
				if (var_sn == sn) {
					/* Compute field type. XXX Memoize. */
					run_texpr(run->program, csi, var->type,
					    &field_ti);

					/* Create and initialize field. */
					run_var_new(run, field_ti, &mbr_var);

					/* Add to field map. */
					intmap_set(&obj->fields, var->name->sid,
					    mbr_var);
				}
			}

			node = list_next(&csi->members, node);
		}

		/* Continue with base CSI */
		csi = csi->base_csi;
	}

	*res = obj_var;
}

/** Run constructor on an object.
 *
 * @param run		Runner object
 * @param obj		Object to run constructor on
 * @param arg_vals	Argument values (list of rdata_item_t)
 */
static void run_object_ctor(run_t *run, rdata_var_t *obj, list_t *arg_vals)
{
	stree_ident_t *ctor_ident;
	stree_symbol_t *csi_sym;
	stree_csi_t *csi;
	stree_symbol_t *ctor_sym;
	stree_ctor_t *ctor;
	run_proc_ar_t *proc_ar;
	rdata_item_t *res;

	csi_sym = obj->u.object_v->class_sym;
	csi = symbol_to_csi(csi_sym);
	assert(csi != NULL);

#ifdef DEBUG_RUN_TRACE
	printf("Run object constructor from CSI '");
	symbol_print_fqn(csi_sym);
	printf("'.\n");
#endif
	ctor_ident = stree_ident_new();
	ctor_ident->sid = strtab_get_sid(CTOR_IDENT);

	/* Find constructor. */
	ctor_sym = symbol_search_csi_no_base(run->program, csi, ctor_ident);
	if (ctor_sym == NULL) {
#ifdef DEBUG_RUN_TRACE
		printf("No constructor found.\n");
#endif
		return;
	}

	ctor = symbol_to_ctor(ctor_sym);
	assert(ctor != NULL);

	/* Create procedure activation record. */
	run_proc_ar_create(run, obj, ctor->proc, &proc_ar);

	/* Fill in argument values. */
	run_proc_ar_set_args(run, proc_ar, arg_vals);

	/* Run the procedure. */
	run_proc(run, proc_ar, &res);

	/* Constructor does not return a value. */
	assert(res == NULL);

	/* Destroy procedure activation record. */
	run_proc_ar_destroy(run, proc_ar);

#ifdef DEBUG_RUN_TRACE
	printf("Returned from constructor..\n");
#endif
}

/** Return boolean value of an item.
 *
 * Try to interpret @a item as a boolean value. If it is not a boolean
 * value, generate an error.
 *
 * @param run		Runner object
 * @param item		Input item
 * @return		Resulting boolean value
 */
bool_t run_item_boolean_value(run_t *run, rdata_item_t *item)
{
	rdata_item_t *vitem;
	rdata_var_t *var;
	bool_t res;

	(void) run;
	run_cvt_value_item(run, item, &vitem);
	if (run_is_bo(run))
		return b_true;

	assert(vitem->ic == ic_value);
	var = vitem->u.value->var;

	assert(var->vc == vc_bool);
	res = var->u.bool_v->value;

	/* Free value item */
	rdata_item_destroy(vitem);
	return res;
}
