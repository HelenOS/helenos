/*
 * SPDX-FileCopyrightText: 2010 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @file Static type system representation. */

#ifndef TDATA_T_H_
#define TDATA_T_H_

#include "intmap_t.h"

/** Class of primitive type. */
typedef enum {
	/** Boolean type */
	tpc_bool,
	/** Character type */
	tpc_char,
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
	/** @c sn_static if expression is a static CSI reference */
	statns_t static_ref;

	/** CSI definition */
	struct stree_csi *csi;

	/** (Real) type arguments */
	list_t targs; /* of tdata_item_t */
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

/** Function signature type.
 *
 * This is a part of functional type or delegate type.
 */
typedef struct {
	/** Types of fixed arguments. */
	list_t arg_ti; /* of tdata_item_t */

	/** Type of variadic argument */
	struct tdata_item *varg_ti;

	/** Return type */
	struct tdata_item *rtype;
} tdata_fun_sig_t;

/** Delegate type. */
typedef struct {
	/** Delegate definition or @c NULL if anonymous delegate */
	struct stree_deleg *deleg;

	/** Delegate signature type */
	tdata_fun_sig_t *tsig;
} tdata_deleg_t;

/** Enum-base type.
 *
 * Type for expression which reference an enum declaration. In run time
 * enum type reference is represented by @c rdata_deleg_t. (Which is used
 * for any symbol references).
 */
typedef struct {
	/** Enum definition */
	struct stree_enum *enum_d;
} tdata_ebase_t;

/** Enum type. */
typedef struct {
	/** Enum definition */
	struct stree_enum *enum_d;
} tdata_enum_t;

/** Functional type. */
typedef struct {
	/** Delegate definition or @c NULL if anonymous delegate */
	struct stree_deleg *deleg;

	/** Function signature type */
	tdata_fun_sig_t *tsig;
} tdata_fun_t;

/** Type variable reference. */
typedef struct {
	/** Definition of type argument this variable is referencing. */
	struct stree_targ *targ;
} tdata_vref_t;

typedef enum {
	/** Primitive type item */
	tic_tprimitive,
	/** Object type item */
	tic_tobject,
	/** Array type item */
	tic_tarray,
	/** Delegate type item */
	tic_tdeleg,
	/** Enum-base type item */
	tic_tebase,
	/** Enum type item */
	tic_tenum,
	/** Function type item */
	tic_tfun,
	/** Type variable item */
	tic_tvref,
	/** Special error-recovery type item */
	tic_ignore
} titem_class_t;

/** Type item, the result of evaluating a type expression. */
typedef struct tdata_item {
	titem_class_t tic;

	union {
		tdata_primitive_t *tprimitive;
		tdata_object_t *tobject;
		tdata_array_t *tarray;
		tdata_deleg_t *tdeleg;
		tdata_ebase_t *tebase;
		tdata_enum_t *tenum;
		tdata_fun_t *tfun;
		tdata_vref_t *tvref;
	} u;
} tdata_item_t;

/** Type variable valuation (mapping of type argument names to values). */
typedef struct {
	/** Maps name SID to type item */
	intmap_t tvv; /* of tdata_item_t */
} tdata_tvv_t;

#endif
