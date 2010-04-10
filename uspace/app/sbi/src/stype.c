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

/**
 * @file Implements a walk on the program that computes and checks static
 * types. 'Types' the program.
 */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "debug.h"
#include "intmap.h"
#include "list.h"
#include "mytypes.h"
#include "run_texpr.h"
#include "stree.h"
#include "strtab.h"
#include "stype_expr.h"
#include "symbol.h"
#include "tdata.h"

#include "stype.h"

static void stype_csi(stype_t *stype, stree_csi_t *csi);
static void stype_fun(stype_t *stype, stree_fun_t *fun);
static void stype_var(stype_t *stype, stree_var_t *var);
static void stype_prop(stype_t *stype, stree_prop_t *prop);

static void stype_block(stype_t *stype, stree_block_t *block);

static void stype_vdecl(stype_t *stype, stree_vdecl_t *vdecl_s);
static void stype_if(stype_t *stype, stree_if_t *if_s);
static void stype_while(stype_t *stype, stree_while_t *while_s);
static void stype_for(stype_t *stype, stree_for_t *for_s);
static void stype_raise(stype_t *stype, stree_raise_t *raise_s);
static void stype_return(stype_t *stype, stree_return_t *return_s);
static void stype_exps(stype_t *stype, stree_exps_t *exp_s, bool_t want_value);
static void stype_wef(stype_t *stype, stree_wef_t *wef_s);

/** Type module */
void stype_module(stype_t *stype, stree_module_t *module)
{
	list_node_t *mbr_n;
	stree_modm_t *mbr;

#ifdef DEBUG_TYPE_TRACE
	printf("Type module.\n");
#endif
	stype->current_csi = NULL;
	stype->proc_vr = NULL;

	mbr_n = list_first(&module->members);
	while (mbr_n != NULL) {
		mbr = list_node_data(mbr_n, stree_modm_t *);
		assert(mbr->mc == mc_csi);

		stype_csi(stype, mbr->u.csi);

		mbr_n = list_next(&module->members, mbr_n);
	}
}

/** Type CSI */
static void stype_csi(stype_t *stype, stree_csi_t *csi)
{
	list_node_t *csimbr_n;
	stree_csimbr_t *csimbr;
	stree_csi_t *prev_ctx;

#ifdef DEBUG_TYPE_TRACE
	printf("Type CSI '");
	symbol_print_fqn(csi_to_symbol(csi));
	printf("'.\n");
#endif
	prev_ctx = stype->current_csi;
	stype->current_csi = csi;

	csimbr_n = list_first(&csi->members);
	while (csimbr_n != NULL) {
		csimbr = list_node_data(csimbr_n, stree_csimbr_t *);

		switch (csimbr->cc) {
		case csimbr_csi: stype_csi(stype, csimbr->u.csi); break;
		case csimbr_fun: stype_fun(stype, csimbr->u.fun); break;
		case csimbr_var: stype_var(stype, csimbr->u.var); break;
		case csimbr_prop: stype_prop(stype, csimbr->u.prop); break;
		}

		csimbr_n = list_next(&csi->members, csimbr_n);
	}

	stype->current_csi = prev_ctx;
}

/** Type function */
static void stype_fun(stype_t *stype, stree_fun_t *fun)
{
	list_node_t *arg_n;
	stree_proc_arg_t *arg;
	stree_symbol_t *fun_sym;
	tdata_item_t *titem;

#ifdef DEBUG_TYPE_TRACE
	printf("Type function '");
	symbol_print_fqn(fun_to_symbol(fun));
	printf("'.\n");
#endif
	fun_sym = fun_to_symbol(fun);

	/*
	 * Type formal arguments.
	 * XXX Save the results.
	 */
	arg_n = list_first(&fun->args);
	while (arg_n != NULL) {
		arg = list_node_data(arg_n, stree_proc_arg_t *);

		/* XXX Because of overloaded builtin WriteLine. */
		if (arg->type == NULL) {
			arg_n = list_next(&fun->args, arg_n);
			continue;
		}

		run_texpr(stype->program, fun_sym->outer_csi, arg->type,
		    &titem);

		arg_n = list_next(&fun->args, arg_n);
	}

	/* Variadic argument */
	if (fun->varg != NULL) {
		/* Check type and verify it is an array. */
		run_texpr(stype->program, fun_sym->outer_csi, fun->varg->type,
		    &titem);

		if (titem->tic != tic_tarray && titem->tic != tic_ignore) {
			printf("Error: Packed argument is not an array.\n");
			stype_note_error(stype);
		}
	}

	/*
	 * Type function body.
	 */

	/* Builtin functions do not have a body. */
	if (fun->proc->body == NULL)
		return;

	stype->proc_vr = stype_proc_vr_new();
	stype->proc_vr->proc = fun->proc;
	list_init(&stype->proc_vr->block_vr);

	stype_block(stype, fun->proc->body);

	free(stype->proc_vr);
	stype->proc_vr = NULL;
}

/** Type member variable */
static void stype_var(stype_t *stype, stree_var_t *var)
{
	tdata_item_t *titem;

	(void) stype;
	(void) var;

	run_texpr(stype->program, stype->current_csi, var->type,
	    &titem);
	if (titem->tic == tic_ignore) {
		/* An error occured. */
		stype_note_error(stype);
		return;
	}
}

/** Type property */
static void stype_prop(stype_t *stype, stree_prop_t *prop)
{
#ifdef DEBUG_TYPE_TRACE
	printf("Type property '");
	symbol_print_fqn(prop_to_symbol(prop));
	printf("'.\n");
#endif
	stype->proc_vr = stype_proc_vr_new();
	list_init(&stype->proc_vr->block_vr);

	if (prop->getter != NULL) {
		stype->proc_vr->proc = prop->getter;
		stype_block(stype, prop->getter->body);
	}

	if (prop->setter != NULL) {
		stype->proc_vr->proc = prop->setter;
		stype_block(stype, prop->setter->body);
	}

	free(stype->proc_vr);
	stype->proc_vr = NULL;
}

/** Type statement block */
static void stype_block(stype_t *stype, stree_block_t *block)
{
	stree_stat_t *stat;
	list_node_t *stat_n;
	stype_block_vr_t *block_vr;
	list_node_t *bvr_n;

#ifdef DEBUG_TYPE_TRACE
	printf("Type block.\n");
#endif

	/* Create block visit record. */
	block_vr = stype_block_vr_new();
	intmap_init(&block_vr->vdecls);

	/* Add block visit record to the stack. */
	list_append(&stype->proc_vr->block_vr, block_vr);

	stat_n = list_first(&block->stats);
	while (stat_n != NULL) {
		stat = list_node_data(stat_n, stree_stat_t *);
		stype_stat(stype, stat, b_false);

		stat_n = list_next(&block->stats, stat_n);
	}

	/* Remove block visit record from the stack, */
	bvr_n = list_last(&stype->proc_vr->block_vr);
	assert(list_node_data(bvr_n, stype_block_vr_t *) == block_vr);
	list_remove(&stype->proc_vr->block_vr, bvr_n);
}

/** Type statement
 *
 * Types a statement. If @a want_value is @c b_true, then warning about
 * ignored expression value will be supressed for this statement (but not
 * for nested statemens). This is used in interactive mode.
 *
 * @param stype		Static typer object.
 * @param stat		Statement to type.
 * @param want_value	@c b_true to allow ignoring expression value.
 */
void stype_stat(stype_t *stype, stree_stat_t *stat, bool_t want_value)
{
#ifdef DEBUG_TYPE_TRACE
	printf("Type statement.\n");
#endif
	switch (stat->sc) {
	case st_vdecl: stype_vdecl(stype, stat->u.vdecl_s); break;
	case st_if: stype_if(stype, stat->u.if_s); break;
	case st_while: stype_while(stype, stat->u.while_s); break;
	case st_for: stype_for(stype, stat->u.for_s); break;
	case st_raise: stype_raise(stype, stat->u.raise_s); break;
	case st_return: stype_return(stype, stat->u.return_s); break;
	case st_exps: stype_exps(stype, stat->u.exp_s, want_value); break;
	case st_wef: stype_wef(stype, stat->u.wef_s); break;
	}
}

/** Type local variable declaration */
static void stype_vdecl(stype_t *stype, stree_vdecl_t *vdecl_s)
{
	stype_block_vr_t *block_vr;
	stree_vdecl_t *old_vdecl;
	tdata_item_t *titem;

#ifdef DEBUG_TYPE_TRACE
	printf("Type variable declaration statement.\n");
#endif
	block_vr = stype_get_current_block_vr(stype);
	old_vdecl = (stree_vdecl_t *) intmap_get(&block_vr->vdecls,
	    vdecl_s->name->sid);

	if (old_vdecl != NULL) {
		printf("Error: Duplicate variable declaration '%s'.\n",
		    strtab_get_str(vdecl_s->name->sid));
		stype_note_error(stype);
	}

	run_texpr(stype->program, stype->current_csi, vdecl_s->type,
	    &titem);
	if (titem->tic == tic_ignore) {
		/* An error occured. */
		stype_note_error(stype);
		return;
	}

	intmap_set(&block_vr->vdecls, vdecl_s->name->sid, vdecl_s);
}

/** Type @c if statement */
static void stype_if(stype_t *stype, stree_if_t *if_s)
{
	stree_expr_t *ccond;

#ifdef DEBUG_TYPE_TRACE
	printf("Type 'if' statement.\n");
#endif
	/* Convert condition to boolean type. */
	stype_expr(stype, if_s->cond);
	ccond = stype_convert(stype, if_s->cond, stype_boolean_titem(stype));

	/* Patch code with augmented expression. */
	if_s->cond = ccond;

	/* Type the @c if block */
	stype_block(stype, if_s->if_block);

	/* Type the @c else block */
	if (if_s->else_block != NULL)
		stype_block(stype, if_s->else_block);
}

/** Type @c while statement */
static void stype_while(stype_t *stype, stree_while_t *while_s)
{
	stree_expr_t *ccond;

#ifdef DEBUG_TYPE_TRACE
	printf("Type 'while' statement.\n");
#endif
	/* Convert condition to boolean type. */
	stype_expr(stype, while_s->cond);
	ccond = stype_convert(stype, while_s->cond,
	    stype_boolean_titem(stype));

	/* Patch code with augmented expression. */
	while_s->cond = ccond;

	/* Type the body of the loop */
	stype_block(stype, while_s->body);
}

/** Type @c for statement */
static void stype_for(stype_t *stype, stree_for_t *for_s)
{
#ifdef DEBUG_TYPE_TRACE
	printf("Type 'for' statement.\n");
#endif
	stype_block(stype, for_s->body);
}

/** Type @c raise statement */
static void stype_raise(stype_t *stype, stree_raise_t *raise_s)
{
#ifdef DEBUG_TYPE_TRACE
	printf("Type 'raise' statement.\n");
#endif
	stype_expr(stype, raise_s->expr);
}

/** Type @c return statement */
static void stype_return(stype_t *stype, stree_return_t *return_s)
{
	stree_symbol_t *outer_sym;
	stree_fun_t *fun;
	stree_prop_t *prop;

	stree_expr_t *cexpr;
	tdata_item_t *dtype;

#ifdef DEBUG_TYPE_TRACE
	printf("Type 'return' statement.\n");
#endif
	stype_expr(stype, return_s->expr);

	/* Determine the type we need to return. */

	outer_sym = stype->proc_vr->proc->outer_symbol;
	switch (outer_sym->sc) {
	case sc_fun:
		fun = symbol_to_fun(outer_sym);
		assert(fun != NULL);

		/* XXX Memoize to avoid recomputing. */
		run_texpr(stype->program, outer_sym->outer_csi, fun->rtype,
		    &dtype);
		break;
	case sc_prop:
		prop = symbol_to_prop(outer_sym);
		assert(prop != NULL);

		if (stype->proc_vr->proc != prop->getter) {
			printf("Error: Return statement in "
			    "setter.\n");
			stype_note_error(stype);
		}

		/* XXX Memoize to avoid recomputing. */
		run_texpr(stype->program, outer_sym->outer_csi, prop->type,
		    &dtype);
		break;
	default:
		assert(b_false);
	}

	/* Convert to the return type. */
	cexpr = stype_convert(stype, return_s->expr, dtype);

	/* Patch code with the augmented expression. */
	return_s->expr = cexpr;
}

/** Type expression statement */
static void stype_exps(stype_t *stype, stree_exps_t *exp_s, bool_t want_value)
{
#ifdef DEBUG_TYPE_TRACE
	printf("Type expression statement.\n");
#endif
	stype_expr(stype, exp_s->expr);

	if (want_value == b_false && exp_s->expr->titem != NULL)
		printf("Warning: Expression value ignored.\n");
}

/** Type With-Except-Finally statement */
static void stype_wef(stype_t *stype, stree_wef_t *wef_s)
{
	list_node_t *ec_n;
	stree_except_t *ec;

#ifdef DEBUG_TYPE_TRACE
	printf("Type WEF statement.\n");
#endif
	/* Type the @c with block. */
	if (wef_s->with_block != NULL)
		stype_block(stype, wef_s->with_block);

	/* Type the @c except clauses. */
	ec_n = list_first(&wef_s->except_clauses);
	while (ec_n != NULL) {
		ec = list_node_data(ec_n, stree_except_t *);
		stype_block(stype, ec->block);

		ec_n = list_next(&wef_s->except_clauses, ec_n);
	}

	/* Type the @c finally block. */
	if (wef_s->finally_block != NULL)
		stype_block(stype, wef_s->finally_block);
}

/** Convert expression of one type to another type.
 *
 * If the type of expression @a expr is not compatible with @a dtype
 * (i.e. there does not exist an implicit conversion from @a expr->type to
 * @a dtype), this function will produce an error (Cannot convert A to B).
 *
 * Otherwise it will either return the expression unmodified (if there is
 * no action to take at run time) or it will return a new expression
 * while clobbering the old one. Typically this would just attach the
 * expression as a subtree of the conversion.
 *
 * Note: No conversion that would require modifying @a expr is implemented
 * yet.
 */
stree_expr_t *stype_convert(stype_t *stype, stree_expr_t *expr,
    tdata_item_t *dest)
{
	tdata_item_t *src;

	(void) stype;
	src = expr->titem;

	if (dest == NULL) {
		printf("Error: Conversion destination is not valid.\n");
		stype_note_error(stype);
		return expr;
	}

	if (src == NULL) {
		printf("Error: Conversion source is not valid.\n");
		stype_note_error(stype);
		return expr;
	}

	if (dest->tic == tic_ignore || src->tic == tic_ignore)
		return expr;

	/*
	 * Special case: Nil to object.
	 */
	if (src->tic == tic_tprimitive && src->u.tprimitive->tpc == tpc_nil) {
		if (dest->tic == tic_tobject)
			return expr;
	}

	if (src->tic != dest->tic)
		goto failure;

	switch (src->tic) {
	case tic_tprimitive:
		/* Check if both have the same tprimitive class. */
		if (src->u.tprimitive->tpc != dest->u.tprimitive->tpc)
			goto failure;
		break;
	case tic_tobject:
		/* Check if @c src is derived from @c dest. */
		if (stree_is_csi_derived_from_csi(src->u.tobject->csi,
		    dest->u.tobject->csi) != b_true) {
			goto failure;
		}
		break;
	case tic_tarray:
		/* Compare rank and base type. */
		if (src->u.tarray->rank != dest->u.tarray->rank)
			goto failure;

		/* XXX Should we convert each element? */
		if (tdata_item_equal(src->u.tarray->base_ti,
		    dest->u.tarray->base_ti) != b_true)
			goto failure;
		break;
	case tic_tfun:
		printf("Error: Unimplemented: Converting '");
		tdata_item_print(src);
		printf("' to '");
		tdata_item_print(dest);
		printf("'.\n");
		stype_note_error(stype);
		break;
	case tic_ignore:
		assert(b_false);
	}

	return expr;

failure:
	printf("Error: Cannot convert ");
	tdata_item_print(src);
	printf(" to ");
	tdata_item_print(dest);
	printf(".\n");

	stype_note_error(stype);
	return expr;
}

/** Return a boolean type item */
tdata_item_t *stype_boolean_titem(stype_t *stype)
{
	tdata_item_t *titem;
	tdata_primitive_t *tprimitive;

	(void) stype;

	titem = tdata_item_new(tic_tprimitive);
	tprimitive = tdata_primitive_new(tpc_bool);
	titem->u.tprimitive = tprimitive;

	return titem;
}

/** Find a local variable in the current function. */
stree_vdecl_t *stype_local_vars_lookup(stype_t *stype, sid_t name)
{
	stype_proc_vr_t *proc_vr;
	stype_block_vr_t *block_vr;
	stree_vdecl_t *vdecl;
	list_node_t *node;

	proc_vr = stype->proc_vr;
	node = list_last(&proc_vr->block_vr);

	/* Walk through all block visit records. */
	while (node != NULL) {
		block_vr = list_node_data(node, stype_block_vr_t *);
		vdecl = intmap_get(&block_vr->vdecls, name);
		if (vdecl != NULL)
			return vdecl;

		node = list_prev(&proc_vr->block_vr, node);
	}

	/* No match */
	return NULL;
}

/** Find argument of the current procedure. */
stree_proc_arg_t *stype_proc_args_lookup(stype_t *stype, sid_t name)
{
	stype_proc_vr_t *proc_vr;

	stree_symbol_t *outer_sym;
	stree_fun_t *fun;
	stree_prop_t *prop;

	list_t *args;
	list_node_t *arg_node;
	stree_proc_arg_t *varg;
	stree_proc_arg_t *arg;
	stree_proc_arg_t *setter_arg;

	proc_vr = stype->proc_vr;
	outer_sym = proc_vr->proc->outer_symbol;

	setter_arg = NULL;

#ifdef DEBUG_TYPE_TRACE
	printf("Look for argument named '%s'.\n", strtab_get_str(name));
#endif

	switch (outer_sym->sc) {
	case sc_fun:
		fun = symbol_to_fun(outer_sym);
		assert(fun != NULL);
		args = &fun->args;
		varg = fun->varg;
		break;
	case sc_prop:
		prop = symbol_to_prop(outer_sym);
		assert(prop != NULL);
		args = &prop->args;
		varg = prop->varg;

		/* If we are in a setter, look also at setter argument. */
		if (prop->setter == proc_vr->proc)
			setter_arg = prop->setter_arg;
		break;
	default:
		assert(b_false);
	}

	arg_node = list_first(args);
	while (arg_node != NULL) {
		arg = list_node_data(arg_node, stree_proc_arg_t *);
		if (arg->name->sid == name) {
			/* Match */
#ifdef DEBUG_TYPE_TRACE
			printf("Found argument.\n");
#endif
			return arg;
		}

		arg_node = list_next(args, arg_node);
	}

	/* Variadic argument */
	if (varg != NULL && varg->name->sid == name) {
#ifdef DEBUG_TYPE_TRACE
		printf("Found variadic argument.\n");
#endif
		return varg;
}

	/* Setter argument */
	if (setter_arg != NULL && setter_arg->name->sid == name) {
#ifdef DEBUG_TYPE_TRACE
		printf("Found setter argument.\n");
#endif
		return setter_arg;

	}

#ifdef DEBUG_TYPE_TRACE
	printf("Not found.\n");
#endif
	/* No match */
	return NULL;
}

/** Note a static typing error that has been immediately recovered. */
void stype_note_error(stype_t *stype)
{
	stype->error = b_true;
}

/** Construct a special type item for recovery. */
tdata_item_t *stype_recovery_titem(stype_t *stype)
{
	tdata_item_t *titem;

	(void) stype;

	titem = tdata_item_new(tic_ignore);
	return titem;
}

/** Get current block visit record. */
stype_block_vr_t *stype_get_current_block_vr(stype_t *stype)
{
	list_node_t *node;

	node = list_last(&stype->proc_vr->block_vr);
	return list_node_data(node, stype_block_vr_t *);
}

stype_proc_vr_t *stype_proc_vr_new(void)
{
	stype_proc_vr_t *proc_vr;

	proc_vr = calloc(1, sizeof(stype_proc_vr_t));
	if (proc_vr == NULL) {
		printf("Memory allocation failed.\n");
		exit(1);
	}

	return proc_vr;
}

stype_block_vr_t *stype_block_vr_new(void)
{
	stype_block_vr_t *block_vr;

	block_vr = calloc(1, sizeof(stype_block_vr_t));
	if (block_vr == NULL) {
		printf("Memory allocation failed.\n");
		exit(1);
	}

	return block_vr;
}
