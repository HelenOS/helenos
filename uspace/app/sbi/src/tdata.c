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

/** @file Run-time data representation. */

#include <stdlib.h>
#include <assert.h>
#include "intmap.h"
#include "list.h"
#include "mytypes.h"
#include "stree.h"
#include "strtab.h"
#include "symbol.h"

#include "tdata.h"

static void tdata_item_subst_tprimitive(tdata_primitive_t *torig,
    tdata_tvv_t *tvv, tdata_item_t **res);
static void tdata_item_subst_tobject(tdata_object_t *torig, tdata_tvv_t *tvv,
    tdata_item_t **res);
static void tdata_item_subst_tarray(tdata_array_t *torig, tdata_tvv_t *tvv,
    tdata_item_t **res);
static void tdata_item_subst_tdeleg(tdata_deleg_t *torig,
    tdata_tvv_t *tvv, tdata_item_t **res);
static void tdata_item_subst_tebase(tdata_ebase_t *tebase,
    tdata_tvv_t *tvv, tdata_item_t **res);
static void tdata_item_subst_tenum(tdata_enum_t *tenum,
    tdata_tvv_t *tvv, tdata_item_t **res);
static void tdata_item_subst_tfun(tdata_fun_t *torig,
    tdata_tvv_t *tvv, tdata_item_t **res);
static void tdata_item_subst_tvref(tdata_vref_t *tvref, tdata_tvv_t *tvv,
    tdata_item_t **res);

static void tdata_item_subst_fun_sig(tdata_fun_sig_t *torig, tdata_tvv_t *tvv,
    tdata_fun_sig_t **res);

static void tdata_tprimitive_print(tdata_primitive_t *tprimitive);
static void tdata_tobject_print(tdata_object_t *tobject);
static void tdata_tarray_print(tdata_array_t *tarray);
static void tdata_tdeleg_print(tdata_deleg_t *tdeleg);
static void tdata_tebase_print(tdata_ebase_t *tebase);
static void tdata_tenum_print(tdata_enum_t *tenum);
static void tdata_tfun_print(tdata_fun_t *tfun);
static void tdata_tvref_print(tdata_vref_t *tvref);

/** Determine if CSI @a a is derived from CSI described by type item @a tb.
 *
 * XXX This won't work with generics.
 *
 * @param a	Potential derived CSI.
 * @param tb	Type of potentail base CSI.
 */
bool_t tdata_is_csi_derived_from_ti(stree_csi_t *a, tdata_item_t *tb)
{
	bool_t res;

	switch (tb->tic) {
	case tic_tobject:
		res = stree_is_csi_derived_from_csi(a, tb->u.tobject->csi);
		break;
	default:
		printf("Error: Base type is not a CSI.\n");
		exit(1);
	}

	return res;
}

/**
 * Determine if CSI described by type item @a a is derived from CSI described
 * by type item @a tb.
 *
 * XXX This is somewhat complementary to stype_convert(). It is used for
 * the explicit @c as conversion. It should only work for objects and only
 * allow conversion from base to derived types. We might want to scrap this
 * for a version specific to @c as. The current code does not work with
 * generics.
 *
 * @param a	Potential derived CSI.
 * @param tb	Type of potentail base CSI.
 */
bool_t tdata_is_ti_derived_from_ti(tdata_item_t *ta, tdata_item_t *tb)
{
	bool_t res;

	switch (ta->tic) {
	case tic_tobject:
		res = tdata_is_csi_derived_from_ti(ta->u.tobject->csi, tb);
		break;
	default:
		printf("Error: Derived type is not a CSI.\n");
		exit(1);
	}

	return res;
}

/** Determine if two type items are equal (i.e. describe the same type).
 *
 * Needed to check compatibility of type arguments in which a parametrized
 * type is not monotonous.
 *
 * @param a	Type item
 * @param b	Type item
 * @return	@c b_true if equal, @c b_false if not.
 */
bool_t tdata_item_equal(tdata_item_t *a, tdata_item_t *b)
{
	/*
	 * Special case: Nil vs. object
	 *
	 * XXX Type of @c Nil should probably be @c object to avoid this
	 * madness.
	 */
	if (a->tic == tic_tprimitive && a->u.tprimitive->tpc == tpc_nil) {
		if (b->tic == tic_tobject)
			return b_true;
	} else if (b->tic == tic_tprimitive && b->u.tprimitive->tpc == tpc_nil) {
		if (a->tic == tic_tobject)
			return b_true;
	}

	if (a->tic != b->tic)
		return b_false;

	switch (a->tic) {
	case tic_tprimitive:
		/* Check if both have the same tprimitive class. */
		return (a->u.tprimitive->tpc == b->u.tprimitive->tpc);
	case tic_tobject:
		/* Check if both use the same CSI definition. */
		return (a->u.tobject->csi == b->u.tobject->csi);
	case tic_tarray:
		/* Compare rank and base type. */
		if (a->u.tarray->rank != b->u.tarray->rank)
			return b_false;

		return tdata_item_equal(a->u.tarray->base_ti,
		    b->u.tarray->base_ti);
	case tic_tenum:
		/* Check if both use the same enum definition. */
		return (a->u.tenum->enum_d == b->u.tenum->enum_d);
	case tic_tvref:
		/* Check if both refer to the same type argument. */
		return (a->u.tvref->targ == b->u.tvref->targ);
	default:
		printf("Warning: Unimplemented: Compare types '");
		tdata_item_print(a);
		printf("' and '");
		tdata_item_print(b);
		printf("'.\n");
		return b_true;
	}
}

/** Substitute type variables in a type item.
 *
 * This is the second part of generic type application. In the first part
 * obtained a TVV using stype_titem_to_tvv() and in this second part we
 * actually substitute type variables in a type item for their values.
 * @a tvv must contain all variables referenced in @a ti.
 *
 * @param ti	Type item to substitute into.
 * @param tvv	Type variable valuation (values of type variables).
 * @param res	Place to store pointer to new type item.
 */
void tdata_item_subst(tdata_item_t *ti, tdata_tvv_t *tvv, tdata_item_t **res)
{
	switch (ti->tic) {
	case tic_tprimitive:
		tdata_item_subst_tprimitive(ti->u.tprimitive, tvv, res);
		break;
	case tic_tobject:
		tdata_item_subst_tobject(ti->u.tobject, tvv, res);
		break;
	case tic_tarray:
		tdata_item_subst_tarray(ti->u.tarray, tvv, res);
		break;
	case tic_tdeleg:
		tdata_item_subst_tdeleg(ti->u.tdeleg, tvv, res);
		break;
	case tic_tebase:
		tdata_item_subst_tebase(ti->u.tebase, tvv, res);
		break;
	case tic_tenum:
		tdata_item_subst_tenum(ti->u.tenum, tvv, res);
		break;
	case tic_tfun:
		tdata_item_subst_tfun(ti->u.tfun, tvv, res);
		break;
	case tic_tvref:
		tdata_item_subst_tvref(ti->u.tvref, tvv, res);
		break;
	case tic_ignore:
		*res = tdata_item_new(tic_ignore);
	}
}

/** Substitute type variables in a primitive type item.
 *
 * @param torig	Type item to substitute into.
 * @param tvv	Type variable valuation (values of type variables).
 * @param res	Place to store pointer to new type item.
 */
static void tdata_item_subst_tprimitive(tdata_primitive_t *torig,
    tdata_tvv_t *tvv, tdata_item_t **res)
{
	tdata_primitive_t *tnew;

	(void) tvv;

	/* Plain copy */
	tnew = tdata_primitive_new(torig->tpc);
	*res = tdata_item_new(tic_tprimitive);
	(*res)->u.tprimitive = tnew;
}

/** Substitute type variables in an object type item.
 *
 * @param torig	Type item to substitute into.
 * @param tvv	Type variable valuation (values of type variables).
 * @param res	Place to store pointer to new type item.
 */
static void tdata_item_subst_tobject(tdata_object_t *torig, tdata_tvv_t *tvv,
    tdata_item_t **res)
{
	tdata_object_t *tnew;
	list_node_t *targ_n;
	tdata_item_t *targ;
	tdata_item_t *new_targ;

	/* Copy static ref flag and base CSI. */
	tnew = tdata_object_new();
	tnew->static_ref = torig->static_ref;
	tnew->csi = torig->csi;
	list_init(&tnew->targs);

	/* Substitute arguments */
	targ_n = list_first(&torig->targs);
	while (targ_n != NULL) {
		targ = list_node_data(targ_n, tdata_item_t *);
		tdata_item_subst(targ, tvv, &new_targ);
		list_append(&tnew->targs, new_targ);

		targ_n = list_next(&torig->targs, targ_n);
	}

	*res = tdata_item_new(tic_tobject);
	(*res)->u.tobject = tnew;
}

/** Substitute type variables in an array type item.
 *
 * @param torig	Type item to substitute into.
 * @param tvv	Type variable valuation (values of type variables).
 * @param res	Place to store pointer to new type item.
 */
static void tdata_item_subst_tarray(tdata_array_t *torig, tdata_tvv_t *tvv,
    tdata_item_t **res)
{
	tdata_array_t *tnew;
	list_node_t *ext_n;
	stree_expr_t *extent;

	tnew = tdata_array_new();

	/* Substitute base type */
	tdata_item_subst(torig->base_ti, tvv, &tnew->base_ti);

	/* Copy rank and extents */
	tnew->rank = torig->rank;
	list_init(&tnew->extents);

	ext_n = list_first(&torig->extents);
	while (ext_n != NULL) {
		extent = list_node_data(ext_n, stree_expr_t *);
		list_append(&tnew->extents, extent);

		ext_n = list_next(&tnew->extents, ext_n);
	}

	*res = tdata_item_new(tic_tarray);
	(*res)->u.tarray = tnew;
}

/** Substitute type variables in a delegate type item.
 *
 * @param torig	Type item to substitute into.
 * @param tvv	Type variable valuation (values of type variables).
 * @param res	Place to store pointer to new type item.
 */
static void tdata_item_subst_tdeleg(tdata_deleg_t *torig, tdata_tvv_t *tvv,
    tdata_item_t **res)
{
	tdata_deleg_t *tnew;

	tnew = tdata_deleg_new();
	tnew->deleg = torig->deleg;
	tdata_item_subst_fun_sig(torig->tsig, tvv, &tnew->tsig);

	*res = tdata_item_new(tic_tdeleg);
	(*res)->u.tdeleg = tnew;
}

/** Substitute type variables in a enum-base type item.
 *
 * @param torig	Type item to substitute into.
 * @param tvv	Type variable valuation (values of type variables).
 * @param res	Place to store pointer to new type item.
 */
static void tdata_item_subst_tebase(tdata_ebase_t *tebase,
    tdata_tvv_t *tvv, tdata_item_t **res)
{
	tdata_ebase_t *tnew;

	(void) tvv;

	/* Plain copy */
	tnew = tdata_ebase_new();
	tnew->enum_d = tebase->enum_d;

	*res = tdata_item_new(tic_tebase);
	(*res)->u.tebase = tnew;
}

/** Substitute type variables in a enum type item.
 *
 * @param torig	Type item to substitute into.
 * @param tvv	Type variable valuation (values of type variables).
 * @param res	Place to store pointer to new type item.
 */
static void tdata_item_subst_tenum(tdata_enum_t *tenum,
    tdata_tvv_t *tvv, tdata_item_t **res)
{
	tdata_enum_t *tnew;

	(void) tvv;

	/* Plain copy */
	tnew = tdata_enum_new();
	tnew->enum_d = tenum->enum_d;

	*res = tdata_item_new(tic_tenum);
	(*res)->u.tenum = tnew;
}

/** Substitute type variables in a functional type item.
 *
 * @param torig	Type item to substitute into.
 * @param tvv	Type variable valuation (values of type variables).
 * @param res	Place to store pointer to new type item.
 */
static void tdata_item_subst_tfun(tdata_fun_t *torig, tdata_tvv_t *tvv,
    tdata_item_t **res)
{
	tdata_fun_t *tnew;

	tnew = tdata_fun_new();
	tdata_item_subst_fun_sig(torig->tsig, tvv, &tnew->tsig);

	*res = tdata_item_new(tic_tfun);
	(*res)->u.tfun = tnew;
}

/** Substitute type variables in a type-variable reference item.
 *
 * @param torig	Type item to substitute into.
 * @param tvv	Type variable valuation (values of type variables).
 * @param res	Place to store pointer to new type item.
 */
static void tdata_item_subst_tvref(tdata_vref_t *tvref, tdata_tvv_t *tvv,
    tdata_item_t **res)
{
	tdata_item_t *ti_new;

	ti_new = tdata_tvv_get_val(tvv, tvref->targ->name->sid);
	assert(ti_new != NULL);

	/* XXX Might be better to clone here. */
	*res = ti_new;
}

/** Substitute type variables in a function signature type fragment.
 *
 * @param torig	Type item to substitute into.
 * @param tvv	Type variable valuation (values of type variables).
 * @param res	Place to store pointer to new type item.
 */
static void tdata_item_subst_fun_sig(tdata_fun_sig_t *torig, tdata_tvv_t *tvv,
    tdata_fun_sig_t **res)
{
	tdata_fun_sig_t *tnew;
	list_node_t *arg_n;
	tdata_item_t *arg_ti;
	tdata_item_t *narg_ti = NULL;

	tnew = tdata_fun_sig_new();

	/* Substitute type of each argument */
	list_init(&tnew->arg_ti);
	arg_n = list_first(&torig->arg_ti);
	while (arg_n != NULL) {
		arg_ti = list_node_data(arg_n, tdata_item_t *);

		/* XXX Because of overloaded Builtin.WriteLine */
		if (arg_ti == NULL)
			narg_ti = NULL;
		else
			tdata_item_subst(arg_ti, tvv, &narg_ti);

		list_append(&tnew->arg_ti, narg_ti);

		arg_n = list_next(&torig->arg_ti, arg_n);
	}

	/* Substitute type of variadic argument */
	if (torig->varg_ti != NULL)
		tdata_item_subst(torig->varg_ti, tvv, &tnew->varg_ti);

	/* Substitute return type */
	if (torig->rtype != NULL)
		tdata_item_subst(torig->rtype, tvv, &tnew->rtype);

	*res = tnew;
}

/** Print type item.
 *
 * @param titem	Type item
 */
void tdata_item_print(tdata_item_t *titem)
{
	if (titem == NULL) {
		printf("none");
		return;
	}

	switch (titem->tic) {
	case tic_tprimitive:
		tdata_tprimitive_print(titem->u.tprimitive);
		break;
	case tic_tobject:
		tdata_tobject_print(titem->u.tobject);
		break;
	case tic_tarray:
		tdata_tarray_print(titem->u.tarray);
		break;
	case tic_tdeleg:
		tdata_tdeleg_print(titem->u.tdeleg);
		break;
	case tic_tebase:
		tdata_tebase_print(titem->u.tebase);
		break;
	case tic_tenum:
		tdata_tenum_print(titem->u.tenum);
		break;
	case tic_tfun:
		tdata_tfun_print(titem->u.tfun);
		break;
	case tic_tvref:
		tdata_tvref_print(titem->u.tvref);
		break;
	case tic_ignore:
		printf("ignore");
		break;
	}
}

/** Print primitive type item.
 *
 * @param tprimitive	Primitive type item
 */
static void tdata_tprimitive_print(tdata_primitive_t *tprimitive)
{
	switch (tprimitive->tpc) {
	case tpc_bool:
		printf("bool");
		break;
	case tpc_char:
		printf("char");
		break;
	case tpc_int:
		printf("int");
		break;
	case tpc_nil:
		printf("nil");
		break;
	case tpc_string:
		printf("string");
		break;
	case tpc_resource:
		printf("resource");
		break;
	}
}

/** Print object type item.
 *
 * @param tobject	Object type item
 */
static void tdata_tobject_print(tdata_object_t *tobject)
{
	stree_symbol_t *csi_sym;
	list_node_t *arg_n;
	tdata_item_t *arg;

	csi_sym = csi_to_symbol(tobject->csi);
	assert(csi_sym != NULL);
	symbol_print_fqn(csi_sym);

	arg_n = list_first(&tobject->targs);
	while (arg_n != NULL) {
		arg = list_node_data(arg_n, tdata_item_t *);
		putchar('/');
		tdata_item_print(arg);
		arg_n = list_next(&tobject->targs, arg_n);
	}
}

/** Print array type item.
 *
 * @param tarray	Array type item
 */
static void tdata_tarray_print(tdata_array_t *tarray)
{
	int i;

	tdata_item_print(tarray->base_ti);

	printf("[");
	for (i = 0; i < tarray->rank - 1; ++i)
		printf(",");
	printf("]");
}

/** Print delegate type item.
 *
 * @param tdeleg	Delegate type item
 */
static void tdata_tdeleg_print(tdata_deleg_t *tdeleg)
{
	stree_symbol_t *deleg_sym;

	deleg_sym = deleg_to_symbol(tdeleg->deleg);
	symbol_print_fqn(deleg_sym);
}

/** Print enum-base type item.
 *
 * @param tebase		Enum-base type item
 */
static void tdata_tebase_print(tdata_ebase_t *tebase)
{
	stree_symbol_t *enum_sym;

	enum_sym = enum_to_symbol(tebase->enum_d);

	printf("typeref(");
	symbol_print_fqn(enum_sym);
	printf(")");
}

/** Print enum type item.
 *
 * @param tenum		Enum type item
 */
static void tdata_tenum_print(tdata_enum_t *tenum)
{
	stree_symbol_t *enum_sym;

	enum_sym = enum_to_symbol(tenum->enum_d);
	symbol_print_fqn(enum_sym);
}

/** Print function type item.
 *
 * @param tfun		Function type item
 */
static void tdata_tfun_print(tdata_fun_t *tfun)
{
	list_node_t *arg_n;
	tdata_item_t *arg_ti;
	bool_t first;

	printf("fun(");

	arg_n = list_first(&tfun->tsig->arg_ti);
	first = b_true;
	while (arg_n != NULL) {
		if (first == b_false)
			printf("; ");
		else
			first = b_false;

		arg_ti = list_node_data(arg_n, tdata_item_t *);
		tdata_item_print(arg_ti);

		arg_n = list_next(&tfun->tsig->arg_ti, arg_n);
	}

	printf(") : ");
	tdata_item_print(tfun->tsig->rtype);
}

/** Print type variable reference type item.
 *
 * @param tvref		Type variable reference type item
 */
static void tdata_tvref_print(tdata_vref_t *tvref)
{
	printf("%s", strtab_get_str(tvref->targ->name->sid));
}

/** Allocate new type item.
 *
 * @param tic	Type item class
 * @return	New type item
 */
tdata_item_t *tdata_item_new(titem_class_t tic)
{
	tdata_item_t *titem;

	titem = calloc(1, sizeof(tdata_item_t));
	if (titem == NULL) {
		printf("Memory allocation failed.\n");
		exit(1);
	}

	titem->tic = tic;
	return titem;
}

/** Allocate new array type item.
 *
 * @return	New array type item
 */
tdata_array_t *tdata_array_new(void)
{
	tdata_array_t *tarray;

	tarray = calloc(1, sizeof(tdata_array_t));
	if (tarray == NULL) {
		printf("Memory allocation failed.\n");
		exit(1);
	}

	return tarray;
}

/** Allocate new object type item.
 *
 * @return	New object type item
 */
tdata_object_t *tdata_object_new(void)
{
	tdata_object_t *tobject;

	tobject = calloc(1, sizeof(tdata_object_t));
	if (tobject == NULL) {
		printf("Memory allocation failed.\n");
		exit(1);
	}

	return tobject;
}

/** Allocate new primitive type item.
 *
 * @return	New primitive type item
 */
tdata_primitive_t *tdata_primitive_new(tprimitive_class_t tpc)
{
	tdata_primitive_t *tprimitive;

	tprimitive = calloc(1, sizeof(tdata_primitive_t));
	if (tprimitive == NULL) {
		printf("Memory allocation failed.\n");
		exit(1);
	}

	tprimitive->tpc = tpc;
	return tprimitive;
}

/** Allocate new delegate type item.
 *
 * @return	New function type item
 */
tdata_deleg_t *tdata_deleg_new(void)
{
	tdata_deleg_t *tdeleg;

	tdeleg = calloc(1, sizeof(tdata_deleg_t));
	if (tdeleg == NULL) {
		printf("Memory allocation failed.\n");
		exit(1);
	}

	return tdeleg;
}

/** Allocate new enum-base type item.
 *
 * @return	New enum type item
 */
tdata_ebase_t *tdata_ebase_new(void)
{
	tdata_ebase_t *tebase;

	tebase = calloc(1, sizeof(tdata_ebase_t));
	if (tebase == NULL) {
		printf("Memory allocation failed.\n");
		exit(1);
	}

	return tebase;
}

/** Allocate new enum type item.
 *
 * @return	New enum type item
 */
tdata_enum_t *tdata_enum_new(void)
{
	tdata_enum_t *tenum;

	tenum = calloc(1, sizeof(tdata_enum_t));
	if (tenum == NULL) {
		printf("Memory allocation failed.\n");
		exit(1);
	}

	return tenum;
}

/** Allocate new functional type item.
 *
 * @return	New function type item
 */
tdata_fun_t *tdata_fun_new(void)
{
	tdata_fun_t *tfun;

	tfun = calloc(1, sizeof(tdata_fun_t));
	if (tfun == NULL) {
		printf("Memory allocation failed.\n");
		exit(1);
	}

	return tfun;
}

/** Allocate new type variable reference type item.
 *
 * @return	New type variable reference type item
 */
tdata_vref_t *tdata_vref_new(void)
{
	tdata_vref_t *tvref;

	tvref = calloc(1, sizeof(tdata_vref_t));
	if (tvref == NULL) {
		printf("Memory allocation failed.\n");
		exit(1);
	}

	return tvref;
}

/** Allocate new function signature type fragment.
 *
 * @return	New function signature type fragment
 */
tdata_fun_sig_t *tdata_fun_sig_new(void)
{
	tdata_fun_sig_t *tfun_sig;

	tfun_sig = calloc(1, sizeof(tdata_fun_sig_t));
	if (tfun_sig == NULL) {
		printf("Memory allocation failed.\n");
		exit(1);
	}

	return tfun_sig;
}

/** Create a new type variable valuation.
 *
 * @retrun	New type variable valuation
 */
tdata_tvv_t *tdata_tvv_new(void)
{
	tdata_tvv_t *tvv;

	tvv = calloc(1, sizeof(tdata_tvv_t));
	if (tvv == NULL) {
		printf("Memory allocation failed.\n");
		exit(1);
	}

	return tvv;
}

/** Get type variable value.
 *
 * Looks up value of the variable with name SID @a name in type
 * variable valuation @a tvv.
 *
 * @param tvv		Type variable valuation
 * @param name		Name of the variable (SID)
 * @return		Value of the type variable (type item) or @c NULL
 *			if not defined in @a tvv
 */
tdata_item_t *tdata_tvv_get_val(tdata_tvv_t *tvv, sid_t name)
{
	return (tdata_item_t *)intmap_get(&tvv->tvv, name);
}

/** Set type variable value.
 *
 * Sets the value of variable with name SID @a name in type variable
 * valuation @a tvv to the value @a tvalue.
 *
 * @param tvv		Type variable valuation
 * @param name		Name of the variable (SID)
 * @param tvalue	Value to set (type item) or @c NULL to unset
 */
void tdata_tvv_set_val(tdata_tvv_t *tvv, sid_t name, tdata_item_t *tvalue)
{
	intmap_set(&tvv->tvv, name, tvalue);
}
