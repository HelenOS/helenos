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
static void run_lit_string(run_t *run, stree_lit_string_t *lit_string,
    rdata_item_t **res);
static void run_binop(run_t *run, stree_binop_t *binop, rdata_item_t **res);
static void run_unop(run_t *run, stree_unop_t *unop, rdata_item_t **res);
static void run_access(run_t *run, stree_access_t *access, rdata_item_t **res);
static void run_call(run_t *run, stree_call_t *call, rdata_item_t **res);
static void run_assign(run_t *run, stree_assign_t *assign, rdata_item_t **res);

static void run_address_read(run_t *run, rdata_address_t *address,
    rdata_item_t **ritem);
static void run_address_write(run_t *run, rdata_address_t *address,
    rdata_value_t *value);

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
	case ec_binop:
		run_binop(run, expr->u.binop, res);
		break;
	case ec_unop:
		run_unop(run, expr->u.unop, res);
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
	rdata_value_t *val;
	rdata_var_t *var;
	rdata_deleg_t *deleg_v;

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

	/* XXX Start lookup in currently active CSI. */
	sym = symbol_lookup_in_csi(run->program, NULL, nameref->name);

	switch (sym->sc) {
	case sc_csi:
#ifdef DEBUG_RUN_TRACE
		printf("Referencing class.\n");
#endif
		item = rdata_item_new(ic_value);
		val = rdata_value_new();
		var = rdata_var_new(vc_deleg);
		deleg_v = rdata_deleg_new();

		item->u.value = val;
		val->var = var;
		var->u.deleg_v = deleg_v;

		deleg_v->obj = NULL;
		deleg_v->sym = sym;
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


/** Evaluate binary operation. */
static void run_binop(run_t *run, stree_binop_t *binop, rdata_item_t **res)
{
	rdata_item_t *rarg1_i, *rarg2_i;
	rdata_item_t *rarg1_vi, *rarg2_vi;
	rdata_value_t *v1, *v2;
	int i1, i2;

	rdata_item_t *item;
	rdata_value_t *value;
	rdata_var_t *var;
	rdata_int_t *int_v;

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

	run_cvt_value_item(run, rarg1_i, &rarg1_vi);
	run_cvt_value_item(run, rarg2_i, &rarg2_vi);

	v1 = rarg1_vi->u.value;
	v2 = rarg2_vi->u.value;

	if (v1->var->vc != vc_int || v2->var->vc != vc_int) {
		printf("Unimplemented: Binary operation arguments are not "
		    "integer values.\n");
		exit(1);
	}

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

/** Evaluate member acccess. */
static void run_access(run_t *run, stree_access_t *access, rdata_item_t **res)
{
	rdata_item_t *rarg;
	rdata_deleg_t *deleg_v;
	stree_symbol_t *member;

#ifdef DEBUG_RUN_TRACE
	printf("Run access operation.\n");
#endif
	run_expr(run, access->arg, &rarg);

	if (rarg->ic == ic_value && rarg->u.value->var->vc == vc_deleg) {
#ifdef DEBUG_RUN_TRACE
		printf("Accessing delegate.\n");
#endif
		deleg_v = rarg->u.value->var->u.deleg_v;
		if (deleg_v->obj != NULL || deleg_v->sym->sc != sc_csi) {
			printf("Error: Using '.' with symbol of wrong "
			    "type (%d).\n", deleg_v->sym->sc);
			exit(1);
		}

		member = symbol_search_csi(run->program, deleg_v->sym->u.csi,
		    access->member_name);

		if (member == NULL) {
			printf("Error: No such member.\n");
			exit(1);
		}

#ifdef DEBUG_RUN_TRACE
		printf("Found member '%s'.\n",
		    strtab_get_str(access->member_name->sid));
#endif

		/* Reuse existing item, value, var, deleg. */
		deleg_v->sym = member;

		*res = rarg;
		return;
	}

	*res = NULL;
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
		run_cvt_value_item(run, rarg_i, &rarg_vi);

		list_append(&arg_vals, rarg_vi);
		node = list_next(&call->args, node);
	}

	run_fun(run, deleg_v->sym->u.fun, &arg_vals);

#ifdef DEBUG_RUN_TRACE
	printf("Returned from function call.\n");
#endif
	*res = NULL;
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

	run_cvt_value_item(run, rsrc_i, &rsrc_vi);
	src_val = rsrc_vi->u.value;

	if (rdest_i->ic != ic_address) {
		printf("Error: Address expression required on left side of "
		    "assignment operator.\n");
		exit(1);
	}

	run_address_write(run, rdest_i->u.address, rsrc_vi->u.value);

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

	run_cvt_value_item(run, item, &vitem);

	assert(vitem->ic == ic_value);
	var = vitem->u.value->var;

	if (var->vc != vc_int) {
		printf("Error: Boolean (int) expression expected.\n");
		exit(1);
	}

	return (var->u.int_v->value != 0);
}

/** Convert item to value item.
 *
 * If @a item is a value, we just return a copy. If @a item is an address,
 * we read from the address.
 */
void run_cvt_value_item(run_t *run, rdata_item_t *item,
    rdata_item_t **ritem)
{
	rdata_value_t *value;

	/* Address item. Perform read operation. */
	if (item->ic == ic_address) {
		run_address_read(run, item->u.address, ritem);
		return;
	}

	/* It already is a value, we can share the @c var. */
	value = rdata_value_new();
	value->var = item->u.value->var;
	*ritem = rdata_item_new(ic_value);
	(*ritem)->u.value = value;
}

/** Read data from an address.
 *
 * Return value stored in a variable at the specified address.
 */
static void run_address_read(run_t *run, rdata_address_t *address,
    rdata_item_t **ritem)
{
	rdata_value_t *value;
	rdata_var_t *rvar;
	(void) run;

	/* Perform a shallow copy of @c var. */
	rdata_var_copy(address->vref, &rvar);

	value = rdata_value_new();
	value->var = rvar;
	*ritem = rdata_item_new(ic_value);
	(*ritem)->u.value = value;
}

/** Write data to an address.
 *
 * Store @a value to the variable at @a address.
 */
static void run_address_write(run_t *run, rdata_address_t *address,
    rdata_value_t *value)
{
	rdata_var_t *nvar;
	rdata_var_t *orig_var;

	/* Perform a shallow copy of @c value->var. */
	rdata_var_copy(value->var, &nvar);
	orig_var = address->vref;

	/* XXX do this in a prettier way. */

	orig_var->vc = nvar->vc;
	switch (nvar->vc) {
	case vc_int: orig_var->u.int_v = nvar->u.int_v; break;
	case vc_ref: orig_var->u.ref_v = nvar->u.ref_v; break;
	case vc_deleg: orig_var->u.deleg_v = nvar->u.deleg_v; break;
	case vc_object: orig_var->u.object_v = nvar->u.object_v; break;
	default: assert(b_false);
	}

	/* XXX We should free some stuff around here. */
}
