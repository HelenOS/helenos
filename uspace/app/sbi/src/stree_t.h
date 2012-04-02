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

#ifndef STREE_T_H_
#define STREE_T_H_

#include "bigint_t.h"
#include "list_t.h"
#include "builtin_t.h"

/*
 * Arithmetic expressions
 */

struct stree_expr;

/** Identifier */
typedef struct {
	int sid;
	struct cspan *cspan;
} stree_ident_t;

/** Name reference */
typedef struct {
	/** Expression backlink */
	struct stree_expr *expr;

	stree_ident_t *name;
} stree_nameref_t;

/** Boolean literal */
typedef struct {
	bool_t value;
} stree_lit_bool_t;

/** Character literal */
typedef struct {
	bigint_t value;
} stree_lit_char_t;

/** Integer literal */
typedef struct {
	bigint_t value;
} stree_lit_int_t;

/** Reference literal (there is only one: @c nil). */
typedef struct {
} stree_lit_ref_t;

/** String literal */
typedef struct {
	char *value;
} stree_lit_string_t;

typedef enum {
	ltc_bool,
	ltc_char,
	ltc_int,
	ltc_ref,
	ltc_string
} literal_class_t;

/** Literal */
typedef struct {
	/** Expression backlink */
	struct stree_expr *expr;

	literal_class_t ltc;
	union {
		stree_lit_bool_t lit_bool;
		stree_lit_char_t lit_char;
		stree_lit_int_t lit_int;
		stree_lit_ref_t lit_ref;
		stree_lit_string_t lit_string;
	} u;
} stree_literal_t;

/** Reference to currently active object. */
typedef struct {
	/** Expression backlink */
	struct stree_expr *expr;
} stree_self_ref_t;

/** Binary operation class */
typedef enum {
	bo_equal,
	bo_notequal,
	bo_lt,
	bo_gt,
	bo_lt_equal,
	bo_gt_equal,
	bo_plus,
	bo_minus,
	bo_mult,
	bo_and,
	bo_or
} binop_class_t;

/** Unary operation class */
typedef enum {
	uo_plus,
	uo_minus,
	uo_not
} unop_class_t;

/** Binary operation */
typedef struct {
	/** Expression backlink */
	struct stree_expr *expr;

	/** Binary operation class */
	binop_class_t bc;

	/** Arguments */
	struct stree_expr *arg1, *arg2;
} stree_binop_t;

/** Unary operation */
typedef struct {
	/** Expression backlink */
	struct stree_expr *expr;

	/** Operation class */
	unop_class_t uc;

	/** Argument */
	struct stree_expr *arg;
} stree_unop_t;

/** New operation */
typedef struct {
	/** Expression backlink */
	struct stree_expr *expr;

	/** Type of object to construct. */
	struct stree_texpr *texpr;

	/** Constructor arguments */
	list_t ctor_args; /* of stree_expr_t */
} stree_new_t;

/** Member access operation */
typedef struct {
	/** Expression backlink */
	struct stree_expr *expr;

	/** Argument */
	struct stree_expr *arg;
	/** Name of member being accessed. */
	stree_ident_t *member_name;
} stree_access_t;

/** Function call operation */
typedef struct {
	/** Expression backlink */
	struct stree_expr *expr;

	/** Function */
	struct stree_expr *fun;

	/** Arguments */
	list_t args; /* of stree_expr_t */
} stree_call_t;

typedef enum {
	ac_set,
	ac_increase
} assign_class_t;

/** Assignment */
typedef struct {
	/** Expression backlink */
	struct stree_expr *expr;

	assign_class_t ac;
	struct stree_expr *dest, *src;
} stree_assign_t;

/** Indexing operation */
typedef struct {
	/** Expression backlink */
	struct stree_expr *expr;

	/** Base */
	struct stree_expr *base;

	/** Arguments (indices) */
	list_t args; /* of stree_expr_t */
} stree_index_t;

/** @c as conversion operation */
typedef struct {
	/** Expression backlink */
	struct stree_expr *expr;

	/** Expression to convert */
	struct stree_expr *arg;

	/** Destination type of conversion. */
	struct stree_texpr *dtype;
} stree_as_t;

/** Boxing of primitive type (pseudo)
 *
 * This pseudo-node is used internally to box a value of primitive type.
 * It is implicitly inserted by stype_convert(). It does not correspond
 * to a an explicit program construct.
 */
typedef struct {
	/** Expression backlink */
	struct stree_expr *expr;

	/* Primitive type expression */
	struct stree_expr *arg;
} stree_box_t;

/** Arithmetic expression class */
typedef enum {
	ec_nameref,
	ec_literal,
	ec_self_ref,
	ec_binop,
	ec_unop,
	ec_new,
	ec_access,
	ec_call,
	ec_assign,
	ec_index,
	ec_as,
	ec_box
} expr_class_t;

/** Arithmetic expression */
typedef struct stree_expr {
	expr_class_t ec;

	/** Type of this expression or @c NULL if not typed yet */
	struct tdata_item *titem;

	/** Coordinate span */
	struct cspan *cspan;

	union {
		stree_nameref_t *nameref;
		stree_literal_t *literal;
		stree_self_ref_t *self_ref;
		stree_binop_t *binop;
		stree_unop_t *unop;
		stree_new_t *new_op;
		stree_access_t *access;
		stree_call_t *call;
		stree_index_t *index;
		stree_assign_t *assign;
		stree_as_t *as_op;
		stree_box_t *box;
	} u;
} stree_expr_t;

/*
 * Type expressions
 */

struct stree_texpr;

/** Type literal class */
typedef enum {
	tlc_bool,
	tlc_char,
	tlc_int,
	tlc_resource,
	tlc_string
} tliteral_class_t;

/** Type literal */
typedef struct {
	/** Type expression backlink */
	struct stree_texpr *texpr;

	tliteral_class_t tlc;
} stree_tliteral_t;

/** Type name reference */
typedef struct {
	/** Type expression backlink */
	struct stree_texpr *texpr;

	stree_ident_t *name;
} stree_tnameref_t;

/** Type member access operation */
typedef struct {
	/** Type expression backlink */
	struct stree_texpr *texpr;

	/** Argument */
	struct stree_texpr *arg;

	/** Name of member being accessed. */
	stree_ident_t *member_name;
} stree_taccess_t;

/** Type application operation */
typedef struct {
	/** Type expression backlink */
	struct stree_texpr *texpr;

	/* Base type */
	struct stree_texpr *gtype;

	/** (Formal) type arguments */
	list_t targs; /* of stree_texpr_t */
} stree_tapply_t;

/** Type index operation */
typedef struct {
	/** Type expression backlink */
	struct stree_texpr *texpr;

	/** Base type */
	struct stree_texpr *base_type;

	/**
	 * Number of arguments (rank). Needed when only rank is specified
	 * and @c args are not used.
	 */
	int n_args;

	/** Arguments (extents) */
	list_t args; /* of stree_expr_t */
} stree_tindex_t;

/** Type expression class */
typedef enum {
	tc_tliteral,
	tc_tnameref,
	tc_taccess,
	tc_tapply,
	tc_tindex
} texpr_class_t;

/** Type expression */
typedef struct stree_texpr {
	texpr_class_t tc;

	/** Coordinate span */
	struct cspan *cspan;

	union {
		stree_tliteral_t *tliteral;
		stree_tnameref_t *tnameref;
		stree_taccess_t *taccess;
		stree_tapply_t *tapply;
		stree_tindex_t *tindex;
	} u;
} stree_texpr_t;

/*
 * Statements, class members and module members.
 */

/** Statement block */
typedef struct stree_block {
	/** List of statements in the block */
	list_t stats; /* of stree_stat_t */
} stree_block_t;

/** Variable declaration */
typedef struct {
	stree_ident_t *name;
	stree_texpr_t *type;

	/** Type of this variable or @c NULL if not typed yet */
	struct tdata_item *titem;
} stree_vdecl_t;

/** @c except clause */
typedef struct {
	stree_ident_t *evar;
	stree_texpr_t *etype;
	stree_block_t *block;

	/** Evaluated etype or @c NULL if not typed yet */
	struct tdata_item *titem;
} stree_except_t;

/** @c if or @c elif clause */
typedef struct {
	stree_expr_t *cond;
	stree_block_t *block;
} stree_if_clause_t;

/** If statement */
typedef struct {
	/** If and elif clauses */
	list_t if_clauses; /* of stree_if_clause_t */

	/** Else block */
	stree_block_t *else_block;
} stree_if_t;

/** @c when clause */
typedef struct {
	/** List of expressions -- cases -- for this clause */
	list_t exprs; /* of stree_expr_t */
	stree_block_t *block;
} stree_when_t;

/** Switch statement */
typedef struct {
	/** Switch expression */
	stree_expr_t *expr;

	/** When clauses */
	list_t when_clauses; /* of stree_when_t */

	/** Else block */
	stree_block_t *else_block;
} stree_switch_t;

/** While statement */
typedef struct {
	stree_expr_t *cond;
	stree_block_t *body;
} stree_while_t;

/** For statement */
typedef struct {
	stree_block_t *body;
} stree_for_t;

/** Raise statement */
typedef struct {
	stree_expr_t *expr;
} stree_raise_t;

/** Break statement */
typedef struct {
} stree_break_t;

/** Return statement */
typedef struct {
	stree_expr_t *expr;
} stree_return_t;

/** Expression statement */
typedef struct {
	stree_expr_t *expr;
} stree_exps_t;

/** With-try-except-finally (WEF) statement */
typedef struct {
	stree_block_t *with_block;
	list_t except_clauses; /* of stree_except_t */
	stree_block_t *finally_block;
} stree_wef_t;

/** Statement class */
typedef enum {
	st_vdecl,
	st_if,
	st_switch,
	st_while,
	st_for,
	st_raise,
	st_break,
	st_return,
	st_exps,
	st_wef
} stat_class_t;

/** Statement */
typedef struct {
	stat_class_t sc;

	union {
		stree_vdecl_t *vdecl_s;
		stree_if_t *if_s;
		stree_switch_t *switch_s;
		stree_while_t *while_s;
		stree_for_t *for_s;
		stree_raise_t *raise_s;
		stree_break_t *break_s;
		stree_return_t *return_s;
		stree_exps_t *exp_s;
		stree_wef_t *wef_s;
	} u;
} stree_stat_t;

/** Argument attribute class */
typedef enum {
	/** Packed argument (for variadic functions) */
	aac_packed
} arg_attr_class_t;

/** Argument atribute */
typedef struct {
	arg_attr_class_t aac;
} stree_arg_attr_t;

/** Formal function parameter */
typedef struct {
	/* Argument name */
	stree_ident_t *name;

	/* Argument type */
	stree_texpr_t *type;

	/* Attributes */
	list_t attr; /* of stree_arg_attr_t */
} stree_proc_arg_t;

/** Function signature.
 *
 * Formal parameters and return type. This is common to function and delegate
 * delcarations.
 */
typedef struct {
	/** Formal parameters */
	list_t args; /* of stree_proc_arg_t */

	/** Variadic argument or @c NULL if none. */
	stree_proc_arg_t *varg;

	/** Return type */
	stree_texpr_t *rtype;
} stree_fun_sig_t;

/** Procedure
 *
 * Procedure is the common term for a getter, setter or function body.
 * A procedure can be invoked. However, the arguments are specified by
 * the containing symbol.
 */
typedef struct stree_proc {
	/** Symbol (function or property) containing the procedure */
	struct stree_symbol *outer_symbol;

	/** Main block for regular procedures */
	stree_block_t *body;

	/** Builtin handler for builtin procedures */
	builtin_proc_t bi_handler;
} stree_proc_t;

/** Constructor declaration */
typedef struct stree_ctor {
	/** Constructor 'name'. Points to the @c new keyword. */
	stree_ident_t *name;

	/** Symbol */
	struct stree_symbol *symbol;

	/** Signature (arguments, return type is always none) */
	stree_fun_sig_t *sig;

	/** Constructor implementation */
	stree_proc_t *proc;

	/** Type item describing the constructor */
	struct tdata_item *titem;
} stree_ctor_t;

/** Delegate declaration */
typedef struct stree_deleg {
	/** Delegate name */
	stree_ident_t *name;

	/** Symbol */
	struct stree_symbol *symbol;

	/** Signature (arguments and return type) */
	stree_fun_sig_t *sig;

	/** Type item describing the delegate */
	struct tdata_item *titem;
} stree_deleg_t;

/** Enum member */
typedef struct stree_embr {
	/** Enum containing this declaration */
	struct stree_enum *outer_enum;

	/** Enum member name */
	stree_ident_t *name;
} stree_embr_t;

/** Enum declaration */
typedef struct stree_enum {
	/** Enum name */
	stree_ident_t *name;

	/** Symbol */
	struct stree_symbol *symbol;

	/** List of enum members */
	list_t members; /* of stree_embr_t */

	/** Type item describing the enum */
	struct tdata_item *titem;
} stree_enum_t;

/** Member function declaration */
typedef struct stree_fun {
	/** Function name */
	stree_ident_t *name;

	/** Symbol */
	struct stree_symbol *symbol;

	/** Signature (arguments and return type) */
	stree_fun_sig_t *sig;

	/** Function implementation */
	stree_proc_t *proc;

	/** Type item describing the function */
	struct tdata_item *titem;
} stree_fun_t;

/** Member variable declaration */
typedef struct stree_var {
	stree_ident_t *name;
	struct stree_symbol *symbol;
	stree_texpr_t *type;
} stree_var_t;

/** Member property declaration */
typedef struct stree_prop {
	stree_ident_t *name;
	struct stree_symbol *symbol;
	stree_texpr_t *type;

	stree_proc_t *getter;

	stree_proc_t *setter;
	stree_proc_arg_t *setter_arg;

	/** Formal parameters (for indexed properties) */
	list_t args; /* of stree_proc_arg_t */

	/** Variadic argument or @c NULL if none. */
	stree_proc_arg_t *varg;

	/** Type of the property */
	struct tdata_item *titem;
} stree_prop_t;

/**
 * Fake identifiers used with symbols that do not really have one.
 */
#define CTOR_IDENT "$ctor"
#define INDEXER_IDENT "$indexer"

typedef enum {
	csimbr_csi,
	csimbr_ctor,
	csimbr_deleg,
	csimbr_enum,
	csimbr_fun,
	csimbr_var,
	csimbr_prop
} csimbr_class_t;

/** Class, struct or interface member */
typedef struct {
	csimbr_class_t cc;

	union {
		struct stree_csi *csi;
		stree_ctor_t *ctor;
		stree_deleg_t *deleg;
		stree_enum_t *enum_d;
		stree_fun_t *fun;
		stree_var_t *var;
		stree_prop_t *prop;
	} u;
} stree_csimbr_t;

typedef enum {
	csi_class,
	csi_struct,
	csi_interface
} csi_class_t;

/** CSI formal type argument */
typedef struct stree_targ {
	stree_ident_t *name;
	struct stree_symbol *symbol;
} stree_targ_t;

/** Class, struct or interface declaration */
typedef struct stree_csi {
	/** Which of class, struct or interface */
	csi_class_t cc;

	/** Name of this CSI */
	stree_ident_t *name;

	/** List of type arguments */
	list_t targ; /* of stree_targ_t */

	/** Symbol for this CSI */
	struct stree_symbol *symbol;

	/** Type expressions referencing inherited CSIs. */
	list_t inherit; /* of stree_texpr_t */

	/** Base CSI. Only available when ancr_state == ws_visited. */
	struct stree_csi *base_csi;

	/** Types of implemented or accumulated interfaces. */
	list_t impl_if_ti; /* of tdata_item_t */

	/** Node state for ancr walks. */
	walk_state_t ancr_state;

	/** List of CSI members */
	list_t members; /* of stree_csimbr_t */
} stree_csi_t;

typedef enum {
	/* Class, struct or interface declaration */
	mc_csi,
	/* Enum declaration */
	mc_enum
} modm_class_t;

/** Module member */
typedef struct {
	modm_class_t mc;
	union {
		stree_csi_t *csi;
		stree_enum_t *enum_d;
	} u;
} stree_modm_t;

/** Module */
typedef struct stree_module {
	/** List of module members */
	list_t members; /* of stree_modm_t */
} stree_module_t;

/** Symbol attribute class */
typedef enum {
	/** Builtin symbol (interpreter hook) */
	sac_builtin,

	/** Static symbol */
	sac_static
} symbol_attr_class_t;

/** Symbol atribute */
typedef struct {
	symbol_attr_class_t sac;
} stree_symbol_attr_t;

typedef enum {
	/** CSI (class, struct or interface) */
	sc_csi,
	/** Constructor */
	sc_ctor,
	/** Member delegate */
	sc_deleg,
	/** Enum */
	sc_enum,
	/** Member function */
	sc_fun,
	/** Member variable */
	sc_var,
	/** Member property */
	sc_prop
} symbol_class_t;

/** Symbol
 *
 * A symbol is a common superclass of different program elements that
 * allow us to refer to them, print their fully qualified names, etc.
 */
typedef struct stree_symbol {
	symbol_class_t sc;

	union {
		struct stree_csi *csi;
		stree_ctor_t *ctor;
		stree_deleg_t *deleg;
		stree_enum_t *enum_d;
		stree_fun_t *fun;
		stree_var_t *var;
		stree_prop_t *prop;
	} u;

	/** Containing CSI */
	stree_csi_t *outer_csi;

	/** Symbol attributes */
	list_t attr; /* of stree_symbol_attr_t */
} stree_symbol_t;

/** Program */
typedef struct stree_program {
	/** The one and only module in the program */
	stree_module_t *module;

	/** Builtin symbols binding. */
	struct builtin *builtin;
} stree_program_t;

#endif
