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

/** @file Runner (executes the code). */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "bigint.h"
#include "builtin.h"
#include "cspan.h"
#include "debug.h"
#include "intmap.h"
#include "list.h"
#include "mytypes.h"
#include "rdata.h"
#include "run_expr.h"
#include "run_texpr.h"
#include "stree.h"
#include "strtab.h"
#include "symbol.h"
#include "tdata.h"

#include "run.h"

static void run_block(run_t *run, stree_block_t *block);
static void run_exps(run_t *run, stree_exps_t *exps, rdata_item_t **res);
static void run_vdecl(run_t *run, stree_vdecl_t *vdecl);
static void run_if(run_t *run, stree_if_t *if_s);
static void run_switch(run_t *run, stree_switch_t *switch_s);
static void run_while(run_t *run, stree_while_t *while_s);
static void run_raise(run_t *run, stree_raise_t *raise_s);
static void run_break(run_t *run, stree_break_t *break_s);
static void run_return(run_t *run, stree_return_t *return_s);
static void run_wef(run_t *run, stree_wef_t *wef_s);

static bool_t run_exc_match(run_t *run, stree_except_t *except_c);
static stree_csi_t *run_exc_payload_get_csi(run_t *run);

static rdata_var_t *run_aprop_get_tpos(run_t *run, rdata_address_t *aprop);

static void run_aprop_read(run_t *run, rdata_addr_prop_t *addr_prop,
    rdata_item_t **ritem);
static void run_aprop_write(run_t *run, rdata_addr_prop_t *addr_prop,
    rdata_value_t *value);

static void run_var_new_tprimitive(run_t *run, tdata_primitive_t *tprimitive,
    rdata_var_t **rvar);
static void run_var_new_null_ref(run_t *run, rdata_var_t **rvar);
static void run_var_new_deleg(run_t *run, rdata_var_t **rvar);
static void run_var_new_enum(run_t *run, tdata_enum_t *tenum,
    rdata_var_t **rvar);

/** Initialize runner instance.
 *
 * @param run		Runner object
 */
void run_init(run_t *run)
{
	(void) run;
}

/** Run program.
 *
 * Associates the program @a prog with the runner object and executes
 * it. If a run-time error occurs during the execution (e.g. an unhandled
 * exception), @a run->error will be set to @c b_true when this function
 * returns.
 *
 * @param run		Runner object
 * @param prog		Program to run
 */
void run_program(run_t *run, stree_program_t *prog)
{
	stree_symbol_t *main_fun_sym;
	stree_fun_t *main_fun;
	rdata_var_t *main_obj;
	stree_ident_t *fake_ident;
	list_t main_args;
	run_proc_ar_t *proc_ar;
	rdata_item_t *res;

	/* Note down link to program code. */
	run->program = prog;

	/* Initialize thread activation record. */
	run->thread_ar = run_thread_ar_new();
	list_init(&run->thread_ar->proc_ar);
	run->thread_ar->bo_mode = bm_none;

	/* Initialize global data structure. */
	run_gdata_init(run);

	/*
	 * Find entry point @c Main().
	 */
	fake_ident = stree_ident_new();
	fake_ident->sid = strtab_get_sid("Main");
	main_fun_sym = symbol_find_epoint(prog, fake_ident);
	if (main_fun_sym == NULL) {
		printf("Error: Entry point 'Main' not found.\n");
		exit(1);
	}

	main_fun = symbol_to_fun(main_fun_sym);
	assert(main_fun != NULL);

	main_obj = run_fun_sobject_find(run, main_fun);

#ifdef DEBUG_RUN_TRACE
	printf("Found function '"); symbol_print_fqn(main_fun_sym);
	printf("'.\n");
#endif

	/* Run function @c main. */
	list_init(&main_args);
	run_proc_ar_create(run, main_obj, main_fun->proc, &proc_ar);
	run_proc_ar_set_args(run, proc_ar, &main_args);
	run_proc(run, proc_ar, &res);
	run_proc_ar_destroy(run, proc_ar);

	run_exc_check_unhandled(run);
}

/** Initialize global data.
 *
 * @param run		Runner object
 */
void run_gdata_init(run_t *run)
{
	rdata_object_t *gobject;

	run->gdata = rdata_var_new(vc_object);
	gobject = rdata_object_new();
	run->gdata->u.object_v = gobject;

	gobject->class_sym = NULL;
	gobject->static_obj = sn_static;
	intmap_init(&gobject->fields);
}

/** Run procedure.
 *
 * Inserts the provided procedure AR @a proc_ar on the execution stack
 * (in the thread AR) and executes the procedure. The return value
 * of the procedure is stored to *(@a res). @c NULL is stored if the
 * procedure returns no value.
 *
 * If the procedure execution bails out due to an exception, this
 * can be determined by looking at @c bo_mode in thread AR. Also,
 * in this case @c NULL is stored into *(@a res).
 *
 * @param run		Runner object
 * @param proc_ar	Procedure activation record
 * @param res		Place to store procedure return value
 */
void run_proc(run_t *run, run_proc_ar_t *proc_ar, rdata_item_t **res)
{
	stree_proc_t *proc;
	list_node_t *node;

	proc = proc_ar->proc;

#ifdef DEBUG_RUN_TRACE
	printf("Start executing function '");
	symbol_print_fqn(proc->outer_symbol);
	printf("'.\n");
#endif
	/* Add procedure AR to the stack. */
	list_append(&run->thread_ar->proc_ar, proc_ar);

	/* Run main procedure block. */
	if (proc->body != NULL) {
		run_block(run, proc->body);
	} else {
		builtin_run_proc(run, proc);
	}

	/* Handle bailout. */
	switch (run->thread_ar->bo_mode) {
	case bm_stat:
		/* Break bailout was not caught. */
		assert(b_false);
		/* Fallthrough */
	case bm_proc:
		run->thread_ar->bo_mode = bm_none;
		break;
	default:
		break;
	}

#ifdef DEBUG_RUN_TRACE
	printf("Done executing procedure '");
	symbol_print_fqn(proc->outer_symbol);
	printf("'.\n");

	run_print_fun_bt(run);
#endif
	/* Remove procedure activation record from the stack. */
	node = list_last(&run->thread_ar->proc_ar);
	assert(list_node_data(node, run_proc_ar_t *) == proc_ar);
	list_remove(&run->thread_ar->proc_ar, node);

	/* Procedure should not return an address. */
	assert(proc_ar->retval == NULL || proc_ar->retval->ic == ic_value);
	*res = proc_ar->retval;
}

/** Run code block.
 *
 * @param run		Runner object
 * @param block		Block to run
 */
static void run_block(run_t *run, stree_block_t *block)
{
	run_proc_ar_t *proc_ar;
	run_block_ar_t *block_ar;
	list_node_t *node;
	stree_stat_t *stat;

#ifdef DEBUG_RUN_TRACE
	printf("Executing one code block.\n");
#endif

	/* Create block activation record. */
	block_ar = run_block_ar_new();
	intmap_init(&block_ar->vars);

	/* Add block activation record to the stack. */
	proc_ar = run_get_current_proc_ar(run);
	list_append(&proc_ar->block_ar, block_ar);

	node = list_first(&block->stats);
	while (node != NULL) {
		stat = list_node_data(node, stree_stat_t *);
		run_stat(run, stat, NULL);

		if (run->thread_ar->bo_mode != bm_none)
			break;

		node = list_next(&block->stats, node);
	}

#ifdef DEBUG_RUN_TRACE
	printf("Done executing code block.\n");
#endif

	/* Remove block activation record from the stack. */
	node = list_last(&proc_ar->block_ar);
	assert(list_node_data(node, run_block_ar_t *) == block_ar);
	list_remove(&proc_ar->block_ar, node);

	/* Deallocate block activation record. */
	run_block_ar_destroy(run, block_ar);
}

/** Run statement.
 *
 * Executes a statement. If @a res is not NULL and the statement is an
 * expression statement with a value, the value item will be stored to
 * @a res.
 *
 * @param run	Runner object
 * @param stat	Statement to run
 * @param res	Place to store exps result or NULL if not interested
 */
void run_stat(run_t *run, stree_stat_t *stat, rdata_item_t **res)
{
#ifdef DEBUG_RUN_TRACE
	printf("Executing one statement %p.\n", stat);
#endif

	if (res != NULL)
		*res = NULL;

	switch (stat->sc) {
	case st_exps:
		run_exps(run, stat->u.exp_s, res);
		break;
	case st_vdecl:
		run_vdecl(run, stat->u.vdecl_s);
		break;
	case st_if:
		run_if(run, stat->u.if_s);
		break;
	case st_switch:
		run_switch(run, stat->u.switch_s);
		break;
	case st_while:
		run_while(run, stat->u.while_s);
		break;
	case st_raise:
		run_raise(run, stat->u.raise_s);
		break;
	case st_break:
		run_break(run, stat->u.break_s);
		break;
	case st_return:
		run_return(run, stat->u.return_s);
		break;
	case st_wef:
		run_wef(run, stat->u.wef_s);
		break;
	case st_for:
		printf("Ignoring unimplemented statement type %d.\n", stat->sc);
		break;
	}
}

/** Run expression statement.
 *
 * Executes an expression statement. If @a res is not NULL then the value
 * of the expression (or NULL if it has no value) will be stored to @a res.
 *
 * @param run	Runner object
 * @param exps	Expression statement to run
 * @param res	Place to store exps result or NULL if not interested
 */
static void run_exps(run_t *run, stree_exps_t *exps, rdata_item_t **res)
{
	rdata_item_t *rexpr;

#ifdef DEBUG_RUN_TRACE
	printf("Executing expression statement.\n");
#endif
	run_expr(run, exps->expr, &rexpr);

	/*
	 * If the expression has a value, the caller should have asked for it.
	 */
	assert(res != NULL || rexpr == NULL);

	if (res != NULL)
		*res = rexpr;
}

/** Run variable declaration statement.
 *
 * @param run	Runner object
 * @param vdecl	Variable declaration statement to run
 */
static void run_vdecl(run_t *run, stree_vdecl_t *vdecl)
{
	run_block_ar_t *block_ar;
	rdata_var_t *var, *old_var;

#ifdef DEBUG_RUN_TRACE
	printf("Executing variable declaration statement.\n");
#endif
	/* Create variable and initialize with default value. */
	run_var_new(run, vdecl->titem, &var);

	block_ar = run_get_current_block_ar(run);
	old_var = (rdata_var_t *) intmap_get(&block_ar->vars, vdecl->name->sid);

	if (old_var != NULL) {
		printf("Error: Duplicate variable '%s'\n",
		    strtab_get_str(vdecl->name->sid));
		exit(1);
	}

	intmap_set(&block_ar->vars, vdecl->name->sid, var);

#ifdef DEBUG_RUN_TRACE
	printf("Declared variable '%s'\n", strtab_get_str(vdecl->name->sid));
#endif
}

/** Run @c if statement.
 *
 * @param run	Runner object
 * @param if_s	If statement to run
 */
static void run_if(run_t *run, stree_if_t *if_s)
{
	rdata_item_t *rcond;
	list_node_t *ifc_node;
	stree_if_clause_t *ifc;
	bool_t rcond_b, clause_fired;

#ifdef DEBUG_RUN_TRACE
	printf("Executing if statement.\n");
#endif
	clause_fired = b_false;
	ifc_node = list_first(&if_s->if_clauses);

	/* Walk through all if/elif clauses and see if they fire. */

	while (ifc_node != NULL) {
		/* Get if/elif clause */
		ifc = list_node_data(ifc_node, stree_if_clause_t *);

		run_expr(run, ifc->cond, &rcond);
		if (run_is_bo(run))
			return;

		rcond_b = run_item_boolean_value(run, rcond);
		rdata_item_destroy(rcond);

		if (rcond_b == b_true) {
#ifdef DEBUG_RUN_TRACE
			printf("Taking non-default path.\n");
#endif
			run_block(run, ifc->block);
			clause_fired = b_true;
			break;
		}

		ifc_node = list_next(&if_s->if_clauses, ifc_node);
	}

	/* If no if/elif clause fired, invoke the else clause. */
	if (clause_fired == b_false && if_s->else_block != NULL) {
#ifdef DEBUG_RUN_TRACE
		printf("Taking default path.\n");
#endif
		run_block(run, if_s->else_block);
	}

#ifdef DEBUG_RUN_TRACE
	printf("If statement terminated.\n");
#endif
}

/** Run @c switch statement.
 *
 * @param run		Runner object
 * @param switch_s	Switch statement to run
 */
static void run_switch(run_t *run, stree_switch_t *switch_s)
{
	rdata_item_t *rsexpr, *rsexpr_vi;
	rdata_item_t *rwexpr, *rwexpr_vi;
	list_node_t *whenc_node;
	stree_when_t *whenc;
	list_node_t *expr_node;
	stree_expr_t *expr;
	bool_t clause_fired;
	bool_t equal;

#ifdef DEBUG_RUN_TRACE
	printf("Executing switch statement.\n");
#endif
	rsexpr_vi = NULL;

	/* Evaluate switch expression */
	run_expr(run, switch_s->expr, &rsexpr);
	if (run_is_bo(run))
		goto cleanup;

	/* Convert to value item */
	run_cvt_value_item(run, rsexpr, &rsexpr_vi);
	rdata_item_destroy(rsexpr);
	if (run_is_bo(run))
		goto cleanup;

	clause_fired = b_false;
	whenc_node = list_first(&switch_s->when_clauses);

	/* Walk through all when clauses and see if they fire. */

	while (whenc_node != NULL) {
		/* Get when clause */
		whenc = list_node_data(whenc_node, stree_when_t *);

		expr_node = list_first(&whenc->exprs);

		/* Walk through all expressions in the when clause */
		while (expr_node != NULL) {
			/* Get expression */
			expr = list_node_data(expr_node, stree_expr_t *);

			/* Evaluate expression */
			run_expr(run, expr, &rwexpr);
			if (run_is_bo(run))
				goto cleanup;

			/* Convert to value item */
			run_cvt_value_item(run, rwexpr, &rwexpr_vi);
			rdata_item_destroy(rwexpr);
			if (run_is_bo(run)) {
				rdata_item_destroy(rwexpr_vi);
				goto cleanup;
			}

			/* Check if values are equal ('==') */
			run_equal(run, rsexpr_vi->u.value,
			    rwexpr_vi->u.value, &equal);
			rdata_item_destroy(rwexpr_vi);
			if (run_is_bo(run))
				goto cleanup;

			if (equal) {
#ifdef DEBUG_RUN_TRACE
				printf("Taking non-default path.\n");
#endif
				run_block(run, whenc->block);
				clause_fired = b_true;
				break;
			}

			expr_node = list_next(&whenc->exprs, expr_node);
		}

		if (clause_fired)
			break;

		whenc_node = list_next(&switch_s->when_clauses, whenc_node);
	}

	/* If no when clause fired, invoke the else clause. */
	if (clause_fired == b_false && switch_s->else_block != NULL) {
#ifdef DEBUG_RUN_TRACE
		printf("Taking default path.\n");
#endif
		run_block(run, switch_s->else_block);
	}
cleanup:
	if (rsexpr_vi != NULL)
		rdata_item_destroy(rsexpr_vi);

#ifdef DEBUG_RUN_TRACE
	printf("Switch statement terminated.\n");
#endif
}

/** Run @c while statement.
 *
 * @param run		Runner object
 * @param while_s	While statement to run
 */
static void run_while(run_t *run, stree_while_t *while_s)
{
	rdata_item_t *rcond;

#ifdef DEBUG_RUN_TRACE
	printf("Executing while statement.\n");
#endif
	run_expr(run, while_s->cond, &rcond);
	if (run_is_bo(run))
		return;

	while (run_item_boolean_value(run, rcond) == b_true) {
		rdata_item_destroy(rcond);
		run_block(run, while_s->body);
		run_expr(run, while_s->cond, &rcond);
		if (run_is_bo(run))
			break;
	}

	if (rcond != NULL)
		rdata_item_destroy(rcond);

	if (run->thread_ar->bo_mode == bm_stat) {
		/* Bailout due to break statement */
		run->thread_ar->bo_mode = bm_none;
	}

#ifdef DEBUG_RUN_TRACE
	printf("While statement terminated.\n");
#endif
}

/** Run @c raise statement.
 *
 * @param run		Runner object
 * @param raise_s	Raise statement to run
 */
static void run_raise(run_t *run, stree_raise_t *raise_s)
{
	rdata_item_t *rexpr;
	rdata_item_t *rexpr_vi;

#ifdef DEBUG_RUN_TRACE
	printf("Executing raise statement.\n");
#endif
	run_expr(run, raise_s->expr, &rexpr);
	if (run_is_bo(run))
		return;

	run_cvt_value_item(run, rexpr, &rexpr_vi);
	rdata_item_destroy(rexpr);
	if (run_is_bo(run))
		return;

	/* Store expression cspan in thread AR. */
	run->thread_ar->exc_cspan = raise_s->expr->cspan;

	/* Store expression result in thread AR. */
	/* XXX rexpr_vi is leaked here, we only return ->u.value */
	run->thread_ar->exc_payload = rexpr_vi->u.value;

	/* Start exception bailout. */
	run->thread_ar->bo_mode = bm_exc;
}

/** Run @c break statement.
 *
 * Forces control to return from the active breakable statement by setting
 * bailout mode to @c bm_stat.
 *
 * @param run		Runner object
 * @param break_s	Break statement to run
 */
static void run_break(run_t *run, stree_break_t *break_s)
{
#ifdef DEBUG_RUN_TRACE
	printf("Executing 'break' statement.\n");
#endif
	(void) break_s;

	/* Force control to ascend and leave the procedure. */
	if (run->thread_ar->bo_mode == bm_none)
		run->thread_ar->bo_mode = bm_stat;
}

/** Run @c return statement.
 *
 * Sets the return value in procedure AR and forces control to return
 * from the function by setting bailout mode to @c bm_proc.
 *
 * @param run		Runner object
 * @param return_s	Return statement to run
 */
static void run_return(run_t *run, stree_return_t *return_s)
{
	rdata_item_t *rexpr;
	rdata_item_t *rexpr_vi;
	run_proc_ar_t *proc_ar;

#ifdef DEBUG_RUN_TRACE
	printf("Executing return statement.\n");
#endif
	if (return_s->expr != NULL) {
		run_expr(run, return_s->expr, &rexpr);
		if (run_is_bo(run))
			return;

		run_cvt_value_item(run, rexpr, &rexpr_vi);
		rdata_item_destroy(rexpr);
		if (run_is_bo(run))
			return;

		/* Store expression result in procedure AR. */
		proc_ar = run_get_current_proc_ar(run);
		proc_ar->retval = rexpr_vi;
	}

	/* Force control to ascend and leave the procedure. */
	if (run->thread_ar->bo_mode == bm_none)
		run->thread_ar->bo_mode = bm_proc;
}

/** Run @c with-except-finally statement.
 *
 * Note: 'With' clause is not implemented.
 *
 * @param run		Runner object
 * @param wef_s		With-except-finally statement to run
 */
static void run_wef(run_t *run, stree_wef_t *wef_s)
{
	list_node_t *except_n;
	stree_except_t *except_c;
	rdata_value_t *exc_payload;
	run_bailout_mode_t bo_mode;

#ifdef DEBUG_RUN_TRACE
	printf("Executing with-except-finally statement.\n");
#endif
	run_block(run, wef_s->with_block);

	if (run->thread_ar->bo_mode == bm_exc) {
#ifdef DEBUG_RUN_TRACE
		printf("With statement detected exception.\n");
#endif
		/* Reset to normal execution. */
		run->thread_ar->bo_mode = bm_none;

		/* Look for an except block. */
		except_n = list_first(&wef_s->except_clauses);
		while (except_n != NULL) {
			except_c = list_node_data(except_n, stree_except_t *);
			if (run_exc_match(run, except_c))
				break;

			except_n = list_next(&wef_s->except_clauses, except_n);
		}

		/* If one was found, execute it. */
		if (except_n != NULL)
			run_block(run, except_c->block);

		/* Execute finally block */
		if (wef_s->finally_block != NULL) {
			/* Put exception on the side temporarily. */
			bo_mode = run->thread_ar->bo_mode;
			exc_payload = run->thread_ar->exc_payload;

			run->thread_ar->bo_mode = bm_none;
			run->thread_ar->exc_payload = NULL;

			run_block(run, wef_s->finally_block);

			if (bo_mode == bm_exc) {
				/*
				 * Restore the original exception. If another
				 * exception occured in the finally block (i.e.
				 * double fault), it is forgotten.
				 */
				run->thread_ar->bo_mode = bm_exc;
				run->thread_ar->exc_payload = exc_payload;
			}
		}
	}

#ifdef DEBUG_RUN_TRACE
	printf("With-except-finally statement terminated.\n");
#endif
}

/** Determine whether currently active exception matches @c except clause.
 *
 * Checks if the currently active exception in the runner object @c run
 * matches except clause @c except_c.
 *
 * @param run		Runner object
 * @param except_c	@c except clause
 * @return		@c b_true if there is a match, @c b_false otherwise
 */
static bool_t run_exc_match(run_t *run, stree_except_t *except_c)
{
	stree_csi_t *exc_csi;

	/* Get CSI of active exception. */
	exc_csi = run_exc_payload_get_csi(run);

	/* Determine if active exc. is derived from type in exc. clause. */
	/* XXX This is wrong, it does not work with generics. */
	return tdata_is_csi_derived_from_ti(exc_csi, except_c->titem);
}

/** Return CSI of the active exception.
 *
 * @param run		Runner object
 * @return		CSI of the active exception
 */
static stree_csi_t *run_exc_payload_get_csi(run_t *run)
{
	rdata_value_t *payload;
	rdata_var_t *payload_v;
	rdata_object_t *payload_o;

	payload = run->thread_ar->exc_payload;
	assert(payload != NULL);

	if (payload->var->vc != vc_ref) {
		/* XXX Prevent this via static type checking. */
		printf("Error: Exception payload must be an object "
		    "(found type %d).\n", payload->var->vc);
		exit(1);
	}

	payload_v = payload->var->u.ref_v->vref;
	if (payload_v->vc != vc_object) {
		/* XXX Prevent this via static type checking. */
		printf("Error: Exception payload must be an object "
		    "(found type %d).\n", payload_v->vc);
		exit(1);
	}

	payload_o = payload_v->u.object_v;

#ifdef DEBUG_RUN_TRACE
	printf("Active exception: '");
	symbol_print_fqn(payload_o->class_sym);
	printf("'.\n");
#endif
	assert(payload_o->class_sym != NULL);
	assert(payload_o->class_sym->sc == sc_csi);

	return payload_o->class_sym->u.csi;
}


/** Check for unhandled exception.
 *
 * Checks whether there is an active exception. If so, it prints an
 * error message and raises a run-time error.
 *
 * @param run		Runner object
 */
void run_exc_check_unhandled(run_t *run)
{
	stree_csi_t *exc_csi;

	if (run->thread_ar->bo_mode != bm_none) {
		assert(run->thread_ar->bo_mode == bm_exc);

		exc_csi = run_exc_payload_get_csi(run);

		if (run->thread_ar->exc_cspan != NULL) {
			cspan_print(run->thread_ar->exc_cspan);
			putchar(' ');
		}

		printf("Error: Unhandled exception '");
		symbol_print_fqn(csi_to_symbol(exc_csi));
		printf("'.\n");

		run_raise_error(run);
	}
}

/** Raise an irrecoverable run-time error, start bailing out.
 *
 * Raises an error that cannot be handled by the user program.
 *
 * @param run		Runner object
 */
void run_raise_error(run_t *run)
{
	run->thread_ar->bo_mode = bm_error;
	run->thread_ar->error = b_true;
}

/** Construct a special recovery item.
 *
 * @param run		Runner object
 */
rdata_item_t *run_recovery_item(run_t *run)
{
	(void) run;
	return NULL;
}

/** Find a local variable in the currently active function.
 *
 * @param run		Runner object
 * @param name		Name SID of the local variable
 * @return		Pointer to var node or @c NULL if not found
 */
rdata_var_t *run_local_vars_lookup(run_t *run, sid_t name)
{
	run_proc_ar_t *proc_ar;
	run_block_ar_t *block_ar;
	rdata_var_t *var;
	list_node_t *node;

	proc_ar = run_get_current_proc_ar(run);
	node = list_last(&proc_ar->block_ar);

	/* Walk through all block activation records. */
	while (node != NULL) {
		block_ar = list_node_data(node, run_block_ar_t *);
		var = intmap_get(&block_ar->vars, name);
		if (var != NULL)
			return var;

		node = list_prev(&proc_ar->block_ar, node);
	}

	/* No match */
	return NULL;
}

/** Get current procedure activation record.
 *
 * @param run		Runner object
 * @return		Active procedure AR
 */
run_proc_ar_t *run_get_current_proc_ar(run_t *run)
{
	list_node_t *node;

	node = list_last(&run->thread_ar->proc_ar);
	return list_node_data(node, run_proc_ar_t *);
}

/** Get current block activation record.
 *
 * @param run		Runner object
 * @return		Active block AR
 */
run_block_ar_t *run_get_current_block_ar(run_t *run)
{
	run_proc_ar_t *proc_ar;
	list_node_t *node;

	proc_ar = run_get_current_proc_ar(run);

	node = list_last(&proc_ar->block_ar);
	return list_node_data(node, run_block_ar_t *);
}

/** Get current CSI.
 *
 * @param run		Runner object
 * @return		Active CSI
 */
stree_csi_t *run_get_current_csi(run_t *run)
{
	run_proc_ar_t *proc_ar;

	proc_ar = run_get_current_proc_ar(run);
	return proc_ar->proc->outer_symbol->outer_csi;
}

/** Get static object (i.e. class object).
 *
 * Looks for a child static object named @a name in static object @a pobject.
 * If the child does not exist yet, it is created.
 *
 * @param run		Runner object
 * @param csi		CSI of the static object named @a name
 * @param pobject	Parent static object
 * @param name		Static object name
 *
 * @return		Static (class) object of the given name
 */
rdata_var_t *run_sobject_get(run_t *run, stree_csi_t *csi,
    rdata_var_t *pobj_var, sid_t name)
{
	rdata_object_t *pobject;
	rdata_var_t *mbr_var;
	rdata_var_t *rvar;
	stree_ident_t *ident;

	assert(pobj_var->vc == vc_object);
	pobject = pobj_var->u.object_v;
#ifdef DEBUG_RUN_TRACE
	printf("Get static object '%s' in '", strtab_get_str(name));
	if (pobject->class_sym != NULL)
		symbol_print_fqn(pobject->class_sym);
	else
		printf("global");
#endif

	assert(pobject->static_obj == sn_static);

	mbr_var = intmap_get(&pobject->fields, name);
	if (mbr_var != NULL) {
#ifdef DEBUG_RUN_TRACE
		printf("Return exising static object (mbr_var=%p).\n", mbr_var);
#endif
		/* Return existing object. */
		return mbr_var;
	}

	/* Construct new object. */
#ifdef DEBUG_RUN_TRACE
	printf("Construct new static object.\n");
#endif
	ident = stree_ident_new();
	ident->sid = name;

	run_new_csi_inst(run, csi, sn_static, &rvar);

	/* Store static object reference for future use. */
	intmap_set(&pobject->fields, name, rvar);

	return rvar;
}

/** Get static object for CSI.
 *
 * In situations where we do not have the parent static object and need
 * to find static object for a CSI from the gdata root we use this.
 *
 * This is only used in special cases such as when invoking the entry
 * point. This is too slow to use during normal execution.
 *
 * @param run		Runner object
 * @param csi		CSI to get static object of
 *
 * @return		Static (class) object
 */
rdata_var_t *run_sobject_find(run_t *run, stree_csi_t *csi)
{
	rdata_var_t *pobj_var;

	if (csi == NULL)
		return run->gdata;

	assert(csi->ancr_state == ws_visited);
	pobj_var = run_sobject_find(run, csi_to_symbol(csi)->outer_csi);

	return run_sobject_get(run, csi, pobj_var, csi->name->sid);
}

/** Get static object for CSI containing function.
 *
 * This is used to obtain active object for invoking static function
 * @a fun. Only used in cases where we don't already have the object such
 * as when running the entry point. Otherwise this would be slow.
 *
 * @param run		Runner object
 * @param fun		Function to get static class object of
 *
 * @return		Static (class) object
 */
rdata_var_t *run_fun_sobject_find(run_t *run, stree_fun_t *fun)
{
	return run_sobject_find(run, fun_to_symbol(fun)->outer_csi);
}

/** Construct variable from a value item.
 *
 * XXX This should be in fact implemented using existing code as:
 *
 * (1) Create a variable of the desired type.
 * (2) Initialize the variable with the provided value.
 *
 * @param item		Value item (initial value for variable).
 * @param var		Place to store new var node.
 */
void run_value_item_to_var(rdata_item_t *item, rdata_var_t **var)
{
	rdata_bool_t *bool_v;
	rdata_char_t *char_v;
	rdata_deleg_t *deleg_v;
	rdata_enum_t *enum_v;
	rdata_int_t *int_v;
	rdata_string_t *string_v;
	rdata_ref_t *ref_v;
	rdata_var_t *in_var;

	assert(item->ic == ic_value);
	in_var = item->u.value->var;

	switch (in_var->vc) {
	case vc_bool:
		*var = rdata_var_new(vc_bool);
		bool_v = rdata_bool_new();

		(*var)->u.bool_v = bool_v;
		bool_v->value = item->u.value->var->u.bool_v->value;
		break;
	case vc_char:
		*var = rdata_var_new(vc_char);
		char_v = rdata_char_new();

		(*var)->u.char_v = char_v;
		bigint_clone(&item->u.value->var->u.char_v->value,
		    &char_v->value);
		break;
	case vc_deleg:
		*var = rdata_var_new(vc_deleg);
		deleg_v = rdata_deleg_new();

		(*var)->u.deleg_v = deleg_v;
		deleg_v->obj = item->u.value->var->u.deleg_v->obj;
		deleg_v->sym = item->u.value->var->u.deleg_v->sym;
		break;
	case vc_enum:
		*var = rdata_var_new(vc_enum);
		enum_v = rdata_enum_new();

		(*var)->u.enum_v = enum_v;
		enum_v->value = item->u.value->var->u.enum_v->value;
		break;
	case vc_int:
		*var = rdata_var_new(vc_int);
		int_v = rdata_int_new();

		(*var)->u.int_v = int_v;
		bigint_clone(&item->u.value->var->u.int_v->value,
		    &int_v->value);
		break;
	case vc_string:
		*var = rdata_var_new(vc_string);
		string_v = rdata_string_new();

		(*var)->u.string_v = string_v;
		string_v->value = item->u.value->var->u.string_v->value;
		break;
	case vc_ref:
		*var = rdata_var_new(vc_ref);
		ref_v = rdata_ref_new();

		(*var)->u.ref_v = ref_v;
		ref_v->vref = item->u.value->var->u.ref_v->vref;
		break;
	default:
		printf("Error: Unimplemented argument type.\n");
		exit(1);

	}
}

/** Construct a procedure AR.
 *
 * @param run		Runner object
 * @param obj		Object whose procedure is being activated
 * @param proc		Procedure that is being activated
 * @param rproc_ar	Place to store pointer to new activation record
 */
void run_proc_ar_create(run_t *run, rdata_var_t *obj, stree_proc_t *proc,
    run_proc_ar_t **rproc_ar)
{
	run_proc_ar_t *proc_ar;
	run_block_ar_t *block_ar;

	(void) run;

	/* Create procedure activation record. */
	proc_ar = run_proc_ar_new();
	proc_ar->obj = obj;
	proc_ar->proc = proc;
	list_init(&proc_ar->block_ar);

	proc_ar->retval = NULL;

	/* Create special block activation record to hold function arguments. */
	block_ar = run_block_ar_new();
	intmap_init(&block_ar->vars);
	list_append(&proc_ar->block_ar, block_ar);

	*rproc_ar = proc_ar;
}

/** Destroy a procedure AR.
 *
 * @param run		Runner object
 * @param proc_ar	Pointer to procedure activation record
 */
void run_proc_ar_destroy(run_t *run, run_proc_ar_t *proc_ar)
{
	list_node_t *ar_node;
	run_block_ar_t *block_ar;

	(void) run;

	/* Destroy special block activation record. */
	ar_node = list_first(&proc_ar->block_ar);
	block_ar = list_node_data(ar_node, run_block_ar_t *);
	list_remove(&proc_ar->block_ar, ar_node);
	run_block_ar_destroy(run, block_ar);

	/* Destroy procedure activation record. */
	proc_ar->obj = NULL;
	proc_ar->proc = NULL;
	list_fini(&proc_ar->block_ar);
	proc_ar->retval = NULL;
	run_proc_ar_delete(proc_ar);
}


/** Fill arguments in a procedure AR.
 *
 * When invoking a procedure this is used to store the argument values
 * in the activation record.
 *
 * @param run		Runner object
 * @param proc_ar	Existing procedure activation record where to store
 *			the values
 * @param arg_vals	List of value items (rdata_item_t *) -- real
 *			argument values
 */
void run_proc_ar_set_args(run_t *run, run_proc_ar_t *proc_ar, list_t *arg_vals)
{
	stree_ctor_t *ctor;
	stree_fun_t *fun;
	stree_prop_t *prop;
	list_t *args;
	stree_proc_arg_t *varg;
	stree_symbol_t *outer_symbol;

	run_block_ar_t *block_ar;
	list_node_t *block_ar_n;
	list_node_t *rarg_n, *parg_n;
	list_node_t *cn;
	rdata_item_t *rarg;
	stree_proc_arg_t *parg;
	rdata_var_t *var;
	rdata_var_t *ref_var;
	rdata_ref_t *ref;
	rdata_array_t *array;
	rdata_var_t *elem_var;
	int n_vargs, idx;

	(void) run;

	/* AR should have been created with run_proc_ar_create(). */
	assert(proc_ar->proc != NULL);
	outer_symbol = proc_ar->proc->outer_symbol;

	/* Make compiler happy. */
	args = NULL;
	varg = NULL;

	/*
	 * The procedure being activated should belong to a member function or
	 * property getter/setter.
	 */
	switch (outer_symbol->sc) {
	case sc_ctor:
		ctor = symbol_to_ctor(outer_symbol);
		args = &ctor->sig->args;
		varg = ctor->sig->varg;
		break;
	case sc_fun:
		fun = symbol_to_fun(outer_symbol);
		args = &fun->sig->args;
		varg = fun->sig->varg;
		break;
	case sc_prop:
		prop = symbol_to_prop(outer_symbol);
		args = &prop->args;
		varg = prop->varg;
		break;
	case sc_csi:
	case sc_deleg:
	case sc_enum:
	case sc_var:
		assert(b_false);
	}

	/* Fetch first block activation record. */
	block_ar_n = list_first(&proc_ar->block_ar);
	assert(block_ar_n != NULL);
	block_ar = list_node_data(block_ar_n, run_block_ar_t *);

	/* Declare local variables to hold argument values. */
	rarg_n = list_first(arg_vals);
	parg_n = list_first(args);

	while (parg_n != NULL) {
		if (rarg_n == NULL) {
			printf("Error: Too few arguments to '");
			symbol_print_fqn(outer_symbol);
			printf("'.\n");
			exit(1);
		}

		rarg = list_node_data(rarg_n, rdata_item_t *);
		parg = list_node_data(parg_n, stree_proc_arg_t *);

		assert(rarg->ic == ic_value);

		/* Construct a variable from the argument value. */
		run_value_item_to_var(rarg, &var);

		/* Declare variable using name of formal argument. */
		intmap_set(&block_ar->vars, parg->name->sid, var);

		rarg_n = list_next(arg_vals, rarg_n);
		parg_n = list_next(args, parg_n);
	}

	if (varg != NULL) {
		/* Function is variadic. Count number of variadic arguments. */
		cn = rarg_n;
		n_vargs = 0;
		while (cn != NULL) {
			n_vargs += 1;
			cn = list_next(arg_vals, cn);
		}

		/* Prepare array to store variadic arguments. */
		array = rdata_array_new(1);
		array->extent[0] = n_vargs;
		rdata_array_alloc_element(array);

		/* Read variadic arguments. */

		idx = 0;
		while (rarg_n != NULL) {
			rarg = list_node_data(rarg_n, rdata_item_t *);
			assert(rarg->ic == ic_value);

			run_value_item_to_var(rarg, &elem_var);
			array->element[idx] = elem_var;

			rarg_n = list_next(arg_vals, rarg_n);
			idx += 1;
		}

		var = rdata_var_new(vc_array);
		var->u.array_v = array;

		/* Create reference to the new array. */
		ref_var = rdata_var_new(vc_ref);
		ref = rdata_ref_new();
		ref_var->u.ref_v = ref;
		ref->vref = var;

		/* Declare variable using name of formal argument. */
		intmap_set(&block_ar->vars, varg->name->sid,
		    ref_var);
	}

	/* Check for excess real parameters. */
	if (rarg_n != NULL) {
		printf("Error: Too many arguments to '");
		symbol_print_fqn(outer_symbol);
		printf("'.\n");
		exit(1);
	}
}

/** Fill setter argument in a procedure AR.
 *
 * When invoking a setter this is used to store its argument value in its
 * procedure activation record.
 *
 * @param run		Runner object
 * @param proc_ar	Existing procedure activation record where to store
 *			the setter argument
 * @param arg_val	Value items (rdata_item_t *) -- real argument value
 */
void run_proc_ar_set_setter_arg(run_t *run, run_proc_ar_t *proc_ar,
    rdata_item_t *arg_val)
{
	stree_prop_t *prop;
	run_block_ar_t *block_ar;
	list_node_t *block_ar_n;
	rdata_var_t *var;

	(void) run;

	/* AR should have been created with run_proc_ar_create(). */
	assert(proc_ar->proc != NULL);

	/* The procedure being activated should belong to a property setter. */
	prop = symbol_to_prop(proc_ar->proc->outer_symbol);
	assert(prop != NULL);
	assert(proc_ar->proc == prop->setter);

	/* Fetch first block activation record. */
	block_ar_n = list_first(&proc_ar->block_ar);
	assert(block_ar_n != NULL);
	block_ar = list_node_data(block_ar_n, run_block_ar_t *);

	assert(arg_val->ic == ic_value);

	/* Construct a variable from the argument value. */
	run_value_item_to_var(arg_val, &var);

	/* Declare variable using name of formal argument. */
	intmap_set(&block_ar->vars, prop->setter_arg->name->sid, var);
}

/** Print function activation backtrace.
 *
 * Prints a backtrace of activated functions for debugging purposes.
 *
 * @param run		Runner object
 */
void run_print_fun_bt(run_t *run)
{
	list_node_t *node;
	run_proc_ar_t *proc_ar;

	printf("Backtrace:\n");
	node = list_last(&run->thread_ar->proc_ar);
	while (node != NULL) {
		printf(" * ");
		proc_ar = list_node_data(node, run_proc_ar_t *);
		symbol_print_fqn(proc_ar->proc->outer_symbol);
		printf("\n");

		node = list_prev(&run->thread_ar->proc_ar, node);
	}
}

/** Destroy a block AR.
 *
 * @param run		Runner object
 * @param proc_ar	Pointer to block activation record
 */
void run_block_ar_destroy(run_t *run, run_block_ar_t *block_ar)
{
	map_elem_t *elem;
	rdata_var_t *var;
	int key;

	(void) run;

	elem = intmap_first(&block_ar->vars);
	while (elem != NULL) {
		/* Destroy the variable */
		var = intmap_elem_get_value(elem);
		rdata_var_destroy(var);

		/* Remove the map element */
		key = intmap_elem_get_key(elem);
		intmap_set(&block_ar->vars, key, NULL);

		elem = intmap_first(&block_ar->vars);
	}

	intmap_fini(&block_ar->vars);
	run_block_ar_delete(block_ar);
}

/** Convert item to value item.
 *
 * If @a item is a value, we just return a copy. If @a item is an address,
 * we read from the address.
 *
 * @param run		Runner object
 * @param item		Input item (value or address)
 * @param ritem		Place to store pointer to new value item
 */
void run_cvt_value_item(run_t *run, rdata_item_t *item, rdata_item_t **ritem)
{
	rdata_value_t *value;

	/*
	 * This can happen when trying to use output of a function which
	 * does not return a value.
	 */
	if (item == NULL) {
		printf("Error: Sub-expression has no value.\n");
		exit(1);
	}

	/* Address item. Perform read operation. */
	if (item->ic == ic_address) {
		run_address_read(run, item->u.address, ritem);
		return;
	}

	/* Make a copy of the var node within. */
	value = rdata_value_new();
	rdata_var_copy(item->u.value->var, &value->var);
	*ritem = rdata_item_new(ic_value);
	(*ritem)->u.value = value;
}

/** Get item var-class.
 *
 * Get var-class of @a item, regardless whether it is a value or address.
 * (I.e. the var class of the value or variable at the given address).
 *
 * @param run		Runner object
 * @param item		Value or address item
 * @return		Varclass of @a item
 */
var_class_t run_item_get_vc(run_t *run, rdata_item_t *item)
{
	var_class_t vc;
	rdata_var_t *tpos;

	(void) run;

	switch (item->ic) {
	case ic_value:
		vc = item->u.value->var->vc;
		break;
	case ic_address:
		switch (item->u.address->ac) {
		case ac_var:
			vc = item->u.address->u.var_a->vref->vc;
			break;
		case ac_prop:
			/* Prefetch the value of the property. */
			tpos = run_aprop_get_tpos(run, item->u.address);
			vc = tpos->vc;
			break;
		default:
			assert(b_false);
		}
		break;
	default:
		assert(b_false);
	}

	return vc;
}

/** Get pointer to current var node in temporary copy in property address.
 *
 * A property address refers to a specific @c var node in a property.
 * This function will fetch a copy of the property value (by running
 * its getter) if there is not a temporary copy in the address yet.
 * It returns a pointer to the relevant @c var node in the temporary
 * copy.
 *
 * @param run	Runner object
 * @param addr	Address of class @c ac_prop
 * @return	Pointer to var node
 */
static rdata_var_t *run_aprop_get_tpos(run_t *run, rdata_address_t *addr)
{
	rdata_item_t *ritem;

	assert(addr->ac == ac_prop);

	if (addr->u.prop_a->tvalue == NULL) {
		/* Fetch value of the property. */
		run_address_read(run, addr, &ritem);
		assert(ritem->ic == ic_value);
		addr->u.prop_a->tvalue = ritem->u.value;
		addr->u.prop_a->tpos = addr->u.prop_a->tvalue->var;
	}

	return addr->u.prop_a->tpos;
}

/** Read data from an address.
 *
 * Read value from the specified address.
 *
 * @param run		Runner object
 * @param address	Address to read
 * @param ritem		Place to store pointer to the value that was read
 */
void run_address_read(run_t *run, rdata_address_t *address,
    rdata_item_t **ritem)
{
	(void) run;
	assert(ritem != NULL);

	switch (address->ac) {
	case ac_var:
		rdata_var_read(address->u.var_a->vref, ritem);
		break;
	case ac_prop:
		run_aprop_read(run, address->u.prop_a, ritem);
		break;
	}

	assert(*ritem == NULL || (*ritem)->ic == ic_value);
}

/** Write data to an address.
 *
 * Store value @a value at address @a address.
 *
 * @param run		Runner object
 * @param address	Address to write
 * @param value		Value to store at the address
 */
void run_address_write(run_t *run, rdata_address_t *address,
    rdata_value_t *value)
{
	(void) run;

	switch (address->ac) {
	case ac_var:
		rdata_var_write(address->u.var_a->vref, value);
		break;
	case ac_prop:
		run_aprop_write(run, address->u.prop_a, value);
		break;
	}
}

/** Read data from a property address.
 *
 * This involves invoking the property getter procedure.
 *
 * @param run		Runner object.
 * @param addr_prop	Property address to read.
 * @param ritem		Place to store pointer to the value that was read.
 */
static void run_aprop_read(run_t *run, rdata_addr_prop_t *addr_prop,
    rdata_item_t **ritem)
{
	rdata_deleg_t *deleg;
	rdata_var_t *obj;
	stree_symbol_t *prop_sym;
	stree_prop_t *prop;

	run_proc_ar_t *proc_ar;

	rdata_var_t *cvar;

#ifdef DEBUG_RUN_TRACE
	printf("Read from property.\n");
#endif
	/*
	 * If @c tvalue is present, we need to use the relevant part from that
	 * instead of re-reading the whole thing.
	 */
	if (addr_prop->tvalue != NULL) {
		/* Copy the value */
		rdata_var_copy(addr_prop->tpos, &cvar);
		*ritem = rdata_item_new(ic_value);
		(*ritem)->u.value = rdata_value_new();
		(*ritem)->u.value->var = cvar;
		return;
	}

	if (addr_prop->apc == apc_named)
		deleg = addr_prop->u.named->prop_d;
	else
		deleg = addr_prop->u.indexed->object_d;

	obj = deleg->obj;
	prop_sym = deleg->sym;
	prop = symbol_to_prop(prop_sym);
	assert(prop != NULL);

	if (prop->getter == NULL) {
		printf("Error: Property is not readable.\n");
		exit(1);
	}

	/* Create procedure activation record. */
	run_proc_ar_create(run, obj, prop->getter, &proc_ar);

	/* Fill in arguments (indices). */
	if (addr_prop->apc == apc_indexed) {
		run_proc_ar_set_args(run, proc_ar,
		    &addr_prop->u.indexed->args);
	}

	/* Run getter. */
	run_proc(run, proc_ar, ritem);

	/* Destroy procedure activation record. */
	run_proc_ar_destroy(run, proc_ar);

#ifdef DEBUG_RUN_TRACE
	printf("Getter returns ");
	rdata_item_print(*ritem);
	printf(".\n");
	printf("Done reading from property.\n");
#endif
}

/** Write data to a property address.
 *
 * This involves invoking the property setter procedure.
 *
 * @param run		Runner object
 * @param addr_prop	Property address to write
 * @param value		Value to store at the address
 */
static void run_aprop_write(run_t *run, rdata_addr_prop_t *addr_prop,
    rdata_value_t *value)
{
	rdata_deleg_t *deleg;
	rdata_var_t *obj;
	stree_symbol_t *prop_sym;
	stree_prop_t *prop;

	run_proc_ar_t *proc_ar;
	rdata_item_t *vitem;
	rdata_item_t *ritem;

#ifdef DEBUG_RUN_TRACE
	printf("Write to property.\n");
#endif
	/* If @c tvalue is present, we need to modify it and write it back. */
	if (addr_prop->tvalue != NULL) {
		printf("Unimplemented: Read-modify-write property access.\n");
		exit(1);
	}

	if (addr_prop->apc == apc_named)
		deleg = addr_prop->u.named->prop_d;
	else
		deleg = addr_prop->u.indexed->object_d;

	obj = deleg->obj;
	prop_sym = deleg->sym;
	prop = symbol_to_prop(prop_sym);
	assert(prop != NULL);

	if (prop->setter == NULL) {
		printf("Error: Property is not writable.\n");
		exit(1);
	}

	vitem = rdata_item_new(ic_value);
	vitem->u.value = value;

	/* Create procedure activation record. */
	run_proc_ar_create(run, obj, prop->setter, &proc_ar);

	/* Fill in arguments (indices). */
	if (addr_prop->apc == apc_indexed) {
		run_proc_ar_set_args(run, proc_ar,
		    &addr_prop->u.indexed->args);
	}

	/* Fill in value argument for setter. */
	run_proc_ar_set_setter_arg(run, proc_ar, vitem);

	/* Run setter. */
	run_proc(run, proc_ar, &ritem);

	/* Setter should not return a value. */
	assert(ritem == NULL);

	/* Destroy procedure activation record. */
	run_proc_ar_destroy(run, proc_ar);

#ifdef DEBUG_RUN_TRACE
	printf("Done writing to property.\n");
#endif
}

/** Return reference to a variable.
 *
 * Constructs a reference (value item) pointing to @a var.
 *
 * @param run		Runner object
 * @param var		Variable node that is being referenced
 * @param res		Place to store pointer to new reference.
 */
void run_reference(run_t *run, rdata_var_t *var, rdata_item_t **res)
{
	rdata_ref_t *ref;
	rdata_var_t *ref_var;
	rdata_value_t *ref_value;
	rdata_item_t *ref_item;

	(void) run;

	/* Create reference to the variable. */
	ref = rdata_ref_new();
	ref_var = rdata_var_new(vc_ref);
	ref->vref = var;
	ref_var->u.ref_v = ref;

	/* Construct value of the reference to return. */
	ref_item = rdata_item_new(ic_value);
	ref_value = rdata_value_new();
	ref_item->u.value = ref_value;
	ref_value->var = ref_var;

	*res = ref_item;
}

/** Return address of reference target.
 *
 * Takes a reference (address or value) and returns the address (item) of
 * the target of the reference.
 *
 * @param run		Runner object
 * @param ref		Reference
 * @param cspan		Cspan to put into exception if reference is nil
 *			or @c NULL if no cspan is provided.
 * @param rtitem	Place to store pointer to the resulting address.
 */
void run_dereference(run_t *run, rdata_item_t *ref, cspan_t *cspan,
    rdata_item_t **ritem)
{
	rdata_item_t *ref_val;
	rdata_item_t *item;
	rdata_address_t *address;
	rdata_addr_var_t *addr_var;

#ifdef DEBUG_RUN_TRACE
	printf("run_dereference()\n");
#endif
	run_cvt_value_item(run, ref, &ref_val);
	if (run_is_bo(run)) {
		*ritem = run_recovery_item(run);
		return;
	}

	assert(ref_val->u.value->var->vc == vc_ref);

	item = rdata_item_new(ic_address);
	address = rdata_address_new(ac_var);
	addr_var = rdata_addr_var_new();
	item->u.address = address;
	address->u.var_a = addr_var;
	addr_var->vref = ref_val->u.value->var->u.ref_v->vref;

	rdata_item_destroy(ref_val);

	if (addr_var->vref == NULL) {
#ifdef DEBUG_RUN_TRACE
		printf("Error: Accessing null reference.\n");
#endif
		/* Raise Error.NilReference */
		run_raise_exc(run, run->program->builtin->error_nilreference,
		    cspan);
		*ritem = run_recovery_item(run);
		return;
	}

#ifdef DEBUG_RUN_TRACE
	printf("vref set to %p\n", addr_var->vref);
#endif
	*ritem = item;
}

/** Raise an exception of the given class.
 *
 * Used when the interpreter generates an exception due to a run-time
 * error (not for the @c raise statement).
 *
 * @param run		Runner object
 * @param csi		Exception class
 * @param cspan		Cspan of code that caused exception (for debugging)
 */
void run_raise_exc(run_t *run, stree_csi_t *csi, cspan_t *cspan)
{
	rdata_item_t *exc_vi;

	/* Store exception cspan in thread AR. */
	run->thread_ar->exc_cspan = cspan;

	/* Create exception object. */
	run_new_csi_inst_ref(run, csi, sn_nonstatic, &exc_vi);
	assert(exc_vi->ic == ic_value);

	/* Store exception object in thread AR. */
	run->thread_ar->exc_payload = exc_vi->u.value;

	/* Start exception bailout. */
	run->thread_ar->bo_mode = bm_exc;
}

/** Determine if we are bailing out.
 *
 * @param run		Runner object
 * @return		@c b_true if we are bailing out, @c b_false otherwise
 */
bool_t run_is_bo(run_t *run)
{
	return run->thread_ar->bo_mode != bm_none;
}

/** Construct a new variable of the given type.
 *
 * The variable is allocated and initialized with a default value
 * based on type item @a ti. For reference types the default value
 * is a null reference. At this point this does not work for generic
 * types (we need RTTI).
 *
 * @param run		Runner object
 * @param ti		Type of variable to create (type item)
 * @param rvar		Place to store pointer to new variable
 */
void run_var_new(run_t *run, tdata_item_t *ti, rdata_var_t **rvar)
{
	rdata_var_t *var;

	switch (ti->tic) {
	case tic_tprimitive:
		run_var_new_tprimitive(run, ti->u.tprimitive, rvar);
		break;
	case tic_tobject:
	case tic_tarray:
		run_var_new_null_ref(run, rvar);
		break;
	case tic_tdeleg:
		run_var_new_deleg(run, rvar);
		break;
	case tic_tebase:
		/*
		 * One cannot declare variable of ebase type. It is just
		 * type of expressions referring to enum types.
		 */
		assert(b_false);
		/* Fallthrough */
	case tic_tenum:
		run_var_new_enum(run, ti->u.tenum, rvar);
		break;
	case tic_tfun:
		run_var_new_deleg(run, rvar);
		break;
	case tic_tvref:
		/*
		 * XXX Need to obtain run-time value of type argument to
		 * initialize variable properly.
		 */
		var = rdata_var_new(vc_int);
		var->u.int_v = rdata_int_new();
		bigint_init(&var->u.int_v->value, 0);
		*rvar = var;
		break;
	case tic_ignore:
		assert(b_false);
	}
}

/** Construct a new variable of primitive type.
 *
 * The variable is allocated and initialized with a default value
 * based on primitive type item @a tprimitive.
 *
 * @param run		Runner object
 * @param ti		Primitive type of variable to create
 * @param rvar		Place to store pointer to new variable
 */
static void run_var_new_tprimitive(run_t *run, tdata_primitive_t *tprimitive,
    rdata_var_t **rvar)
{
	rdata_var_t *var;

	(void) run;

	/* Make compiler happy. */
	var = NULL;

	switch (tprimitive->tpc) {
	case tpc_bool:
		var = rdata_var_new(vc_bool);
		var->u.bool_v = rdata_bool_new();
		var->u.bool_v->value = b_false;
		break;
	case tpc_char:
		var = rdata_var_new(vc_char);
		var->u.char_v = rdata_char_new();
		bigint_init(&var->u.char_v->value, 0);
		break;
	case tpc_int:
		var = rdata_var_new(vc_int);
		var->u.int_v = rdata_int_new();
		bigint_init(&var->u.int_v->value, 0);
		break;
	case tpc_nil:
		assert(b_false);
		/* Fallthrough */
	case tpc_string:
		var = rdata_var_new(vc_string);
		var->u.string_v = rdata_string_new();
		var->u.string_v->value = "";
		break;
	case tpc_resource:
		var = rdata_var_new(vc_resource);
		var->u.resource_v = rdata_resource_new();
		var->u.resource_v->data = NULL;
		break;
	}

	*rvar = var;
}

/** Construct a new variable containing null reference.
 *
 * @param run		Runner object
 * @param rvar		Place to store pointer to new variable
 */
static void run_var_new_null_ref(run_t *run, rdata_var_t **rvar)
{
	rdata_var_t *var;

	(void) run;

	/* Return null reference. */
	var = rdata_var_new(vc_ref);
	var->u.ref_v = rdata_ref_new();

	*rvar = var;
}

/** Construct a new variable containing invalid delegate.
 *
 * @param run		Runner object
 * @param rvar		Place to store pointer to new variable
 */
static void run_var_new_deleg(run_t *run, rdata_var_t **rvar)
{
	rdata_var_t *var;

	(void) run;

	/* Return null reference. */
	var = rdata_var_new(vc_deleg);
	var->u.deleg_v = rdata_deleg_new();

	*rvar = var;
}

/** Construct a new variable containing default value of an enum type.
 *
 * @param run		Runner object
 * @param rvar		Place to store pointer to new variable
 */
static void run_var_new_enum(run_t *run, tdata_enum_t *tenum,
    rdata_var_t **rvar)
{
	rdata_var_t *var;
	list_node_t *embr_n;
	stree_embr_t *embr;

	(void) run;

	/* Get first member of enum which will serve as default value. */
	embr_n = list_first(&tenum->enum_d->members);
	assert(embr_n != NULL);

	embr = list_node_data(embr_n, stree_embr_t *);

	/* Return null reference. */
	var = rdata_var_new(vc_enum);
	var->u.enum_v = rdata_enum_new();
	var->u.enum_v->value = embr;

	*rvar = var;
}

/** Construct a new thread activation record.
 *
 * @param run	Runner object
 * @return	New thread AR.
 */
run_thread_ar_t *run_thread_ar_new(void)
{
	run_thread_ar_t *thread_ar;

	thread_ar = calloc(1, sizeof(run_thread_ar_t));
	if (thread_ar == NULL) {
		printf("Memory allocation failed.\n");
		exit(1);
	}

	return thread_ar;
}

/** Allocate a new procedure activation record.
 *
 * @return	New procedure AR.
 */
run_proc_ar_t *run_proc_ar_new(void)
{
	run_proc_ar_t *proc_ar;

	proc_ar = calloc(1, sizeof(run_proc_ar_t));
	if (proc_ar == NULL) {
		printf("Memory allocation failed.\n");
		exit(1);
	}

	return proc_ar;
}

/** Deallocate a procedure activation record.
 *
 * @return	New procedure AR.
 */
void run_proc_ar_delete(run_proc_ar_t *proc_ar)
{
	assert(proc_ar != NULL);
	free(proc_ar);
}

/** Allocate a new block activation record.
 *
 * @param run	Runner object
 * @return	New block AR.
 */
run_block_ar_t *run_block_ar_new(void)
{
	run_block_ar_t *block_ar;

	block_ar = calloc(1, sizeof(run_block_ar_t));
	if (block_ar == NULL) {
		printf("Memory allocation failed.\n");
		exit(1);
	}

	return block_ar;
}

/** Deallocate a new block activation record.
 *
 * @param run	Runner object
 * @return	New block AR.
 */
void run_block_ar_delete(run_block_ar_t *block_ar)
{
	assert(block_ar != NULL);
	free(block_ar);
}
