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
#include "list.h"
#include "mytypes.h"
#include "stree.h"
#include "symbol.h"

#include "tdata.h"

static void tdata_tprimitive_print(tdata_primitive_t *tprimitive);
static void tdata_tobject_print(tdata_object_t *tobject);
static void tdata_tarray_print(tdata_array_t *tarray);
static void tdata_tfun_print(tdata_fun_t *tfun);

/** Determine if CSI @a a is derived from CSI described by type item @a tb. */
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

/** Determine if two type items are equal (i.e. describe the same type). */
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
	default:
		printf("Warning: Unimplemented: Compare types '");
		tdata_item_print(a);
		printf("' and '");
		tdata_item_print(b);
		printf("'.\n");
		return b_true;
	}
}

/** Print type item. */
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
	case tic_tfun:
		tdata_tfun_print(titem->u.tfun);
		break;
	case tic_ignore:
		printf("ignore");
		break;
	}
}

static void tdata_tprimitive_print(tdata_primitive_t *tprimitive)
{
	switch (tprimitive->tpc) {
	case tpc_bool: printf("bool"); break;
	case tpc_char: printf("char"); break;
	case tpc_int: printf("int"); break;
	case tpc_nil: printf("nil"); break;
	case tpc_string: printf("string"); break;
	case tpc_resource: printf("resource"); break;
	}
}

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

static void tdata_tarray_print(tdata_array_t *tarray)
{
	int i;

	tdata_item_print(tarray->base_ti);

	printf("[");
	for (i = 0; i < tarray->rank - 1; ++i)
		printf(",");
	printf("]");
}

static void tdata_tfun_print(tdata_fun_t *tfun)
{
	(void) tfun;
	printf("unimplemented(fun)");
}

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
