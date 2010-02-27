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

/** @file Stree (syntax tree) intermediate representation. */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "list.h"
#include "mytypes.h"

#include "stree.h"

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
	csi->base_csi_ref = NULL;
	list_init(&csi->members);
	return csi;
}

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

stree_fun_arg_t *stree_fun_arg_new(void)
{
	stree_fun_arg_t *fun_arg;

	fun_arg = calloc(1, sizeof(stree_fun_arg_t));
	if (fun_arg == NULL) {
		printf("Memory allocation failed.\n");
		exit(1);
	}

	return fun_arg;
}

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

stree_tliteral_t *stree_tliteral_new(void)
{
	stree_tliteral_t *tliteral;

	tliteral = calloc(1, sizeof(stree_tliteral_t));
	if (tliteral == NULL) {
		printf("Memory allocation failed.\n");
		exit(1);
	}

	return tliteral;
}

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

stree_symbol_t *stree_symbol_new(symbol_class_t sc)
{
	stree_symbol_t *symbol;

	symbol = calloc(1, sizeof(stree_symbol_t));
	if (symbol == NULL) {
		printf("Memory allocation failed.\n");
		exit(1);
	}

	symbol->sc = sc;
	return symbol;
}

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
