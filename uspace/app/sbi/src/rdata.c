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

/** @file Run-time data representation.
 *
 * At run time SBI represents all data as a graph of interconnected @c var
 * nodes (variable nodes). Any piece of memory addressable by the program
 * (i.e. all variables) are stored in var nodes. However, var nodes are also
 * used internally to implement value items. (I.e. values in value items
 * have exactly the same structure as program variables).
 *
 * Unlike byte- or word-oriented memory on a real machine, var nodes provide
 * structured and typed storage. (This typing is dynamic, however and has
 * nothing to do with the static type system).
 *
 * There are several types of var nodes, one for each primitive type,
 * reference, delegate, array, and object. A reference var node contains
 * a pointer to another var node. Delegate var node points to some stree
 * declaration. Array and object var nodes refer to a collection of child
 * nodes (fields, elements).
 */

#include <stdlib.h>
#include <assert.h>
#include "bigint.h"
#include "list.h"
#include "mytypes.h"
#include "stree.h"
#include "symbol.h"
#include "strtab.h"

#include "rdata.h"

static void rdata_bool_copy(rdata_bool_t *src, rdata_bool_t **dest);
static void rdata_char_copy(rdata_char_t *src, rdata_char_t **dest);
static void rdata_int_copy(rdata_int_t *src, rdata_int_t **dest);
static void rdata_string_copy(rdata_string_t *src, rdata_string_t **dest);
static void rdata_ref_copy(rdata_ref_t *src, rdata_ref_t **dest);
static void rdata_deleg_copy(rdata_deleg_t *src, rdata_deleg_t **dest);
static void rdata_enum_copy(rdata_enum_t *src, rdata_enum_t **dest);
static void rdata_array_copy(rdata_array_t *src, rdata_array_t **dest);
static void rdata_object_copy(rdata_object_t *src, rdata_object_t **dest);
static void rdata_resource_copy(rdata_resource_t *src,
    rdata_resource_t **dest);
static void rdata_symbol_copy(rdata_symbol_t *src, rdata_symbol_t **dest);

static void rdata_var_destroy_inner(rdata_var_t *var);

static void rdata_bool_destroy(rdata_bool_t *bool_v);
static void rdata_char_destroy(rdata_char_t *char_v);
static void rdata_int_destroy(rdata_int_t *int_v);
static void rdata_string_destroy(rdata_string_t *string_v);
static void rdata_ref_destroy(rdata_ref_t *ref_v);
static void rdata_deleg_destroy(rdata_deleg_t *deleg_v);
static void rdata_enum_destroy(rdata_enum_t *enum_v);
static void rdata_array_destroy(rdata_array_t *array_v);
static void rdata_object_destroy(rdata_object_t *object_v);
static void rdata_resource_destroy(rdata_resource_t *resource_v);
static void rdata_symbol_destroy(rdata_symbol_t *symbol_v);

static int rdata_array_get_dim(rdata_array_t *array);
static void rdata_var_copy_to(rdata_var_t *src, rdata_var_t *dest);

static void rdata_address_print(rdata_address_t *address);
static void rdata_var_print(rdata_var_t *var);

/** Allocate new data item.
 *
 * @param ic	Item class.
 * @return	New item.
 */
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

/** Allocate new address.
 *
 * @return	New address.
 */
rdata_addr_var_t *rdata_addr_var_new(void)
{
	rdata_addr_var_t *addr_var;

	addr_var = calloc(1, sizeof(rdata_addr_var_t));
	if (addr_var == NULL) {
		printf("Memory allocation failed.\n");
		exit(1);
	}

	return addr_var;
}

/** Allocate new named property address.
 *
 * @return	New named property address.
 */
rdata_aprop_named_t *rdata_aprop_named_new(void)
{
	rdata_aprop_named_t *aprop_named;

	aprop_named = calloc(1, sizeof(rdata_aprop_named_t));
	if (aprop_named == NULL) {
		printf("Memory allocation failed.\n");
		exit(1);
	}

	return aprop_named;
}

/** Allocate new indexed property address.
 *
 * @return	New indexed property address.
 */
rdata_aprop_indexed_t *rdata_aprop_indexed_new(void)
{
	rdata_aprop_indexed_t *aprop_indexed;

	aprop_indexed = calloc(1, sizeof(rdata_aprop_indexed_t));
	if (aprop_indexed == NULL) {
		printf("Memory allocation failed.\n");
		exit(1);
	}

	return aprop_indexed;
}

/** Allocate new property address.
 *
 * @param apc	Property address class.
 * @return	New property address.
 */
rdata_addr_prop_t *rdata_addr_prop_new(aprop_class_t apc)
{
	rdata_addr_prop_t *addr_prop;

	addr_prop = calloc(1, sizeof(rdata_addr_prop_t));
	if (addr_prop == NULL) {
		printf("Memory allocation failed.\n");
		exit(1);
	}

	addr_prop->apc = apc;
	return addr_prop;
}

/** Allocate new address.
 *
 * @param ac	Address class.
 * @return	New address.
 */
rdata_address_t *rdata_address_new(address_class_t ac)
{
	rdata_address_t *address;

	address = calloc(1, sizeof(rdata_address_t));
	if (address == NULL) {
		printf("Memory allocation failed.\n");
		exit(1);
	}

	address->ac = ac;
	return address;
}

/** Allocate new value.
 *
 * @return	New value.
 */
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

/** Allocate new var node.
 *
 * @param vc	Var node class (varclass).
 * @return	New var node.
 */
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

/** Allocate new reference.
 *
 * @return	New reference.
 */
rdata_ref_t *rdata_ref_new(void)
{
	rdata_ref_t *ref;

	ref = calloc(1, sizeof(rdata_ref_t));
	if (ref == NULL) {
		printf("Memory allocation failed.\n");
		exit(1);
	}

	return ref;
}

/** Allocate new delegate.
 *
 * @return	New delegate.
 */
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

/** Allocate new enum value.
 *
 * @return	New enum value.
 */
rdata_enum_t *rdata_enum_new(void)
{
	rdata_enum_t *enum_v;

	enum_v = calloc(1, sizeof(rdata_enum_t));
	if (enum_v == NULL) {
		printf("Memory allocation failed.\n");
		exit(1);
	}

	return enum_v;
}

/** Allocate new array.
 *
 * @return	New array.
 */
rdata_array_t *rdata_array_new(int rank)
{
	rdata_array_t *array;

	array = calloc(1, sizeof(rdata_array_t));
	if (array == NULL) {
		printf("Memory allocation failed.\n");
		exit(1);
	}

	array->rank = rank;
	array->extent = calloc(rank, sizeof(int));
	if (array == NULL) {
		printf("Memory allocation failed.\n");
		exit(1);
	}

	return array;
}

/** Allocate new object.
 *
 * @return	New object.
 */
rdata_object_t *rdata_object_new(void)
{
	rdata_object_t *object;

	object = calloc(1, sizeof(rdata_object_t));
	if (object == NULL) {
		printf("Memory allocation failed.\n");
		exit(1);
	}

	return object;
}

/** Allocate new boolean.
 *
 * @return	New boolean.
 */
rdata_bool_t *rdata_bool_new(void)
{
	rdata_bool_t *bool_v;

	bool_v = calloc(1, sizeof(rdata_bool_t));
	if (bool_v == NULL) {
		printf("Memory allocation failed.\n");
		exit(1);
	}

	return bool_v;
}

/** Allocate new character.
 *
 * @return	New character.
 */
rdata_char_t *rdata_char_new(void)
{
	rdata_char_t *char_v;

	char_v = calloc(1, sizeof(rdata_char_t));
	if (char_v == NULL) {
		printf("Memory allocation failed.\n");
		exit(1);
	}

	return char_v;
}

/** Allocate new integer.
 *
 * @return	New integer.
 */
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

/** Allocate new string.
 *
 * @return	New string.
 */
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

/** Allocate new resource.
 *
 * @return	New resource.
 */
rdata_resource_t *rdata_resource_new(void)
{
	rdata_resource_t *resource_v;

	resource_v = calloc(1, sizeof(rdata_resource_t));
	if (resource_v == NULL) {
		printf("Memory allocation failed.\n");
		exit(1);
	}

	return resource_v;
}

/** Allocate new symbol reference.
 *
 * @return	New symbol reference.
 */
rdata_symbol_t *rdata_symbol_new(void)
{
	rdata_symbol_t *symbol_v;

	symbol_v = calloc(1, sizeof(rdata_symbol_t));
	if (symbol_v == NULL) {
		printf("Memory allocation failed.\n");
		exit(1);
	}

	return symbol_v;
}

/** Allocate array elements.
 *
 * Allocates element array of @a array.
 *
 * @param array		Array.
 */
void rdata_array_alloc_element(rdata_array_t *array)
{
	int dim;

	dim = rdata_array_get_dim(array);

	array->element = calloc(dim, sizeof(rdata_var_t *));
	if (array->element == NULL) {
		printf("Memory allocation failed.\n");
		exit(1);
	}
}

/** Get array dimension.
 *
 * Dimension is the total number of elements in an array, in other words,
 * the product of all extents.
 *
 * @param array		Array.
 */
static int rdata_array_get_dim(rdata_array_t *array)
{
	int didx, dim;

	dim = 1;
	for (didx = 0; didx < array->rank; ++didx)
		dim = dim * array->extent[didx];

	return dim;
}

/** Deallocate item.
 *
 * @param item	Item node
 */
void rdata_item_delete(rdata_item_t *item)
{
	assert(item != NULL);
	free(item);
}

/** Deallocate variable address.
 *
 * @param addr_var	Variable address node
 */
void rdata_addr_var_delete(rdata_addr_var_t *addr_var)
{
	assert(addr_var != NULL);
	free(addr_var);
}

/** Deallocate property address.
 *
 * @param addr_prop	Variable address node
 */
void rdata_addr_prop_delete(rdata_addr_prop_t *addr_prop)
{
	assert(addr_prop != NULL);
	free(addr_prop);
}

/** Deallocate named property address.
 *
 * @param aprop_named	Variable address node
 */
void rdata_aprop_named_delete(rdata_aprop_named_t *aprop_named)
{
	assert(aprop_named != NULL);
	free(aprop_named);
}

/** Deallocate indexed property address.
 *
 * @param aprop_indexed	Variable address node
 */
void rdata_aprop_indexed_delete(rdata_aprop_indexed_t *aprop_indexed)
{
	assert(aprop_indexed != NULL);
	free(aprop_indexed);
}

/** Deallocate address.
 *
 * @param address	Address node
 */
void rdata_address_delete(rdata_address_t *address)
{
	assert(address != NULL);
	free(address);
}

/** Deallocate value.
 *
 * @param value		Value node
 */
void rdata_value_delete(rdata_value_t *value)
{
	assert(value != NULL);
	free(value);
}

/** Deallocate var node.
 *
 * @param var	Var node
 */
void rdata_var_delete(rdata_var_t *var)
{
	assert(var != NULL);
	free(var);
}

/** Deallocate boolean.
 *
 * @param bool_v		Boolean
 */
void rdata_bool_delete(rdata_bool_t *bool_v)
{
	assert(bool_v != NULL);
	free(bool_v);
}

/** Deallocate character.
 *
 * @param char_v	Character
 */
void rdata_char_delete(rdata_char_t *char_v)
{
	assert(char_v != NULL);
	free(char_v);
}

/** Deallocate integer.
 *
 * @param int_v		Integer
 */
void rdata_int_delete(rdata_int_t *int_v)
{
	assert(int_v != NULL);
	free(int_v);
}

/** Deallocate string.
 *
 * @param string_v	String
 */
void rdata_string_delete(rdata_string_t *string_v)
{
	assert(string_v != NULL);
	free(string_v);
}

/** Deallocate reference.
 *
 * @param ref_v		Reference
 */
void rdata_ref_delete(rdata_ref_t *ref_v)
{
	assert(ref_v != NULL);
	free(ref_v);
}

/** Deallocate delegate.
 *
 * @param deleg_v		Reference
 */
void rdata_deleg_delete(rdata_deleg_t *deleg_v)
{
	assert(deleg_v != NULL);
	free(deleg_v);
}

/** Deallocate enum.
 *
 * @param enum_v		Reference
 */
void rdata_enum_delete(rdata_enum_t *enum_v)
{
	assert(enum_v != NULL);
	free(enum_v);
}

/** Deallocate array.
 *
 * @param array_v		Array
 */
void rdata_array_delete(rdata_array_t *array_v)
{
	assert(array_v != NULL);
	free(array_v);
}

/** Deallocate object.
 *
 * @param object_v		Object
 */
void rdata_object_delete(rdata_object_t *object_v)
{
	assert(object_v != NULL);
	free(object_v);
}

/** Deallocate resource.
 *
 * @param resource_v		Resource
 */
void rdata_resource_delete(rdata_resource_t *resource_v)
{
	assert(resource_v != NULL);
	free(resource_v);
}

/** Deallocate symbol.
 *
 * @param symbol_v		Symbol
 */
void rdata_symbol_delete(rdata_symbol_t *symbol_v)
{
	assert(symbol_v != NULL);
	free(symbol_v);
}

/** Copy value.
 *
 * @param src		Input value
 * @param dest		Place to store pointer to new value
 */
void rdata_value_copy(rdata_value_t *src, rdata_value_t **dest)
{
	assert(src != NULL);

	*dest = rdata_value_new();
	rdata_var_copy(src->var, &(*dest)->var);
}

/** Make copy of a variable.
 *
 * Creates a new var node that is an exact copy of an existing var node.
 * This can be thought of as a shallow copy.
 *
 * @param src		Source var node.
 * @param dest		Place to store pointer to new var node.
 */
void rdata_var_copy(rdata_var_t *src, rdata_var_t **dest)
{
	rdata_var_t *nvar;

	nvar = rdata_var_new(src->vc);
	rdata_var_copy_to(src, nvar);

	*dest = nvar;
}

/** Copy variable content to another variable.
 *
 * Writes an exact copy of an existing var node to another var node.
 * The varclass of @a src and @a dest must match. The content of
 * @a dest.u must be invalid.
 *
 * @param src		Source var node.
 * @param dest		Destination var node.
 */
static void rdata_var_copy_to(rdata_var_t *src, rdata_var_t *dest)
{
	dest->vc = src->vc;

	switch (src->vc) {
	case vc_bool:
		rdata_bool_copy(src->u.bool_v, &dest->u.bool_v);
		break;
	case vc_char:
		rdata_char_copy(src->u.char_v, &dest->u.char_v);
		break;
	case vc_int:
		rdata_int_copy(src->u.int_v, &dest->u.int_v);
		break;
	case vc_string:
		rdata_string_copy(src->u.string_v, &dest->u.string_v);
		break;
	case vc_ref:
		rdata_ref_copy(src->u.ref_v, &dest->u.ref_v);
		break;
	case vc_deleg:
		rdata_deleg_copy(src->u.deleg_v, &dest->u.deleg_v);
		break;
	case vc_enum:
		rdata_enum_copy(src->u.enum_v, &dest->u.enum_v);
		break;
	case vc_array:
		rdata_array_copy(src->u.array_v, &dest->u.array_v);
		break;
	case vc_object:
		rdata_object_copy(src->u.object_v, &dest->u.object_v);
		break;
	case vc_resource:
		rdata_resource_copy(src->u.resource_v, &dest->u.resource_v);
		break;
	case vc_symbol:
		rdata_symbol_copy(src->u.symbol_v, &dest->u.symbol_v);
		break;
	}
}


/** Copy boolean.
 *
 * @param src		Source boolean
 * @param dest		Place to store pointer to new boolean
 */
static void rdata_bool_copy(rdata_bool_t *src, rdata_bool_t **dest)
{
	*dest = rdata_bool_new();
	(*dest)->value = src->value;
}

/** Copy character.
 *
 * @param src		Source character
 * @param dest		Place to store pointer to new character
 */
static void rdata_char_copy(rdata_char_t *src, rdata_char_t **dest)
{
	*dest = rdata_char_new();
	bigint_clone(&src->value, &(*dest)->value);
}

/** Copy integer.
 *
 * @param src		Source integer
 * @param dest		Place to store pointer to new integer
 */
static void rdata_int_copy(rdata_int_t *src, rdata_int_t **dest)
{
	*dest = rdata_int_new();
	bigint_clone(&src->value, &(*dest)->value);
}

/** Copy string.
 *
 * @param src		Source string.
 * @param dest		Place to store pointer to new string.
 */
static void rdata_string_copy(rdata_string_t *src, rdata_string_t **dest)
{
	*dest = rdata_string_new();
	(*dest)->value = src->value;
}

/** Copy reference.
 *
 * @param src		Source reference.
 * @param dest		Place to store pointer to new reference.
 */
static void rdata_ref_copy(rdata_ref_t *src, rdata_ref_t **dest)
{
	*dest = rdata_ref_new();
	(*dest)->vref = src->vref;
}

/** Copy delegate.
 *
 * @param src		Source delegate.
 * @param dest		Place to store pointer to new delegate.
 */
static void rdata_deleg_copy(rdata_deleg_t *src, rdata_deleg_t **dest)
{
	*dest = rdata_deleg_new();
	(*dest)->obj = src->obj;
	(*dest)->sym = src->sym;
}

/** Copy enum value.
 *
 * @param src		Source enum value.
 * @param dest		Place to store pointer to new enum value.
 */
static void rdata_enum_copy(rdata_enum_t *src, rdata_enum_t **dest)
{
	*dest = rdata_enum_new();
	(*dest)->value = src->value;
}

/** Copy array.
 *
 * @param src		Source array.
 * @param dest		Place to store pointer to new array.
 */
static void rdata_array_copy(rdata_array_t *src, rdata_array_t **dest)
{
	(void) src;
	(void) dest;
	printf("Unimplemented: Copy array.\n");
	exit(1);
}

/** Copy object.
 *
 * @param src		Source object.
 * @param dest		Place to store pointer to new object.
 */
static void rdata_object_copy(rdata_object_t *src, rdata_object_t **dest)
{
	(void) src;
	(void) dest;
	printf("Unimplemented: Copy object.\n");
	abort();
}

/** Copy resource.
 *
 * @param src		Source resource.
 * @param dest		Place to store pointer to new resource.
 */
static void rdata_resource_copy(rdata_resource_t *src, rdata_resource_t **dest)
{
	*dest = rdata_resource_new();
	(*dest)->data = src->data;
}

/** Copy symbol.
 *
 * @param src		Source symbol.
 * @param dest		Place to store pointer to new symbol.
 */
static void rdata_symbol_copy(rdata_symbol_t *src, rdata_symbol_t **dest)
{
	*dest = rdata_symbol_new();
	(*dest)->sym = src->sym;
}

/** Destroy var node.
 *
 * @param var	Var node
 */
void rdata_var_destroy(rdata_var_t *var)
{
	/* First destroy class-specific part */
	rdata_var_destroy_inner(var);

	/* Deallocate var node */
	rdata_var_delete(var);
}

/** Destroy inside of var node.
 *
 * Destroy content of var node, but do not deallocate the var node
 * itself.
 *
 * @param var	Var node
 */
static void rdata_var_destroy_inner(rdata_var_t *var)
{
	/* First destroy class-specific part */

	switch (var->vc) {
	case vc_bool:
		rdata_bool_destroy(var->u.bool_v);
		break;
	case vc_char:
		rdata_char_destroy(var->u.char_v);
		break;
	case vc_int:
		rdata_int_destroy(var->u.int_v);
		break;
	case vc_string:
		rdata_string_destroy(var->u.string_v);
		break;
	case vc_ref:
		rdata_ref_destroy(var->u.ref_v);
		break;
	case vc_deleg:
		rdata_deleg_destroy(var->u.deleg_v);
		break;
	case vc_enum:
		rdata_enum_destroy(var->u.enum_v);
		break;
	case vc_array:
		rdata_array_destroy(var->u.array_v);
		break;
	case vc_object:
		rdata_object_destroy(var->u.object_v);
		break;
	case vc_resource:
		rdata_resource_destroy(var->u.resource_v);
		break;
	case vc_symbol:
		rdata_symbol_destroy(var->u.symbol_v);
		break;
	}
}

/** Destroy item.
 *
 * Destroy an item including the value or address within.
 *
 * @param item	Item
 */
void rdata_item_destroy(rdata_item_t *item)
{
	/* First destroy class-specific part */

	switch (item->ic) {
	case ic_address:
		rdata_address_destroy(item->u.address);
		break;
	case ic_value:
		rdata_value_destroy(item->u.value);
		break;
	}

	/* Deallocate item node */
	rdata_item_delete(item);
}

/** Destroy address.
 *
 * Destroy an address node.
 *
 * @param address	Address
 */
void rdata_address_destroy(rdata_address_t *address)
{
	switch (address->ac) {
	case ac_var:
		rdata_addr_var_destroy(address->u.var_a);
		break;
	case ac_prop:
		rdata_addr_prop_destroy(address->u.prop_a);
		break;
	}

	/* Deallocate address node */
	rdata_address_delete(address);
}

/** Destroy variable address.
 *
 * Destroy a variable address node.
 *
 * @param addr_var	Variable address
 */
void rdata_addr_var_destroy(rdata_addr_var_t *addr_var)
{
	addr_var->vref = NULL;

	/* Deallocate variable address node */
	rdata_addr_var_delete(addr_var);
}

/** Destroy property address.
 *
 * Destroy a property address node.
 *
 * @param addr_prop	Property address
 */
void rdata_addr_prop_destroy(rdata_addr_prop_t *addr_prop)
{
	switch (addr_prop->apc) {
	case apc_named:
		rdata_aprop_named_destroy(addr_prop->u.named);
		break;
	case apc_indexed:
		rdata_aprop_indexed_destroy(addr_prop->u.indexed);
		break;
	}

	if (addr_prop->tvalue != NULL) {
		rdata_value_destroy(addr_prop->tvalue);
		addr_prop->tvalue = NULL;
	}

	addr_prop->tpos = NULL;

	/* Deallocate property address node */
	rdata_addr_prop_delete(addr_prop);
}

/** Destroy named property address.
 *
 * Destroy a named property address node.
 *
 * @param aprop_named	Named property address
 */
void rdata_aprop_named_destroy(rdata_aprop_named_t *aprop_named)
{
	rdata_deleg_destroy(aprop_named->prop_d);

	/* Deallocate named property address node */
	rdata_aprop_named_delete(aprop_named);
}

/** Destroy indexed property address.
 *
 * Destroy a indexed property address node.
 *
 * @param aprop_indexed		Indexed property address
 */
void rdata_aprop_indexed_destroy(rdata_aprop_indexed_t *aprop_indexed)
{
	list_node_t *arg_node;
	rdata_item_t *arg_i;

	/* Destroy the object delegate. */
	rdata_deleg_destroy(aprop_indexed->object_d);

	/*
	 * Walk through all argument items (indices) and destroy them,
	 * removing them from the list as well.
	 */
	while (!list_is_empty(&aprop_indexed->args)) {
		arg_node = list_first(&aprop_indexed->args);
		arg_i = list_node_data(arg_node, rdata_item_t *);

		rdata_item_destroy(arg_i);
		list_remove(&aprop_indexed->args, arg_node);
	}

	/* Destroy the now empty list */
	list_fini(&aprop_indexed->args);

	/* Deallocate indexed property address node */
	rdata_aprop_indexed_delete(aprop_indexed);
}

/** Destroy value.
 *
 * Destroy a value node.
 *
 * @param value		Value
 */
void rdata_value_destroy(rdata_value_t *value)
{
	/* Assumption: Var nodes in values are not shared. */
	rdata_var_destroy(value->var);

	/* Deallocate value node */
	rdata_value_delete(value);
}

/** Destroy boolean.
 *
 * @param bool_v		Boolean
 */
static void rdata_bool_destroy(rdata_bool_t *bool_v)
{
	rdata_bool_delete(bool_v);
}

/** Destroy character.
 *
 * @param char_v	Character
 */
static void rdata_char_destroy(rdata_char_t *char_v)
{
	bigint_destroy(&char_v->value);
	rdata_char_delete(char_v);
}

/** Destroy integer.
 *
 * @param int_v		Integer
 */
static void rdata_int_destroy(rdata_int_t *int_v)
{
	bigint_destroy(&int_v->value);
	rdata_int_delete(int_v);
}

/** Destroy string.
 *
 * @param string_v	String
 */
static void rdata_string_destroy(rdata_string_t *string_v)
{
	/*
	 * String values are shared so we cannot free them. Just deallocate
	 * the node.
	 */
	rdata_string_delete(string_v);
}

/** Destroy reference.
 *
 * @param ref_v		Reference
 */
static void rdata_ref_destroy(rdata_ref_t *ref_v)
{
	ref_v->vref = NULL;
	rdata_ref_delete(ref_v);
}

/** Destroy delegate.
 *
 * @param deleg_v		Reference
 */
static void rdata_deleg_destroy(rdata_deleg_t *deleg_v)
{
	deleg_v->obj = NULL;
	deleg_v->sym = NULL;
	rdata_deleg_delete(deleg_v);
}

/** Destroy enum.
 *
 * @param enum_v		Reference
 */
static void rdata_enum_destroy(rdata_enum_t *enum_v)
{
	enum_v->value = NULL;
	rdata_enum_delete(enum_v);
}

/** Destroy array.
 *
 * @param array_v		Array
 */
static void rdata_array_destroy(rdata_array_t *array_v)
{
	int d;
	size_t n_elems, p;

	/*
	 * Compute total number of elements in array.
	 * At the same time zero out the extent array.
	 */
	n_elems = 1;
	for (d = 0; d < array_v->rank; d++) {
		n_elems = n_elems * array_v->extent[d];
		array_v->extent[d] = 0;
	}

	/* Destroy all elements and zero out the array */
	for (p = 0; p < n_elems; p++) {
		rdata_var_delete(array_v->element[p]);
		array_v->element[p] = NULL;
	}

	/* Free the arrays */
	free(array_v->element);
	free(array_v->extent);

	array_v->rank = 0;

	/* Deallocate the node */
	rdata_array_delete(array_v);
}

/** Destroy object.
 *
 * @param object_v		Object
 */
static void rdata_object_destroy(rdata_object_t *object_v)
{
	/* XXX TODO */
	rdata_object_delete(object_v);
}

/** Destroy resource.
 *
 * @param resource_v		Resource
 */
static void rdata_resource_destroy(rdata_resource_t *resource_v)
{
	/*
	 * XXX Presumably this should be handled by the appropriate
	 * built-in module, so, some call-back function would be required.
	 */
	resource_v->data = NULL;
	rdata_resource_delete(resource_v);
}

/** Destroy symbol.
 *
 * @param symbol_v		Symbol
 */
static void rdata_symbol_destroy(rdata_symbol_t *symbol_v)
{
	symbol_v->sym = NULL;
	rdata_symbol_delete(symbol_v);
}

/** Read data from a variable.
 *
 * This copies data from the variable to a value item. Ideally any read access
 * to a program variable should go through this function. (Keep in mind
 * that although values are composed of var nodes internally, but are not
 * variables per se. Therefore this function is not used to read from values)
 *
 * @param var		Variable to read from (var node where it is stored).
 * @param ritem		Place to store pointer to new value item read from
 *			the variable.
 */
void rdata_var_read(rdata_var_t *var, rdata_item_t **ritem)
{
	rdata_value_t *value;
	rdata_var_t *rvar;

	/* Perform a shallow copy of @a var. */
	rdata_var_copy(var, &rvar);

	value = rdata_value_new();
	value->var = rvar;
	*ritem = rdata_item_new(ic_value);
	(*ritem)->u.value = value;
}

/** Write data to a variable.
 *
 * This copies data to the variable from a value. Ideally any write access
 * to a program variable should go through this function. (Keep in mind
 * that even though values are composed of var nodes internally, but are not
 * variables per se. Therefore this function is not used to write to values)
 *
 * @param var		Variable to write to (var node where it is stored).
 * @param value		The value to write.
 */
void rdata_var_write(rdata_var_t *var, rdata_value_t *value)
{
	/* Free old content of var->u */
	rdata_var_destroy_inner(var);

	/* Perform a shallow copy of @c value->var. */
	rdata_var_copy_to(value->var, var);
}

/** Print data item in human-readable form.
 *
 * @param item		Item to print.
 */
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

/** Print address in human-readable form.
 *
 * Actually this displays contents of the var node that is being addressed.
 *
 * XXX Maybe we should really rather print the address and not the data
 * it is pointing to?
 *
 * @param item		Address to print.
 */
static void rdata_address_print(rdata_address_t *address)
{
	switch (address->ac) {
	case ac_var:
		rdata_var_print(address->u.var_a->vref);
		break;
	case ac_prop:
		printf("Warning: Unimplemented: Print property address.\n");
		break;
	}
}

/** Print value in human-readable form.
 *
 * @param value		Value to print.
 */
void rdata_value_print(rdata_value_t *value)
{
	rdata_var_print(value->var);
}

/** Print contents of var node in human-readable form.
 *
 * @param item		Var node to print.
 */
static void rdata_var_print(rdata_var_t *var)
{
	int val;

	switch (var->vc) {
	case vc_bool:
		printf("bool(%s)", var->u.bool_v->value ? "true" : "false");
		break;
	case vc_char:
		printf("char(");
		if (bigint_get_value_int(&var->u.char_v->value, &val) == EOK)
			printf("'%c'", val);
		else
			printf("???:x%x\n", (unsigned) val);
		printf(")");
		break;
	case vc_int:
		printf("int(");
		bigint_print(&var->u.int_v->value);
		printf(")");
		break;
	case vc_string:
		printf("string(\"%s\")", var->u.string_v->value);
		break;
	case vc_ref:
		if (var->u.ref_v->vref != NULL) {
			printf("ref(");
			rdata_var_print(var->u.ref_v->vref);
			printf(")");
		} else {
			printf("nil");
		}
		break;
	case vc_deleg:
		printf("deleg(");
		if (var->u.deleg_v->sym != NULL) {
			if (var->u.deleg_v->obj != NULL) {
				rdata_var_print(var->u.deleg_v->obj);
				printf(",");
			}
			symbol_print_fqn(var->u.deleg_v->sym);
		} else {
			printf("nil");
		}
		printf(")");
		break;
	case vc_enum:
		symbol_print_fqn(
		    enum_to_symbol(var->u.enum_v->value->outer_enum));
		printf(".%s",
		    strtab_get_str(var->u.enum_v->value->name->sid));
		break;
	case vc_array:
		printf("array");
		break;
	case vc_object:
		printf("object");
		break;
	case vc_resource:
		printf("resource(%p)", var->u.resource_v->data);
		break;
	case vc_symbol:
		printf("symbol(");
		if (var->u.symbol_v->sym != NULL) {
			symbol_print_fqn(var->u.symbol_v->sym);
		} else {
			printf("nil");
		}
		printf(")");
		break;
	}
}
