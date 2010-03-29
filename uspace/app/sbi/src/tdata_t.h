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

/** @file Static type system representation. */

#ifndef TDATA_T_H_
#define TDATA_T_H_

/** Class of primitive type. */
typedef enum {
	/** Integer type */
	tpc_int,
	/** Special type for nil reference */
	tpc_nil,
	/** String type */
	tpc_string,
	/** Resource type */
	tpc_resource
} tprimitive_class_t;

/** Primitive type. */
typedef struct {
	/** Class of primitive type */
	tprimitive_class_t tpc;
} tdata_primitive_t;

/** Object type. */
typedef struct {
	/** @c true if expression is a static CSI reference */
	bool_t static_ref;

	/** CSI definition */
	struct stree_csi *csi;
} tdata_object_t;

/** Array type. */
typedef struct {
	/** Base type item */
	struct tdata_item *base_ti;

	/** Rank */
	int rank;

	/** Extents */
	list_t extents; /* of stree_expr_t */
} tdata_array_t;

/** Generic type. */
typedef struct {
} tdata_generic_t;

/** Functional type. */
typedef struct {
	/**
	 * Function definition. We'll leave expansion to the call operation.
	 */
	struct stree_fun *fun;
} tdata_fun_t;

typedef enum {
	tic_tprimitive,
	tic_tobject,
	tic_tarray,
	tic_tgeneric,
	tic_tfun
} titem_class_t;

/** Type item, the result of evaluating a type expression. */
typedef struct tdata_item {
	titem_class_t tic;

	union {
		tdata_primitive_t *tprimitive;
		tdata_object_t *tobject;
		tdata_array_t *tarray;
		tdata_generic_t *tgeneric;
		tdata_fun_t *tfun;
	} u;
} tdata_item_t;

#endif
