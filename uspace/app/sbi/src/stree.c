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

/** @file Stree (syntax tree) intermediate representation. */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "list.h"
#include "mytypes.h"

#include "stree.h"

/** Allocate new module.
 *
 * @return	New module
 */
stree_module_t *stree_module_new(void)
{
	stree_module_t *module;

	module = calloc(1, sizeof(stree_module_t));
	if (module == NULL) {
		printf("Memory allocation failed.\n");
		exit(1);
	}

	list_init(&module->members);
	return module;
}

/** Allocate new module member.
 *
 * @param mc	Module member class
 * @return	New module member
 */
stree_modm_t *stree_modm_new(modm_class_t mc)
{
	stree_modm_t *modm;

	modm = calloc(1, sizeof(stree_modm_t));
	if (modm == NULL) {
		printf("Memory allocation failed.\n");
		exit(1);
	}

	modm->mc = mc;
	return modm;
}

/** Allocate new CSI.
 *
 * @param cc	CSI class
 * @return	New CSI
 */
stree_csi_t *stree_csi_new(csi_class_t cc)
{
	stree_csi_t *csi;

	csi = calloc(1, sizeof(stree_csi_t));
	if (csi == NULL) {
		printf("Memory allocation failed.\n");
		exit(1);
	}

	csi->cc = cc;
	csi->ancr_state = ws_unvisited;
	csi->name = NULL;
	csi->base_csi = NULL;
	list_init(&csi->inherit);
	list_init(&csi->impl_if_ti);
	list_init(&csi->members);

	return csi;
}

/** Allocate new CSI member.
 *
 * @param cc	CSI member class
 * @return	New CSI member
 */
stree_csimbr_t *stree_csimbr_new(csimbr_class_t cc)
{
	stree_csimbr_t *csimbr;

	csimbr = calloc(1, sizeof(stree_csimbr_t));
	if (csimbr == NULL) {
		printf("Memory allocation failed.\n");
		exit(1);
	}

	csimbr->cc = cc;
	return csimbr;
}

/** Allocate new constructor.
 *
 * @return	New constructor
 */
stree_ctor_t *stree_ctor_new(void)
{
	stree_ctor_t *ctor;

	ctor = calloc(1, sizeof(stree_ctor_t));
	if (ctor == NULL) {
		printf("Memory allocation failed.\n");
		exit(1);
	}

	return ctor;
}

/** Allocate new member delegate.
 *
 * @return	New member delegate
 */
stree_deleg_t *stree_deleg_new(void)
{
	stree_deleg_t *deleg;

	deleg = calloc(1, sizeof(stree_deleg_t));
	if (deleg == NULL) {
		printf("Memory allocation failed.\n");
		exit(1);
	}

	return deleg;
}

/** Allocate new enum.
 *
 * @return	New enum
 */
stree_enum_t *stree_enum_new(void)
{
	stree_enum_t *enum_d;

	enum_d = calloc(1, sizeof(stree_enum_t));
	if (enum_d == NULL) {
		printf("Memory allocation failed.\n");
		exit(1);
	}

	return enum_d;
}

/** Allocate new enum member.
 *
 * @return	New enum member
 */
stree_embr_t *stree_embr_new(void)
{
	stree_embr_t *embr;

	embr = calloc(1, sizeof(stree_embr_t));
	if (embr == NULL) {
		printf("Memory allocation failed.\n");
		exit(1);
	}

	return embr;
}

/** Allocate new member function.
 *
 * @return	New member function
 */
stree_fun_t *stree_fun_new(void)
{
	stree_fun_t *fun;

	fun = calloc(1, sizeof(stree_fun_t));
	if (fun == NULL) {
		printf("Memory allocation failed.\n");
		exit(1);
	}

	return fun;
}

/** Allocate new member variable.
 *
 * @return	New member variable
 */
stree_var_t *stree_var_new(void)
{
	stree_var_t *var;

	var = calloc(1, sizeof(stree_var_t));
	if (var == NULL) {
		printf("Memory allocation failed.\n");
		exit(1);
	}

	return var;
}

/** Allocate new property.
 *
 * @return	New property
 */
stree_prop_t *stree_prop_new(void)
{
	stree_prop_t *prop;

	prop = calloc(1, sizeof(stree_prop_t));
	if (prop == NULL) {
		printf("Memory allocation failed.\n");
		exit(1);
	}

	return prop;
}

/** Allocate new type argument.
 *
 * @return	New type argument
 */
stree_targ_t *stree_targ_new(void)
{
	stree_targ_t *targ;

	targ = calloc(1, sizeof(stree_targ_t));
	if (targ == NULL) {
		printf("Memory allocation failed.\n");
		exit(1);
	}

	return targ;
}

/** Allocate new symbol attribute.
 *
 * @param sac	Symbol attribute class
 * @return	New symbol attribute
 */
stree_symbol_attr_t *stree_symbol_attr_new(symbol_attr_class_t sac)
{
	stree_symbol_attr_t *symbol_attr;

	symbol_attr = calloc(1, sizeof(stree_symbol_attr_t));
	if (symbol_attr == NULL) {
		printf("Memory allocation failed.\n");
		exit(1);
	}

	symbol_attr->sac = sac;
	return symbol_attr;
}

/** Allocate new procedure.
 *
 * @return	New procedure
 */
stree_proc_t *stree_proc_new(void)
{
	stree_proc_t *proc;

	proc = calloc(1, sizeof(stree_proc_t));
	if (proc == NULL) {
		printf("Memory allocation failed.\n");
		exit(1);
	}

	return proc;
}

/** Allocate new procedure argument.
 *
 * @return	New procedure argument
 */
stree_proc_arg_t *stree_proc_arg_new(void)
{
	stree_proc_arg_t *proc_arg;

	proc_arg = calloc(1, sizeof(stree_proc_arg_t));
	if (proc_arg == NULL) {
		printf("Memory allocation failed.\n");
		exit(1);
	}

	return proc_arg;
}

/** Allocate new function signature.
 *
 * @return	New procedure argument
 */
stree_fun_sig_t *stree_fun_sig_new(void)
{
	stree_fun_sig_t *fun_sig;

	fun_sig = calloc(1, sizeof(stree_fun_sig_t));
	if (fun_sig == NULL) {
		printf("Memory allocation failed.\n");
		exit(1);
	}

	return fun_sig;
}

/** Allocate new procedure argument attribute.
 *
 * @param	Argument attribute class
 * @return	New procedure argument attribute
 */
stree_arg_attr_t *stree_arg_attr_new(arg_attr_class_t aac)
{
	stree_arg_attr_t *arg_attr;

	arg_attr = calloc(1, sizeof(stree_arg_attr_t));
	if (arg_attr == NULL) {
		printf("Memory allocation failed.\n");
		exit(1);
	}

	arg_attr->aac = aac;
	return arg_attr;
}

/** Allocate new statement.
 *
 * @param sc	Statement class
 * @return	New statement
 */
stree_stat_t *stree_stat_new(stat_class_t sc)
{
	stree_stat_t *stat;

	stat = calloc(1, sizeof(stree_stat_t));
	if (stat == NULL) {
		printf("Memory allocation failed.\n");
		exit(1);
	}

	stat->sc = sc;
	return stat;
}

/** Allocate new local variable declaration.
 *
 * @return	New local variable declaration
 */
stree_vdecl_t *stree_vdecl_new(void)
{
	stree_vdecl_t *vdecl_s;

	vdecl_s = calloc(1, sizeof(stree_vdecl_t));
	if (vdecl_s == NULL) {
		printf("Memory allocation failed.\n");
		exit(1);
	}

	return vdecl_s;
}

/** Allocate new @c if statement.
 *
 * @return	New @c if statement
 */
stree_if_t *stree_if_new(void)
{
	stree_if_t *if_s;

	if_s = calloc(1, sizeof(stree_if_t));
	if (if_s == NULL) {
		printf("Memory allocation failed.\n");
		exit(1);
	}

	return if_s;
}

/** Allocate new @c switch statement.
 *
 * @return	New @c if statement
 */
stree_switch_t *stree_switch_new(void)
{
	stree_switch_t *switch_s;

	switch_s = calloc(1, sizeof(stree_switch_t));
	if (switch_s == NULL) {
		printf("Memory allocation failed.\n");
		exit(1);
	}

	return switch_s;
}

/** Allocate new @c while statement.
 *
 * @return	New @c while statement
 */
stree_while_t *stree_while_new(void)
{
	stree_while_t *while_s;

	while_s = calloc(1, sizeof(stree_while_t));
	if (while_s == NULL) {
		printf("Memory allocation failed.\n");
		exit(1);
	}

	return while_s;
}

/** Allocate new @c for statement.
 *
 * @return	New @c for statement
 */
stree_for_t *stree_for_new(void)
{
	stree_for_t *for_s;

	for_s = calloc(1, sizeof(stree_for_t));
	if (for_s == NULL) {
		printf("Memory allocation failed.\n");
		exit(1);
	}

	return for_s;
}

/** Allocate new @c raise statement.
 *
 * @return	New @c raise statement
 */
stree_raise_t *stree_raise_new(void)
{
	stree_raise_t *raise_s;

	raise_s = calloc(1, sizeof(stree_raise_t));
	if (raise_s == NULL) {
		printf("Memory allocation failed.\n");
		exit(1);
	}

	return raise_s;
}

/** Allocate new @c break statement.
 *
 * @return	New @c break statement
 */
stree_break_t *stree_break_new(void)
{
	stree_break_t *break_s;

	break_s = calloc(1, sizeof(stree_break_t));
	if (break_s == NULL) {
		printf("Memory allocation failed.\n");
		exit(1);
	}

	return break_s;
}

/** Allocate new @c return statement.
 *
 * @return	New @c return statement
 */
stree_return_t *stree_return_new(void)
{
	stree_return_t *return_s;

	return_s = calloc(1, sizeof(stree_return_t));
	if (return_s == NULL) {
		printf("Memory allocation failed.\n");
		exit(1);
	}

	return return_s;
}

/** Allocate new with-except-finally statement.
 *
 * @return	New with-except-finally statement.
 */
stree_wef_t *stree_wef_new(void)
{
	stree_wef_t *wef_s;

	wef_s = calloc(1, sizeof(stree_wef_t));
	if (wef_s == NULL) {
		printf("Memory allocation failed.\n");
		exit(1);
	}

	return wef_s;
}

/** Allocate new expression statement.
 *
 * @return	New expression statement
 */
stree_exps_t *stree_exps_new(void)
{
	stree_exps_t *exp_s;

	exp_s = calloc(1, sizeof(stree_exps_t));
	if (exp_s == NULL) {
		printf("Memory allocation failed.\n");
		exit(1);
	}

	return exp_s;
}

/** Allocate new @c except clause.
 *
 * @return	New @c except clause
 */
stree_except_t *stree_except_new(void)
{
	stree_except_t *except_c;

	except_c = calloc(1, sizeof(stree_except_t));
	if (except_c == NULL) {
		printf("Memory allocation failed.\n");
		exit(1);
	}

	return except_c;
}

/** Allocate new @c if/elif clause.
 *
 * @return	New @c if/elif clause
 */
stree_if_clause_t *stree_if_clause_new(void)
{
	stree_if_clause_t *if_clause;

	if_clause = calloc(1, sizeof(stree_if_clause_t));
	if (if_clause == NULL) {
		printf("Memory allocation failed.\n");
		exit(1);
	}

	return if_clause;
}

/** Allocate new @c when clause.
 *
 * @return	New @c when clause
 */
stree_when_t *stree_when_new(void)
{
	stree_when_t *when_c;

	when_c = calloc(1, sizeof(stree_when_t));
	if (when_c == NULL) {
		printf("Memory allocation failed.\n");
		exit(1);
	}

	return when_c;
}

/** Allocate new statement block.
 *
 * @return	New statement block
 */
stree_block_t *stree_block_new(void)
{
	stree_block_t *block;

	block = calloc(1, sizeof(stree_block_t));
	if (block == NULL) {
		printf("Memory allocation failed.\n");
		exit(1);
	}

	return block;
}

/** Allocate new expression.
 *
 * @param ec	Expression class
 * @return	New expression
 */
stree_expr_t *stree_expr_new(expr_class_t ec)
{
	stree_expr_t *expr;

	expr = calloc(1, sizeof(stree_expr_t));
	if (expr == NULL) {
		printf("Memory allocation failed.\n");
		exit(1);
	}

	expr->ec = ec;
	return expr;
}

/** Allocate new assignment.
 *
 * @param ac	Assignment class
 * @return	New assignment
 */
stree_assign_t *stree_assign_new(assign_class_t ac)
{
	stree_assign_t *assign;

	assign = calloc(1, sizeof(stree_assign_t));
	if (assign == NULL) {
		printf("Memory allocation failed.\n");
		exit(1);
	}

	assign->ac = ac;
	return assign;
}

/** Allocate new binary operation.
 *
 * @return	New binary operation
 */
stree_binop_t *stree_binop_new(binop_class_t bc)
{
	stree_binop_t *binop;

	binop = calloc(1, sizeof(stree_binop_t));
	if (binop == NULL) {
		printf("Memory allocation failed.\n");
		exit(1);
	}

	binop->bc = bc;
	return binop;
}

/** Allocate new unary operation.
 *
 * @param uc	Unary operation class
 * @return	New unary operation
 */
stree_unop_t *stree_unop_new(unop_class_t uc)
{
	stree_unop_t *unop;

	unop = calloc(1, sizeof(stree_unop_t));
	if (unop == NULL) {
		printf("Memory allocation failed.\n");
		exit(1);
	}

	unop->uc = uc;
	return unop;
}

/** Allocate new @c new operation.
 *
 * @return	New @c new operation
 */
stree_new_t *stree_new_new(void)
{
	stree_new_t *new_op;

	new_op = calloc(1, sizeof(stree_new_t));
	if (new_op == NULL) {
		printf("Memory allocation failed.\n");
		exit(1);
	}

	return new_op;
}

/** Allocate new .
 *
 * @return	New
 */
stree_access_t *stree_access_new(void)
{
	stree_access_t *access;

	access = calloc(1, sizeof(stree_access_t));
	if (access == NULL) {
		printf("Memory allocation failed.\n");
		exit(1);
	}

	return access;
}

/** Allocate new function call operation.
 *
 * @return	New function call operation
 */
stree_call_t *stree_call_new(void)
{
	stree_call_t *call;

	call = calloc(1, sizeof(stree_call_t));
	if (call == NULL) {
		printf("Memory allocation failed.\n");
		exit(1);
	}

	return call;
}

/** Allocate new indexing operation.
 *
 * @return	New indexing operation
 */
stree_index_t *stree_index_new(void)
{
	stree_index_t *index;

	index = calloc(1, sizeof(stree_index_t));
	if (index == NULL) {
		printf("Memory allocation failed.\n");
		exit(1);
	}

	return index;
}

/** Allocate new as conversion.
 *
 * @return	New as conversion
 */
stree_as_t *stree_as_new(void)
{
	stree_as_t *as_expr;

	as_expr = calloc(1, sizeof(stree_as_t));
	if (as_expr == NULL) {
		printf("Memory allocation failed.\n");
		exit(1);
	}

	return as_expr;
}

/** Allocate new boxing operation.
 *
 * @return	New boxing operation
 */
stree_box_t *stree_box_new(void)
{
	stree_box_t *box_expr;

	box_expr = calloc(1, sizeof(stree_box_t));
	if (box_expr == NULL) {
		printf("Memory allocation failed.\n");
		exit(1);
	}

	return box_expr;
}

/** Allocate new name reference operation.
 *
 * @return	New name reference operation
 */
stree_nameref_t *stree_nameref_new(void)
{
	stree_nameref_t *nameref;

	nameref = calloc(1, sizeof(stree_nameref_t));
	if (nameref == NULL) {
		printf("Memory allocation failed.\n");
		exit(1);
	}

	return nameref;
}

/** Allocate new identifier.
 *
 * @return	New identifier
 */
stree_ident_t *stree_ident_new(void)
{
	stree_ident_t *ident;

	ident = calloc(1, sizeof(stree_ident_t));
	if (ident == NULL) {
		printf("Memory allocation failed.\n");
		exit(1);
	}

	return ident;
}

/** Allocate new literal.
 *
 * @param ltc	Literal class
 * @return	New literal
 */
stree_literal_t *stree_literal_new(literal_class_t ltc)
{
	stree_literal_t *literal;

	literal = calloc(1, sizeof(stree_literal_t));
	if (literal == NULL) {
		printf("Memory allocation failed.\n");
		exit(1);
	}

	literal->ltc = ltc;
	return literal;
}

/** Allocate new @c self reference.
 *
 * @return	New @c self reference
 */
stree_self_ref_t *stree_self_ref_new(void)
{
	stree_self_ref_t *self_ref;

	self_ref = calloc(1, sizeof(stree_self_ref_t));
	if (self_ref == NULL) {
		printf("Memory allocation failed.\n");
		exit(1);
	}

	return self_ref;
}

/** Allocate new type expression
 *
 * @return	New type expression
 */
stree_texpr_t *stree_texpr_new(texpr_class_t tc)
{
	stree_texpr_t *texpr;

	texpr = calloc(1, sizeof(stree_texpr_t));
	if (texpr == NULL) {
		printf("Memory allocation failed.\n");
		exit(1);
	}

	texpr->tc = tc;
	return texpr;
}

/** Allocate new type access operation.
 *
 * @return	New type access operation
 */
stree_taccess_t *stree_taccess_new(void)
{
	stree_taccess_t *taccess;

	taccess = calloc(1, sizeof(stree_taccess_t));
	if (taccess == NULL) {
		printf("Memory allocation failed.\n");
		exit(1);
	}

	return taccess;
}

/** Allocate new type application operation.
 *
 * @return	New type application operation
 */
stree_tapply_t *stree_tapply_new(void)
{
	stree_tapply_t *tapply;

	tapply = calloc(1, sizeof(stree_tapply_t));
	if (tapply == NULL) {
		printf("Memory allocation failed.\n");
		exit(1);
	}

	return tapply;
}

/** Allocate new type indexing operation.
 *
 * @return	New type indexing operation
 */
stree_tindex_t *stree_tindex_new(void)
{
	stree_tindex_t *tindex;

	tindex = calloc(1, sizeof(stree_tindex_t));
	if (tindex == NULL) {
		printf("Memory allocation failed.\n");
		exit(1);
	}

	return tindex;
}

/** Allocate new type literal.
 *
 * @return	New type literal
 */
stree_tliteral_t *stree_tliteral_new(tliteral_class_t tlc)
{
	stree_tliteral_t *tliteral;

	tliteral = calloc(1, sizeof(stree_tliteral_t));
	if (tliteral == NULL) {
		printf("Memory allocation failed.\n");
		exit(1);
	}

	tliteral->tlc = tlc;
	return tliteral;
}

/** Allocate new type name reference.
 *
 * @return	New type name reference
 */
stree_tnameref_t *stree_tnameref_new(void)
{
	stree_tnameref_t *tnameref;

	tnameref = calloc(1, sizeof(stree_tnameref_t));
	if (tnameref == NULL) {
		printf("Memory allocation failed.\n");
		exit(1);
	}

	return tnameref;
}

/** Allocate new symbol.
 *
 * @return	New symbol
 */
stree_symbol_t *stree_symbol_new(symbol_class_t sc)
{
	stree_symbol_t *symbol;

	symbol = calloc(1, sizeof(stree_symbol_t));
	if (symbol == NULL) {
		printf("Memory allocation failed.\n");
		exit(1);
	}

	symbol->sc = sc;
	list_init(&symbol->attr);

	return symbol;
}

/** Allocate new program.
 *
 * @return	New program
 */
stree_program_t *stree_program_new(void)
{
	stree_program_t *program;

	program = calloc(1, sizeof(stree_program_t));
	if (program == NULL) {
		printf("Memory allocation failed.\n");
		exit(1);
	}

	return program;
}

/** Determine if @a symbol has attribute of class @a sac.
 *
 * @param symbol	Symbol
 * @param sac		Symbol attribute class
 * @return		@c b_true if yes, @c b_false if no
 */
bool_t stree_symbol_has_attr(stree_symbol_t *symbol, symbol_attr_class_t sac)
{
	list_node_t *node;
	stree_symbol_attr_t *attr;

	node = list_first(&symbol->attr);
	while (node != NULL) {
		attr = list_node_data(node, stree_symbol_attr_t *);
		if (attr->sac == sac)
			return b_true;

		node = list_next(&symbol->attr, node);
	}

	return b_false;
}

/** Determine if argument @a arg has attribute of class @a aac.
 *
 * @param arg		Formal procedure argument
 * @param aac		Argument attribute class
 * @return		@c b_true if yes, @c b_false if no.
 */
bool_t stree_arg_has_attr(stree_proc_arg_t *arg, arg_attr_class_t aac)
{
	list_node_t *node;
	stree_arg_attr_t *attr;

	node = list_first(&arg->attr);
	while (node != NULL) {
		attr = list_node_data(node, stree_arg_attr_t *);
		if (attr->aac == aac)
			return b_true;

		node = list_next(&arg->attr, node);
	}

	return b_false;
}

/** Determine wheter @a a is derived (transitively) from @a b.
 *
 * XXX This does not work right with generics.
 *
 * @param a	Derived CSI.
 * @param b	Base CSI.
 * @return	@c b_true if @a a is equal to or directly or indirectly
 *		derived from @a b.
 */
bool_t stree_is_csi_derived_from_csi(stree_csi_t *a, stree_csi_t *b)
{
	stree_csi_t *csi;

	csi = a;
	while (csi != NULL) {
		if (csi == b)
			return b_true;

		csi = csi->base_csi;
	}

	/* We went all the way to the root and did not find b. */
	return b_false;
}

/** Determine if @a symbol is static.
 *
 * @param symbol	Symbol
 * @return		@c b_true if symbol is static, @c b_false otherwise
 */
bool_t stree_symbol_is_static(stree_symbol_t *symbol)
{
	/* Module-wide symbols are static. */
	if (symbol->outer_csi == NULL)
		return b_true;

	/* Symbols with @c static attribute are static. */
	if (stree_symbol_has_attr(symbol, sac_static))
		return b_true;

	switch (symbol->sc) {
	case sc_csi:
	case sc_deleg:
	case sc_enum:
		return b_true;
	case sc_ctor:
	case sc_fun:
	case sc_var:
	case sc_prop:
		break;
	}

	return b_false;
}

/** Search for CSI type argument of the given name.
 *
 * @param csi		CSI to look in
 * @param ident		Identifier of the type argument
 * @return		Type argument declaration or @c NULL if not found
 */
stree_targ_t *stree_csi_find_targ(stree_csi_t *csi, stree_ident_t *ident)
{
	list_node_t *targ_n;
	stree_targ_t *targ;

	targ_n = list_first(&csi->targ);
	while (targ_n != NULL) {
		targ = list_node_data(targ_n, stree_targ_t *);
		if (targ->name->sid == ident->sid)
			return targ;

		targ_n = list_next(&csi->targ, targ_n);
	}

	/* No match */
	return NULL;
}

/** Search for enum member of the given name.
 *
 * @param enum_d	Enum to look in
 * @param ident		Identifier of the enum member
 * @return		Enum member declaration or @c NULL if not found
 */
stree_embr_t *stree_enum_find_mbr(stree_enum_t *enum_d, stree_ident_t *ident)
{
	list_node_t *embr_n;
	stree_embr_t *embr;

	embr_n = list_first(&enum_d->members);
	while (embr_n != NULL) {
		embr = list_node_data(embr_n, stree_embr_t *);
		if (embr->name->sid == ident->sid)
			return embr;

		embr_n = list_next(&enum_d->members, embr_n);
	}

	/* No match */
	return NULL;
}

/** Get CSI member name.
 *
 * @param csimbr	CSI member
 * @return		Member name
 */
stree_ident_t *stree_csimbr_get_name(stree_csimbr_t *csimbr)
{
	stree_ident_t *mbr_name;

	/* Keep compiler happy. */
	mbr_name = NULL;

	switch (csimbr->cc) {
	case csimbr_csi:
		mbr_name = csimbr->u.csi->name;
		break;
	case csimbr_ctor:
		mbr_name = csimbr->u.ctor->name;
		break;
	case csimbr_deleg:
		mbr_name = csimbr->u.deleg->name;
		break;
	case csimbr_enum:
		mbr_name = csimbr->u.enum_d->name;
		break;
	case csimbr_fun:
		mbr_name = csimbr->u.fun->name;
		break;
	case csimbr_var:
		mbr_name = csimbr->u.var->name;
		break;
	case csimbr_prop:
		mbr_name = csimbr->u.prop->name;
		break;
	}

	return mbr_name;
}
