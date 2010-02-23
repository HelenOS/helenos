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
#include "mytypes.h"

#include "rdata.h"

static void rdata_int_copy(rdata_int_t *src, rdata_int_t **dest);
static void rdata_string_copy(rdata_string_t *src, rdata_string_t **dest);
static void rdata_ref_copy(rdata_ref_t *src, rdata_ref_t **dest);
static void rdata_deleg_copy(rdata_deleg_t *src, rdata_deleg_t **dest);
static void rdata_object_copy(rdata_object_t *src, rdata_object_t **dest);

static void rdata_address_print(rdata_address_t *address);
static void rdata_value_print(rdata_value_t *value);
static void rdata_var_print(rdata_var_t *var);


rdata_item_t *rdata_item_new(item_class_t ic)
{
	rdata_item_t *item;

	item = calloc(1, sizeof(rdata_item_t));
	if (item == NULL) {
		printf("Memory allocation failed.\n");
		exit(1);
	}

	item->ic = ic;
	return item;
}

rdata_address_t *rdata_address_new(void)
{
	rdata_address_t *address;

	address = calloc(1, sizeof(rdata_address_t));
	if (address == NULL) {
		printf("Memory allocation failed.\n");
		exit(1);
	}

	return address;
}

rdata_value_t *rdata_value_new(void)
{
	rdata_value_t *value;

	value = calloc(1, sizeof(rdata_value_t));
	if (value == NULL) {
		printf("Memory allocation failed.\n");
		exit(1);
	}

	return value;
}

rdata_var_t *rdata_var_new(var_class_t vc)
{
	rdata_var_t *var;

	var = calloc(1, sizeof(rdata_var_t));
	if (var == NULL) {
		printf("Memory allocation failed.\n");
		exit(1);
	}

	var->vc = vc;
	return var;
}

rdata_deleg_t *rdata_deleg_new(void)
{
	rdata_deleg_t *deleg;

	deleg = calloc(1, sizeof(rdata_deleg_t));
	if (deleg == NULL) {
		printf("Memory allocation failed.\n");
		exit(1);
	}

	return deleg;
}

rdata_int_t *rdata_int_new(void)
{
	rdata_int_t *int_v;

	int_v = calloc(1, sizeof(rdata_int_t));
	if (int_v == NULL) {
		printf("Memory allocation failed.\n");
		exit(1);
	}

	return int_v;
}

rdata_string_t *rdata_string_new(void)
{
	rdata_string_t *string_v;

	string_v = calloc(1, sizeof(rdata_string_t));
	if (string_v == NULL) {
		printf("Memory allocation failed.\n");
		exit(1);
	}

	return string_v;
}

/** Make copy of a variable. */
void rdata_var_copy(rdata_var_t *src, rdata_var_t **dest)
{
	rdata_var_t *nvar;

	nvar = rdata_var_new(src->vc);

	switch (src->vc) {
	case vc_int:
		rdata_int_copy(src->u.int_v, &nvar->u.int_v);
		break;
	case vc_string:
		rdata_string_copy(src->u.string_v, &nvar->u.string_v);
		break;
	case vc_ref:
		rdata_ref_copy(src->u.ref_v, &nvar->u.ref_v);
		break;
	case vc_deleg:
		rdata_deleg_copy(src->u.deleg_v, &nvar->u.deleg_v);
		break;
	case vc_object:
		rdata_object_copy(src->u.object_v, &nvar->u.object_v);
		break;
	}

	*dest = nvar;
}

static void rdata_int_copy(rdata_int_t *src, rdata_int_t **dest)
{
	*dest = rdata_int_new();
	(*dest)->value = src->value;
}

static void rdata_string_copy(rdata_string_t *src, rdata_string_t **dest)
{
	*dest = rdata_string_new();
	(*dest)->value = src->value;
}

static void rdata_ref_copy(rdata_ref_t *src, rdata_ref_t **dest)
{
	printf("Unimplemented: Copy reference.\n");
	exit(1);
}

static void rdata_deleg_copy(rdata_deleg_t *src, rdata_deleg_t **dest)
{
	printf("Unimplemented: Copy delegate.\n");
	exit(1);
}

static void rdata_object_copy(rdata_object_t *src, rdata_object_t **dest)
{
	printf("Unimplemented: Copy object.\n");
	exit(1);
}

void rdata_item_print(rdata_item_t *item)
{
	if (item == NULL) {
		printf("none");
		return;
	}

	switch (item->ic) {
	case ic_address:
		printf("address:");
		rdata_address_print(item->u.address);
		break;
	case ic_value:
		printf("value:");
		rdata_value_print(item->u.value);
		break;
	}
}

static void rdata_address_print(rdata_address_t *address)
{
	rdata_var_print(address->vref);
}

static void rdata_value_print(rdata_value_t *value)
{
	rdata_var_print(value->var);
}

static void rdata_var_print(rdata_var_t *var)
{
	switch (var->vc) {
	case vc_int:
		printf("int(%d)", var->u.int_v->value);
		break;
	case vc_ref:
		printf("ref");
		break;
	case vc_deleg:
		printf("deleg");
		break;
	case vc_object:
		printf("object");
		break;
	default:
		assert(b_false);
	}
}
