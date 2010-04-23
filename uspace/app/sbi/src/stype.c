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
 *
 * If a type error is encountered, stype_note_error() is called to set
 * the typing error flag.
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

static void stype_fun_sig(stype_t *stype, stree_csi_t *outer_csi,
    stree_fun_sig_t *sig, tdata_fun_sig_t **rtsig);
static void stype_fun_body(stype_t *stype, stree_fun_t *fun);
static void stype_block(stype_t *stype, stree_block_t *block);

static void stype_vdecl(stype_t *stype, stree_vdecl_t *vdecl_s);
static void stype_if(stype_t *stype, stree_if_t *if_s);
static void stype_while(stype_t *stype, stree_while_t *while_s);
static void stype_for(stype_t *stype, stree_for_t *for_s);
static void stype_raise(stype_t *stype, stree_raise_t *raise_s);
static void stype_return(stype_t *stype, stree_return_t *return_s);
static void stype_exps(stype_t *stype, stree_exps_t *exp_s, bool_t want_value);
static void stype_wef(stype_t *stype, stree_wef_t *wef_s);

static stree_expr_t *stype_convert_tprimitive(stype_t *stype,
    stree_expr_t *expr, tdata_item_t *dest);
static stree_expr_t *stype_convert_tprim_tobj(stype_t *stype,
    stree_expr_t *expr, tdata_item_t *dest);
static stree_expr_t *stype_convert_tobject(stype_t *stype, stree_expr_t *expr,
    tdata_item_t *dest);
static stree_expr_t *stype_convert_tarray(stype_t *stype, stree_expr_t *expr,
    tdata_item_t *dest);
static stree_expr_t *stype_convert_tdeleg(stype_t *stype, stree_expr_t *expr,
    tdata_item_t *dest);
static stree_expr_t *stype_convert_tfun_tdeleg(stype_t *stype,
    stree_expr_t *expr, tdata_item_t *dest);
static stree_expr_t *stype_convert_tvref(stype_t *stype, stree_expr_t *expr,
    tdata_item_t *dest);
static void stype_convert_failure(stype_t *stype, tdata_item_t *src,
    tdata_item_t *dest);

static bool_t stype_fun_sig_equal(stype_t *stype, tdata_fun_sig_t *asig,
    tdata_fun_sig_t *sdig);

/** Type module.
 *
 * If the module contains a type error, @a stype->error will be set
 * when this function returns.
 *
 * @param stype		Static typing object
 * @param module	Module to type
 */
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

/** Type CSI.
 *
 * @param stype		Static typing object
 * @param csi		CSI to type
 */
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
		case csimbr_deleg: stype_deleg(stype, csimbr->u.deleg); break;
		case csimbr_fun: stype_fun(stype, csimbr->u.fun); break;
		case csimbr_var: stype_var(stype, csimbr->u.var); break;
		case csimbr_prop: stype_prop(stype, csimbr->u.prop); break;
		}

		csimbr_n = list_next(&csi->members, csimbr_n);
	}

	stype->current_csi = prev_ctx;
}

/** Type delegate.
 *
 * @param stype		Static typing object.
 * @param deleg		Delegate to type.
 */
void stype_deleg(stype_t *stype, stree_deleg_t *deleg)
{
	stree_symbol_t *deleg_sym;
	tdata_item_t *deleg_ti;
	tdata_deleg_t *tdeleg;
	tdata_fun_sig_t *tsig;

#ifdef DEBUG_TYPE_TRACE
	printf("Type delegate '");
	symbol_print_fqn(deleg_to_symbol(deleg));
	printf("'.\n");
#endif
	if (deleg->titem == NULL) {
		deleg_ti = tdata_item_new(tic_tdeleg);
		deleg->titem = deleg_ti;
		tdeleg = tdata_deleg_new();
		deleg_ti->u.tdeleg = tdeleg;
	} else {
		deleg_ti = deleg->titem;
		assert(deleg_ti->u.tdeleg != NULL);
		tdeleg = deleg_ti->u.tdeleg;
	}

	if (tdeleg->tsig != NULL)
		return; /* Delegate has already been typed. */

	deleg_sym = deleg_to_symbol(deleg);

	/* Type function signature. Store result in deleg->titem. */
	stype_fun_sig(stype, deleg_sym->outer_csi, deleg->sig, &tsig);

	tdeleg->deleg = deleg;
	tdeleg->tsig = tsig;
}

/** Type function.
 *
 * We split typing of function header and body because at the point we
 * are typing the body of some function we may encounter function calls.
 * To type a function call we first need to type the header of the function
 * being called.
 *
 * @param stype		Static typing object.
 * @param fun		Function to type.
 */
static void stype_fun(stype_t *stype, stree_fun_t *fun)
{
#ifdef DEBUG_TYPE_TRACE
	printf("Type function '");
	symbol_print_fqn(fun_to_symbol(fun));
	printf("'.\n");
#endif
	if (fun->titem == NULL)
		stype_fun_header(stype, fun);

	stype_fun_body(stype, fun);
}

/** Type function header.
 *
 * Types the header of @a fun (but not its body).
 *
 * @param stype		Static typing object
 * @param fun		Funtction
 */
void stype_fun_header(stype_t *stype, stree_fun_t *fun)
{
	stree_symbol_t *fun_sym;
	tdata_item_t *fun_ti;
	tdata_fun_t *tfun;
	tdata_fun_sig_t *tsig;

#ifdef DEBUG_TYPE_TRACE
	printf("Type function '");
	symbol_print_fqn(fun_to_symbol(fun));
	printf("' header.\n");
#endif
	if (fun->titem != NULL)
		return; /* Function header has already been typed. */

	fun_sym = fun_to_symbol(fun);

	/* Type function signature. */
	stype_fun_sig(stype, fun_sym->outer_csi, fun->sig, &tsig);

	fun_ti = tdata_item_new(tic_tfun);
	tfun = tdata_fun_new();
	fun_ti->u.tfun = tfun;
	tfun->tsig = tsig;

	fun->titem = fun_ti;
}

/** Type function signature.
 *
 * Types the function signature @a sig.
 *
 * @param stype		Static typing object
 * @param outer_csi	CSI within which the signature is defined.
 * @param sig		Function signature
 */
static void stype_fun_sig(stype_t *stype, stree_csi_t *outer_csi,
    stree_fun_sig_t *sig, tdata_fun_sig_t **rtsig)
{
	list_node_t *arg_n;
	stree_proc_arg_t *arg;
	tdata_item_t *titem;
	tdata_fun_sig_t *tsig;

#ifdef DEBUG_TYPE_TRACE
	printf("Type function signature.\n");
#endif
	tsig = tdata_fun_sig_new();

	list_init(&tsig->arg_ti);

	/*
	 * Type formal arguments.
	 */
	arg_n = list_first(&sig->args);
	while (arg_n != NULL) {
		arg = list_node_data(arg_n, stree_proc_arg_t *);

		/* XXX Because of overloaded builtin WriteLine. */
		if (arg->type == NULL) {
			list_append(&tsig->arg_ti, NULL);
			arg_n = list_next(&sig->args, arg_n);
			continue;
		}

		run_texpr(stype->program, outer_csi, arg->type, &titem);
		list_append(&tsig->arg_ti, titem);

		arg_n = list_next(&sig->args, arg_n);
	}

	/* Variadic argument */
	if (sig->varg != NULL) {
		/* Check type and verify it is an array. */
		run_texpr(stype->program, outer_csi, sig->varg->type, &titem);
		tsig->varg_ti = titem;

		if (titem->tic != tic_tarray && titem->tic != tic_ignore) {
			printf("Error: Packed argument is not an array.\n");
			stype_note_error(stype);
		}
	}

	/* Return type */
	if (sig->rtype != NULL) {
		run_texpr(stype->program, outer_csi, sig->rtype, &titem);
		tsig->rtype = titem;
	}

	*rtsig = tsig;
}

/** Type function body.
 *
 * Types the body of function @a fun (if it has one).
 *
 * @param stype		Static typing object
 * @param fun		Funtction
 */
static void stype_fun_body(stype_t *stype, stree_fun_t *fun)
{
#ifdef DEBUG_TYPE_TRACE
	printf("Type function '");
	symbol_print_fqn(fun_to_symbol(fun));
	printf("' body.\n");
#endif
	assert(stype->proc_vr == NULL);

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

/** Type member variable.
 *
 * @param stype		Static typing object
 * @param var		Member variable
 */
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

/** Type property.
 *
 * @param stype		Static typing object
 * @param prop		Property
 */
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

/** Type statement block.
 *
 * @param stype		Static typing object
 * @param block		Statement block
 */
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
 * @param stype		Static typing object
 * @param stat		Statement to type
 * @param want_value	@c b_true to allow ignoring expression value
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

/** Type local variable declaration statement.
 *
 * @param stype		Static typing object
 * @param vdecl_s	Variable delcaration statement
 */
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

/** Type @c if statement.
 *
 * @param stype		Static typing object
 * @param if_s		@c if statement
 */
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

/** Type @c while statement
 *
 * @param stype		Static typing object
 * @param while_s	@c while statement
 */
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

/** Type @c for statement.
 *
 * @param stype		Static typing object
 * @param for_s		@c for statement
 */
static void stype_for(stype_t *stype, stree_for_t *for_s)
{
#ifdef DEBUG_TYPE_TRACE
	printf("Type 'for' statement.\n");
#endif
	stype_block(stype, for_s->body);
}

/** Type @c raise statement.
 *
 * @param stype		Static typing object
 * @param raise_s	@c raise statement
 */
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
		run_texpr(stype->program, outer_sym->outer_csi,
		    fun->sig->rtype, &dtype);
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

/** Type expression statement.
 *
 * @param stype		Static typing object
 * @param exp_s		Expression statement
 */
static void stype_exps(stype_t *stype, stree_exps_t *exp_s, bool_t want_value)
{
#ifdef DEBUG_TYPE_TRACE
	printf("Type expression statement.\n");
#endif
	stype_expr(stype, exp_s->expr);

	if (want_value == b_false && exp_s->expr->titem != NULL)
		printf("Warning: Expression value ignored.\n");
}

/** Type with-except-finally statement.
 *
 * @param stype		Static typing object
 * @param wef_s		With-except-finally statement
 */
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
 *
 * @param stype		Static typing object
 * @param expr		Expression
 * @param dest		Destination type
 */
stree_expr_t *stype_convert(stype_t *stype, stree_expr_t *expr,
    tdata_item_t *dest)
{
	tdata_item_t *src;

	src = expr->titem;

#ifdef DEBUG_TYPE_TRACE
	printf("Convert '");
	tdata_item_print(src);
	printf("' to '");
	tdata_item_print(dest);
	printf("'.\n");
#endif

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

	if (src->tic == tic_tprimitive && dest->tic == tic_tobject) {
		return stype_convert_tprim_tobj(stype, expr, dest);
	}

	if (src->tic == tic_tfun && dest->tic == tic_tdeleg) {
		return stype_convert_tfun_tdeleg(stype, expr, dest);
	}

	if (src->tic != dest->tic) {
		stype_convert_failure(stype, src, dest);
		return expr;
	}

	switch (src->tic) {
	case tic_tprimitive:
		expr = stype_convert_tprimitive(stype, expr, dest);
		break;
	case tic_tobject:
		expr = stype_convert_tobject(stype, expr, dest);
		break;
	case tic_tarray:
		expr = stype_convert_tarray(stype, expr, dest);
		break;
	case tic_tdeleg:
		expr = stype_convert_tdeleg(stype, expr, dest);
		break;
	case tic_tfun:
		assert(b_false);
	case tic_tvref:
		expr = stype_convert_tvref(stype, expr, dest);
		break;
	case tic_ignore:
		assert(b_false);
	}

	return expr;
}

/** Convert expression of primitive type to primitive type.
 *
 * @param stype		Static typing object
 * @param expr		Expression
 * @param dest		Destination type
 */
static stree_expr_t *stype_convert_tprimitive(stype_t *stype,
    stree_expr_t *expr, tdata_item_t *dest)
{
	tdata_item_t *src;

#ifdef DEBUG_TYPE_TRACE
	printf("Convert primitive type.\n");
#endif
	src = expr->titem;
	assert(src->tic == tic_tprimitive);
	assert(dest->tic == tic_tprimitive);

	/* Check if both have the same tprimitive class. */
	if (src->u.tprimitive->tpc != dest->u.tprimitive->tpc)
		stype_convert_failure(stype, src, dest);

	return expr;
}

/** Convert expression of primitive type to object type.
 *
 * This function implements autoboxing. It modified the code.
 *
 * @param stype		Static typing object
 * @param expr		Expression
 * @param dest		Destination type
 */
static stree_expr_t *stype_convert_tprim_tobj(stype_t *stype,
    stree_expr_t *expr, tdata_item_t *dest)
{
	tdata_item_t *src;
	builtin_t *bi;
	stree_symbol_t *csi_sym;
	stree_symbol_t *bp_sym;
	stree_box_t *box;
	stree_expr_t *bexpr;

#ifdef DEBUG_TYPE_TRACE
	printf("Convert primitive type to object.\n");
#endif
	src = expr->titem;
	assert(src->tic == tic_tprimitive);
	assert(dest->tic == tic_tobject);

	bi = stype->program->builtin;
	csi_sym = csi_to_symbol(dest->u.tobject->csi);

	switch (src->u.tprimitive->tpc) {
	case tpc_bool: bp_sym = bi->boxed_bool; break;
	case tpc_char: bp_sym = bi->boxed_char; break;
	case tpc_int: bp_sym = bi->boxed_int; break;
	case tpc_nil: assert(b_false);
	case tpc_string: bp_sym = bi->boxed_string; break;
	case tpc_resource:
		stype_convert_failure(stype, src, dest);
		return expr;
	}

	/* Target type must be boxed @a src or Object */
	if (csi_sym != bp_sym && csi_sym != bi->gf_class)
		stype_convert_failure(stype, src, dest);

	/* Patch the code to box the primitive value */
	box = stree_box_new();
	box->arg = expr;
	bexpr = stree_expr_new(ec_box);
	bexpr->u.box = box;

	/* No action needed to optionally convert boxed type to Object */

	return bexpr;
}

/** Convert expression of object type to object type.
 *
 * @param stype		Static typing object
 * @param expr		Expression
 * @param dest		Destination type
 */
static stree_expr_t *stype_convert_tobject(stype_t *stype, stree_expr_t *expr,
    tdata_item_t *dest)
{
	tdata_item_t *src;
	tdata_item_t *cur;
	stree_csi_t *cur_csi;
	tdata_tvv_t *tvv;
	tdata_item_t *b_ti, *bs_ti;

#ifdef DEBUG_TYPE_TRACE
	printf("Convert object type.\n");
#endif
	list_node_t *ca_n, *da_n;
	tdata_item_t *carg, *darg;

	src = expr->titem;
	assert(src->tic == tic_tobject);
	assert(dest->tic == tic_tobject);

	cur = src;

	while (cur->u.tobject->csi != dest->u.tobject->csi) {

		cur_csi = cur->u.tobject->csi;
		stype_titem_to_tvv(stype, cur, &tvv);

		if (cur_csi->base_csi_ref != NULL) {
			run_texpr(stype->program, cur_csi, cur_csi->base_csi_ref, &b_ti);
			if (b_ti->tic == tic_ignore) {
				/* An error occured. */
				stype_note_error(stype);
				return expr;
			}

			tdata_item_subst(b_ti, tvv, &bs_ti);
			cur = bs_ti;
			assert(cur->tic == tic_tobject);

		} else if (cur_csi->base_csi != NULL) {
			/* No explicit reference. Use grandfather class. */
			cur = tdata_item_new(tic_tobject);
			cur->u.tobject = tdata_object_new();
			cur->u.tobject->csi = cur_csi->base_csi;
			cur->u.tobject->static_ref = b_false;

			list_init(&cur->u.tobject->targs);
		} else {
			/* No match */
			stype_convert_failure(stype, src, dest);
			return expr;
		}
	}

	/* Verify that type arguments match */
	ca_n = list_first(&cur->u.tobject->targs);
	da_n = list_first(&dest->u.tobject->targs);

	while (ca_n != NULL && da_n != NULL) {
		carg = list_node_data(ca_n, tdata_item_t *);
		darg = list_node_data(da_n, tdata_item_t *);

		if (tdata_item_equal(carg, darg) != b_true) {
			/* Diferent argument type */
			stype_convert_failure(stype, src, dest);
			printf("Different argument type '");
			tdata_item_print(carg);
			printf("' vs. '");
			tdata_item_print(darg);
			printf("'.\n");
			return expr;
		}

		ca_n = list_next(&cur->u.tobject->targs, ca_n);
		da_n = list_next(&dest->u.tobject->targs, da_n);
	}

	if (ca_n != NULL || da_n != NULL) {
		/* Diferent number of arguments */
		stype_convert_failure(stype, src, dest);
		printf("Different number of arguments.\n");
		return expr;
	}

	return expr;
}

/** Convert expression of array type to array type.
 *
 * @param stype		Static typing object
 * @param expr		Expression
 * @param dest		Destination type
 */
static stree_expr_t *stype_convert_tarray(stype_t *stype, stree_expr_t *expr,
    tdata_item_t *dest)
{
	tdata_item_t *src;

#ifdef DEBUG_TYPE_TRACE
	printf("Convert array type.\n");
#endif
	src = expr->titem;
	assert(src->tic == tic_tarray);
	assert(dest->tic == tic_tarray);

	/* Compare rank and base type. */
	if (src->u.tarray->rank != dest->u.tarray->rank) {
		stype_convert_failure(stype, src, dest);
		return expr;
	}

	/* XXX Should we convert each element? */
	if (tdata_item_equal(src->u.tarray->base_ti,
	    dest->u.tarray->base_ti) != b_true) {
		stype_convert_failure(stype, src, dest);
	}

	return expr;
}

/** Convert expression of delegate type to delegate type.
 *
 * @param stype		Static typing object
 * @param expr		Expression
 * @param dest		Destination type
 */
static stree_expr_t *stype_convert_tdeleg(stype_t *stype, stree_expr_t *expr,
    tdata_item_t *dest)
{
	tdata_item_t *src;
	tdata_deleg_t *sdeleg, *ddeleg;

#ifdef DEBUG_TYPE_TRACE
	printf("Convert delegate type.\n");
#endif
	src = expr->titem;
	assert(src->tic == tic_tdeleg);
	assert(dest->tic == tic_tdeleg);

	sdeleg = src->u.tdeleg;
	ddeleg = dest->u.tdeleg;

	/*
	 * XXX We need to redesign handling of generic types to handle
	 * delegates in generic CSIs properly.
	 */

	/* Destination should never be anonymous delegate. */
	assert(ddeleg->deleg != NULL);

	/* Both must be the same delegate. */
	if (sdeleg->deleg != ddeleg->deleg) {
		stype_convert_failure(stype, src, dest);
		return expr;
	}

	return expr;
}

/** Convert expression of function type to delegate type.
 *
 * @param stype		Static typing object
 * @param expr		Expression
 * @param dest		Destination type
 */
static stree_expr_t *stype_convert_tfun_tdeleg(stype_t *stype,
    stree_expr_t *expr, tdata_item_t *dest)
{
	tdata_item_t *src;
	tdata_fun_t *sfun;
	tdata_deleg_t *ddeleg;
	tdata_fun_sig_t *ssig, *dsig;

#ifdef DEBUG_TYPE_TRACE
	printf("Convert delegate type.\n");
#endif
	src = expr->titem;
	assert(src->tic == tic_tfun);
	assert(dest->tic == tic_tdeleg);

	sfun = src->u.tfun;
	ddeleg = dest->u.tdeleg;

	ssig = sfun->tsig;
	assert(ssig != NULL);
	dsig = stype_deleg_get_sig(stype, ddeleg);
	assert(dsig != NULL);

	/* Signature type must match. */

	if (!stype_fun_sig_equal(stype, ssig, dsig)) {
		stype_convert_failure(stype, src, dest);
		return expr;
	}

	/*
	 * XXX We should also compare attributes. Either the
	 * tdeleg should be extended or we should get them
	 * from stree_deleg.
	 */

	return expr;
}


/** Convert expression of variable type to variable type.
 *
 * @param stype		Static typing object
 * @param expr		Expression
 * @param dest		Destination type
 */
static stree_expr_t *stype_convert_tvref(stype_t *stype, stree_expr_t *expr,
    tdata_item_t *dest)
{
	tdata_item_t *src;

#ifdef DEBUG_TYPE_TRACE
	printf("Convert variable type.\n");
#endif
	src = expr->titem;

	/* Currently only allow if both types are the same. */
	if (src->u.tvref->targ != dest->u.tvref->targ) {
		stype_convert_failure(stype, src, dest);
		return expr;
	}

	return expr;
}

/** Display conversion error message and note error.
 *
 * @param stype		Static typing object
 * @param src		Original type
 * @param dest		Destination type
 */
static void stype_convert_failure(stype_t *stype, tdata_item_t *src,
    tdata_item_t *dest)
{
	printf("Error: Cannot convert ");
	tdata_item_print(src);
	printf(" to ");
	tdata_item_print(dest);
	printf(".\n");

	stype_note_error(stype);
}

/** Determine if two type signatures are equal.
 *
 * XXX This does not compare the attributes, which are missing from
 * @c tdata_fun_sig_t.
 *
 * @param stype		Static typing object
 * @param asig		First function signature type
 * @param bsig		Second function signature type
 */
static bool_t stype_fun_sig_equal(stype_t *stype, tdata_fun_sig_t *asig,
    tdata_fun_sig_t *bsig)
{
	list_node_t *aarg_n, *barg_n;
	tdata_item_t *aarg_ti, *barg_ti;

	(void) stype;

	/* Compare types of arguments */
	aarg_n = list_first(&asig->arg_ti);
	barg_n = list_first(&bsig->arg_ti);

	while (aarg_n != NULL && barg_n != NULL) {
		aarg_ti = list_node_data(aarg_n, tdata_item_t *);
		barg_ti = list_node_data(barg_n, tdata_item_t *);

		if (!tdata_item_equal(aarg_ti, barg_ti))
			return b_false;

		aarg_n = list_next(&asig->arg_ti, aarg_n);
		barg_n = list_next(&bsig->arg_ti, barg_n);
	}

	if (aarg_n != NULL || barg_n != NULL)
		return b_false;

	/* Compare variadic argument */

	if (asig->varg_ti != NULL || bsig->varg_ti != NULL) {
		if (asig->varg_ti == NULL ||
		    bsig->varg_ti == NULL) {
			return b_false;
		}

		if (!tdata_item_equal(asig->varg_ti, bsig->varg_ti)) {
			return b_false;
		}
	}

	/* Compare return type */
	if (!tdata_item_equal(asig->rtype, bsig->rtype))
		return b_false;

	return b_true;
}

/** Get function signature from delegate.
 *
 * Function signature can be missing if the delegate type is incomplete.
 * This is used to break circular dependency when typing delegates.
 * If this happens, we type the delegate, which gives us the signature.
 */
tdata_fun_sig_t *stype_deleg_get_sig(stype_t *stype, tdata_deleg_t *tdeleg)
{
	if (tdeleg->tsig == NULL)
		stype_deleg(stype, tdeleg->deleg);

	/* Now we should have a signature. */
	assert(tdeleg->tsig != NULL);
	return tdeleg->tsig;
}

/** Convert tic_tobject type item to TVV,
 *
 * We split generic type application into two steps. In the first step
 * we match argument names of @a ti->csi to argument values in @a ti
 * to produce a TVV (name to value map for type arguments). That is the
 * purpose of this function.
 *
 * In the second step we substitute variables in another type item
 * with their values using the TVV. This is performed by tdata_item_subst().
 *
 * @param stype		Static typing object.
 * @param ti		Type item of class tic_tobject.
 * @param rtvv		Place to store pointer to new TVV.
 */
void stype_titem_to_tvv(stype_t *stype, tdata_item_t *ti, tdata_tvv_t **rtvv)
{
	tdata_tvv_t *tvv;
	stree_csi_t *csi;

	list_node_t *formal_n;
	list_node_t *real_n;

	stree_targ_t *formal_arg;
	tdata_item_t *real_arg;

	assert(ti->tic == tic_tobject);

	tvv = tdata_tvv_new();
	intmap_init(&tvv->tvv);

	csi = ti->u.tobject->csi;
	formal_n = list_first(&csi->targ);
	real_n = list_first(&ti->u.tobject->targs);

	while (formal_n != NULL && real_n != NULL) {
		formal_arg = list_node_data(formal_n, stree_targ_t *);
		real_arg = list_node_data(real_n, tdata_item_t *);

		/* Store argument value into valuation. */
		tdata_tvv_set_val(tvv, formal_arg->name->sid, real_arg);

		formal_n = list_next(&csi->targ, formal_n);
		real_n = list_next(&ti->u.tobject->targs, real_n);
	}

	if (formal_n != NULL || real_n != NULL) {
		printf("Error: Incorrect number of type arguments.\n");
		stype_note_error(stype);

		/* Fill missing arguments with recovery type items. */
		while (formal_n != NULL) {
			formal_arg = list_node_data(formal_n, stree_targ_t *);
			/* Store recovery value into valuation. */
			tdata_tvv_set_val(tvv, formal_arg->name->sid,
			    stype_recovery_titem(stype));

			formal_n = list_next(&csi->targ, formal_n);
		}
	}

	*rtvv = tvv;
}

/** Return a boolean type item.
 *
 * @param stype		Static typing object
 * @return		New boolean type item.
 */
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

/** Find a local variable in the current function.
 *
 * @param stype		Static typing object
 * @param name		Name of variable (SID).
 * @return		Pointer to variable declaration or @c NULL if not
 *			found.
 */
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

/** Find argument of the current procedure.
 *
 * @param stype		Static typing object
 * @param name		Name of argument (SID).
 * @return		Pointer to argument declaration or @c NULL if not
 *			found.
 */
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
		args = &fun->sig->args;
		varg = fun->sig->varg;
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

/** Note a static typing error that has been immediately recovered.
 *
 * @param stype		Static typing object
 */
void stype_note_error(stype_t *stype)
{
	stype->error = b_true;
}

/** Construct a special type item for recovery.
 *
 * The recovery item is propagated towards the expression root and causes
 * any further typing errors in the expression to be supressed.
 *
 * @param stype		Static typing object
 */
tdata_item_t *stype_recovery_titem(stype_t *stype)
{
	tdata_item_t *titem;

	(void) stype;

	titem = tdata_item_new(tic_ignore);
	return titem;
}

/** Get current block visit record.
 *
 * @param stype		Static typing object
 */
stype_block_vr_t *stype_get_current_block_vr(stype_t *stype)
{
	list_node_t *node;

	node = list_last(&stype->proc_vr->block_vr);
	return list_node_data(node, stype_block_vr_t *);
}

/** Allocate new procedure visit record.
 *
 * @return	New procedure VR
 */
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

/** Allocate new block visit record.
 *
 * @return	New block VR
 */
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
