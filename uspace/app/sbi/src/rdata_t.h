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


#ifndef RDATA_T_H_
#define RDATA_T_H_

#include "intmap_t.h"

/** Integer variable */
typedef struct {
	/*
	 * Note: Sysel int type should be able to store arbitrarily large
	 * numbers. But for now we can live with limited width.
	 */
	int value;
} rdata_int_t;

/** String variable */
typedef struct {
	char *value;
} rdata_string_t;

/** Reference variable */
typedef struct {
	struct rdata_var *vref;
} rdata_ref_t;

/** Delegate variable
 *
 * A delegate variable points to a static or non-static symbol. If the
 * symbol is non static, @c obj points to the object the symbol instance
 * belongs to.
 */
typedef struct {
	/** Object or @c NULL if deleg. points to a CSI or static member. */
	struct rdata_var *obj;

	/** Member symbol. */
	struct stree_symbol *sym;
} rdata_deleg_t;

/** Object variable */
typedef struct {
	/** Class of this object (symbol) */
	struct stree_symbol *class_sym;

	/** Map field name SID to field data */
	intmap_t fields; /* of (rdata_var_t *) */
} rdata_object_t;

typedef enum var_class {
	/** Integer */
	vc_int,

	/** String */
	vc_string,

	/** Reference */
	vc_ref,

	/** Delegate */
	vc_deleg,

	/** Object */
	vc_object
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
		rdata_int_t *int_v;
		rdata_string_t *string_v;
		rdata_ref_t *ref_v;
		rdata_deleg_t *deleg_v;
		rdata_object_t *object_v;
	} u;
} rdata_var_t;

/** Address item. */
typedef struct {
	/** Targeted variable */
	rdata_var_t *vref;
} rdata_address_t;

/** Value item. */
typedef struct {
	/**
	 * Read-only Variable holding a copy of the data. The same @c var
	 * can be shared between different instances of @c rdata_value_t.
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
