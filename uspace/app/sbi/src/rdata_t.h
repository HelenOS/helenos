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

/** @file Run-time data representation. */

#ifndef RDATA_T_H_
#define RDATA_T_H_

#include "intmap_t.h"

/** Boolean variable. */
typedef struct {
	bool_t value;
} rdata_bool_t;

/** Character variable.
 *
 * Sysel character type should be able to store arbitrarily (or at least
 * very) large character sets.
 */
typedef struct {
	bigint_t value;
} rdata_char_t;

/** Integer variable.
 *
 * Sysel int type should be able to store arbitrarily (or at least
 * very) large numbers.
 */
typedef struct {
	bigint_t value;
} rdata_int_t;

/** String variable */
typedef struct {
	const char *value;
} rdata_string_t;

/** Reference variable */
typedef struct {
	struct rdata_var *vref;
} rdata_ref_t;

/** Delegate variable
 *
 * A delegate variable points to a static or non-static symbol. If the
 * symbol is non static, @c obj points to the object the symbol
 * belongs to.
 */
typedef struct {
	/** Object or @c NULL if deleg. points to a static function. */
	struct rdata_var *obj;

	/** Member symbol. */
	struct stree_symbol *sym;
} rdata_deleg_t;

/** Enumerated type value. */
typedef struct {
	/** Enum member declaration */
	struct stree_embr *value;
} rdata_enum_t;

/** Array variable */
typedef struct {
	/** Rank */
	int rank;

	/** Extents (@c rank entries) */
	int *extent;

	/**
	 * Elements (extent[0] * extent[1] * ... extent[rank - 1] entries)
	 * stored in lexicographical order. Each element is (rdata_var_t *).
	 */
	struct rdata_var **element;
} rdata_array_t;

/** Object variable */
typedef struct {
	/** Class of this object (symbol) */
	struct stree_symbol *class_sym;

	/** @c sn_static if this is a static object (i.e. class object) */
	statns_t static_obj;

	/** Map field name SID to field data */
	intmap_t fields; /* of (rdata_var_t *) */
} rdata_object_t;

/** Resource handle
 *
 * Binding to external data. This type can be used to refer to data used
 * by builtin functions (such as files).
 */
typedef struct {
	/** Only understood by the right builtin function. */
	void *data;
} rdata_resource_t;

/** Symbol reference variable
 *
 * A symbol reference points to a program symbol.
 */
typedef struct {
	/** Program symbol. */
	struct stree_symbol *sym;
} rdata_symbol_t;

typedef enum var_class {
	/** Boolean */
	vc_bool,

	/** Character */
	vc_char,

	/** Integer */
	vc_int,

	/** String */
	vc_string,

	/** Reference */
	vc_ref,

	/** Delegate */
	vc_deleg,

	/** Enumerated type value */
	vc_enum,

	/** Array */
	vc_array,

	/** Object */
	vc_object,

	/** Interpreter builtin resource */
	vc_resource,

	/** Symbol reference */
	vc_symbol
} var_class_t;

/** Variable.
 *
 * A piece of memory holding one of the basic types of data element.
 * It is addressable (via rdata_var_t *) and mutable, at least from
 * internal point of view of the interpreter.
 */
typedef struct rdata_var {
	var_class_t vc;

	union {
		rdata_bool_t *bool_v;
		rdata_char_t *char_v;
		rdata_int_t *int_v;
		rdata_string_t *string_v;
		rdata_ref_t *ref_v;
		rdata_deleg_t *deleg_v;
		rdata_enum_t *enum_v;
		rdata_array_t *array_v;
		rdata_object_t *object_v;
		rdata_resource_t *resource_v;
		rdata_symbol_t *symbol_v;
	} u;
} rdata_var_t;

/** Address class */
typedef enum {
	/** Variable address */
	ac_var,

	/** Property address */
	ac_prop
} address_class_t;

/** Variable address */
typedef struct {
	/** Targeted variable */
	rdata_var_t *vref;
} rdata_addr_var_t;

/** Named property address */
typedef struct {
	/** Delegate to the property */
	rdata_deleg_t *prop_d;
} rdata_aprop_named_t;

/** Indexed property address */
typedef struct {
	/** Delegate to the object (or CSI) which is being indexed. */
	rdata_deleg_t *object_d;

	/** Arguments (indices) */
	list_t args; /* of rdata_item_t */
} rdata_aprop_indexed_t;

typedef enum {
	/* Named property address */
	apc_named,

	/* Indexed property address */
	apc_indexed
} aprop_class_t;

/** Property address.
 *
 * When an access or index operation is performed on a property, the getter
 * is run and the prefetched value is stored in @c tvalue. If the property
 * is a non-scalar value type (a struct), then we might want to point to
 * the proper @c var node inside it. @c tpos is used for this purpose.
 *
 * The assignment operator will modify @c tvalue and at the end the setter
 * is called to store @c tvalue back into the property.
 */
typedef struct {
	aprop_class_t apc;

	/** Temporary copy of property value or @c NULL when not used. */
	struct rdata_value *tvalue;

	/**
	 * Points to the specific var node within @c tvalue that is addressed
	 * or @c NULL when @c tvalue is not used.
	 */
	rdata_var_t *tpos;

	union {
		rdata_aprop_named_t *named;
		rdata_aprop_indexed_t *indexed;
	} u;
} rdata_addr_prop_t;

/** Address item */
typedef struct rdata_address {
	address_class_t ac;

	union {
		rdata_addr_var_t *var_a;
		rdata_addr_prop_t *prop_a;
	} u;
} rdata_address_t;

/** Value item. */
typedef struct rdata_value {
	/**
	 * Read-only Variable holding a copy of the data. Currently we don't
	 * allow sharing the same @c var node between different value nodes
	 * so that when destroying the value we can destroy the var.
	 *
	 * We could share this, but would need to reference-count it.
	 */
	rdata_var_t *var;
} rdata_value_t;

typedef enum {
	/** Address of a variable. */
	ic_address,
	/** Value */
	ic_value
} item_class_t;

/** Data item.
 *
 * Data item is the result of evaluating an expression. An address expression
 * yields an address item (a.k.a. L-value), a value expression yields a data
 * item (a.k.a. R-value). This model is used to accomodate semantics of the
 * assignment operator.
 */
typedef struct rdata_item {
	item_class_t ic;

	union {
		rdata_address_t *address;
		rdata_value_t *value;
	} u;
} rdata_item_t;

#endif
