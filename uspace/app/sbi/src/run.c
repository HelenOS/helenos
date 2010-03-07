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

/** @file Runner (executes the code). */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "builtin.h"
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

#include "run.h"

static void run_block(run_t *run, stree_block_t *block);
static void run_stat(run_t *run, stree_stat_t *stat);
static void run_exps(run_t *run, stree_exps_t *exps);
static void run_vdecl(run_t *run, stree_vdecl_t *vdecl);
static void run_if(run_t *run, stree_if_t *if_s);
static void run_while(run_t *run, stree_while_t *while_s);
static void run_raise(run_t *run, stree_raise_t *raise_s);
static void run_return(run_t *run, stree_return_t *return_s);
static void run_wef(run_t *run, stree_wef_t *wef_s);

static bool_t run_exc_match(run_t *run, stree_except_t *except_c);

/** Initialize runner instance. */
void run_init(run_t *run)
{
	(void) run;
}

/** Run program */
void run_program(run_t *run, stree_program_t *prog)
{
	stree_symbol_t *main_fun_sym;
	stree_fun_t *main_fun;
	stree_ident_t *fake_ident;
	list_t main_args;
	run_fun_ar_t *fun_ar;
	rdata_item_t *res;

	/* Note down link to program code. */
	run->program = prog;

	/* Initialize thread activation record. */
	run->thread_ar = run_thread_ar_new();
	list_init(&run->thread_ar->fun_ar);
	run->thread_ar->bo_mode = bm_none;

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

#ifdef DEBUG_RUN_TRACE
	printf("Found function '"); symbol_print_fqn(prog, main_fun_sym);
	printf("'.\n");
#endif

	/* Run function @c main. */
	list_init(&main_args);
	run_fun_ar_create(run, NULL, main_fun, &fun_ar);
	run_fun_ar_set_args(run, fun_ar, &main_args);
	run_fun(run, fun_ar, &res);

	/* Check for unhandled exceptions. */
	if (run->thread_ar->bo_mode != bm_none) {
		assert(run->thread_ar->bo_mode == bm_exc);
		printf("Error: Unhandled exception.\n");
		exit(1);
	}
}

/** Run member function */
void run_fun(run_t *run, run_fun_ar_t *fun_ar, rdata_item_t **res)
{
	stree_symbol_t *fun_sym;
	stree_fun_t *fun;
	list_node_t *node;

	fun_sym = fun_ar->fun_sym;
	fun = symbol_to_fun(fun_sym);
	assert(fun != NULL);

#ifdef DEBUG_RUN_TRACE
	printf("Start executing function '");
	symbol_print_fqn(run->program, fun_sym);
	printf("'.\n");
#endif
	/* Add function AR to the stack. */
	list_append(&run->thread_ar->fun_ar, fun_ar);

	/* Run main function block. */
	if (fun->body != NULL) {
		run_block(run, fun->body);
	} else {
		builtin_run_fun(run, fun_sym);
	}

	/* Handle bailout. */
	switch (run->thread_ar->bo_mode) {
	case bm_stat:
		printf("Error: Misplaced 'break' statement.\n");
		exit(1);
	case bm_fun:
		run->thread_ar->bo_mode = bm_none;
		break;
	default:
		break;
	}

#ifdef DEBUG_RUN_TRACE
	printf("Done executing function '");
	symbol_print_fqn(run->program, fun_sym);
	printf("'.\n");

	run_print_fun_bt(run);
#endif
	/* Remove function activation record from the stack. */
	node = list_last(&run->thread_ar->fun_ar);
	assert(list_node_data(node, run_fun_ar_t *) == fun_ar);
	list_remove(&run->thread_ar->fun_ar, node);

	*res = fun_ar->retval;
}

/** Run code block */
static void run_block(run_t *run, stree_block_t *block)
{
	run_fun_ar_t *fun_ar;
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
	fun_ar = run_get_current_fun_ar(run);
	list_append(&fun_ar->block_ar, block_ar);

	node = list_first(&block->stats);
	while (node != NULL) {
		stat = list_node_data(node, stree_stat_t *);
		run_stat(run, stat);

		if (run->thread_ar->bo_mode != bm_none)
			break;

		node = list_next(&block->stats, node);
	}

#ifdef DEBUG_RUN_TRACE
	printf("Done executing code block.\n");
#endif

	/* Remove block activation record from the stack. */
	node = list_last(&fun_ar->block_ar);
	assert(list_node_data(node, run_block_ar_t *) == block_ar);
	list_remove(&fun_ar->block_ar, node);
}

/** Run statement. */
static void run_stat(run_t *run, stree_stat_t *stat)
{
#ifdef DEBUG_RUN_TRACE
	printf("Executing one statement %p.\n", stat);
#endif

	switch (stat->sc) {
	case st_exps:
		run_exps(run, stat->u.exp_s);
		break;
	case st_vdecl:
		run_vdecl(run, stat->u.vdecl_s);
		break;
	case st_if:
		run_if(run, stat->u.if_s);
		break;
	case st_while:
		run_while(run, stat->u.while_s);
		break;
	case st_raise:
		run_raise(run, stat->u.raise_s);
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
	default:
		assert(b_false);
	}
}

/** Run expression statement. */
static void run_exps(run_t *run, stree_exps_t *exps)
{
	rdata_item_t *rexpr;

#ifdef DEBUG_RUN_TRACE
	printf("Executing expression statement.\n");
#endif
	run_expr(run, exps->expr, &rexpr);

	if (rexpr != NULL) {
		printf("Warning: Expression value ignored.\n");
	}
}

/** Run variable declaration statement. */
static void run_vdecl(run_t *run, stree_vdecl_t *vdecl)
{
	run_block_ar_t *block_ar;
	rdata_var_t *var, *old_var;
	rdata_int_t *int_v;

#ifdef DEBUG_RUN_TRACE
	printf("Executing variable declaration statement.\n");
#endif

	/* XXX Need to support other variables than int. */

	var = rdata_var_new(vc_int);
	int_v = rdata_int_new();

	var->u.int_v = int_v;
	int_v->value = 0;

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

/** Run @c if statement. */
static void run_if(run_t *run, stree_if_t *if_s)
{
	rdata_item_t *rcond;

#ifdef DEBUG_RUN_TRACE
	printf("Executing if statement.\n");
#endif
	run_expr(run, if_s->cond, &rcond);

	if (run_item_boolean_value(run, rcond) == b_true) {
#ifdef DEBUG_RUN_TRACE
		printf("Taking true path.\n");
#endif
		run_block(run, if_s->if_block);
	} else {
#ifdef DEBUG_RUN_TRACE
		printf("Taking false path.\n");
#endif
        	if (if_s->else_block != NULL)
			run_block(run, if_s->else_block);
	}

#ifdef DEBUG_RUN_TRACE
	printf("If statement terminated.\n");
#endif
}

/** Run @c while statement. */
static void run_while(run_t *run, stree_while_t *while_s)
{
	rdata_item_t *rcond;

#ifdef DEBUG_RUN_TRACE
	printf("Executing while statement.\n");
#endif
	run_expr(run, while_s->cond, &rcond);

	while (run_item_boolean_value(run, rcond) == b_true) {
		run_block(run, while_s->body);
		run_expr(run, while_s->cond, &rcond);

		if (run->thread_ar->bo_mode != bm_none)
			break;
	}

#ifdef DEBUG_RUN_TRACE
	printf("While statement terminated.\n");
#endif
}

/** Run @c raise statement. */
static void run_raise(run_t *run, stree_raise_t *raise_s)
{
	rdata_item_t *rexpr;
	rdata_item_t *rexpr_vi;

#ifdef DEBUG_RUN_TRACE
	printf("Executing raise statement.\n");
#endif
	run_expr(run, raise_s->expr, &rexpr);
	rdata_cvt_value_item(rexpr, &rexpr_vi);

	/* Store expression result in thread AR. */
	run->thread_ar->exc_payload = rexpr_vi->u.value;

	/* Start exception bailout. */
	run->thread_ar->bo_mode = bm_exc;
}

/** Run @c return statement. */
static void run_return(run_t *run, stree_return_t *return_s)
{
	rdata_item_t *rexpr;
	run_fun_ar_t *fun_ar;

#ifdef DEBUG_RUN_TRACE
	printf("Executing return statement.\n");
#endif
	run_expr(run, return_s->expr, &rexpr);

	/* Store expression result in function AR. */
	fun_ar = run_get_current_fun_ar(run);
	fun_ar->retval = rexpr;

	/* Force control to ascend and leave the function. */
	if (run->thread_ar->bo_mode == bm_none)
		run->thread_ar->bo_mode = bm_fun;
}

/** Run @c with-except-finally statement. */
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
 * matches except clause @c except_c. Generates an error if the exception
 * payload has invalid type (i.e. not an object).
 *
 * @param run		Runner object.
 * @param except_c	@c except clause.
 * @return		@c b_true if there is a match, @c b_false otherwise.
 */
static bool_t run_exc_match(run_t *run, stree_except_t *except_c)
{
	rdata_value_t *payload;
	rdata_var_t *payload_v;
	rdata_object_t *payload_o;
	rdata_titem_t *etype;

	payload = run->thread_ar->exc_payload;
	assert(payload != NULL);

	if (payload->var->vc != vc_ref) {
		printf("Error: Exception payload must be an object "
		    "(found type %d).\n", payload->var->vc);
		exit(1);
	}

	payload_v = payload->var->u.ref_v->vref;
	if (payload_v->vc != vc_object) {
		printf("Error: Exception payload must be an object "
		    "(found type %d).\n", payload_v->vc);
		exit(1);
	}

	payload_o = payload_v->u.object_v;

#ifdef DEBUG_RUN_TRACE
	printf("Active exception: '");
	symbol_print_fqn(run->program, payload_o->class_sym);
	printf("'.\n");
#endif
	assert(payload_o->class_sym != NULL);
	assert(payload_o->class_sym->sc == sc_csi);

	/* Evaluate type expression in except clause. */
	run_texpr(run, except_c->etype, &etype);

	return rdata_is_csi_derived_from_ti(payload_o->class_sym->u.csi,
	    etype);
}

/** Find a local variable in the currently active function. */
rdata_var_t *run_local_vars_lookup(run_t *run, sid_t name)
{
	run_fun_ar_t *fun_ar;
	run_block_ar_t *block_ar;
	rdata_var_t *var;
	list_node_t *node;

	fun_ar = run_get_current_fun_ar(run);
	node = list_last(&fun_ar->block_ar);

	/* Walk through all block activation records. */
	while (node != NULL) {
		block_ar = list_node_data(node, run_block_ar_t *);
		var = intmap_get(&block_ar->vars, name);
		if (var != NULL)
			return var;

		node = list_prev(&fun_ar->block_ar, node);
	}

	/* No match */
	return NULL;
}

/** Get current function activation record. */
run_fun_ar_t *run_get_current_fun_ar(run_t *run)
{
	list_node_t *node;

	node = list_last(&run->thread_ar->fun_ar);
	return list_node_data(node, run_fun_ar_t *);
}

/** Get current block activation record. */
run_block_ar_t *run_get_current_block_ar(run_t *run)
{
	run_fun_ar_t *fun_ar;
	list_node_t *node;

	fun_ar = run_get_current_fun_ar(run);

	node = list_last(&fun_ar->block_ar);
	return list_node_data(node, run_block_ar_t *);
}

/** Get current CSI. */
stree_csi_t *run_get_current_csi(run_t *run)
{
	run_fun_ar_t *fun_ar;

	fun_ar = run_get_current_fun_ar(run);
	return fun_ar->fun_sym->outer_csi;
}

/** Construct variable from a value item.
 *
 * XXX This should be in fact implemented using existing code as:
 *
 * (1) Create a variable of the desired type.
 * (2) Initialize the variable with the provided value.
 */
void run_value_item_to_var(rdata_item_t *item, rdata_var_t **var)
{
	rdata_int_t *int_v;
	rdata_string_t *string_v;
	rdata_ref_t *ref_v;
	rdata_var_t *in_var;

	assert(item->ic == ic_value);
	in_var = item->u.value->var;

	switch (in_var->vc) {
	case vc_int:
		*var = rdata_var_new(vc_int);
		int_v = rdata_int_new();

		(*var)->u.int_v = int_v;
		int_v->value = item->u.value->var->u.int_v->value;
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

/** Construct a function AR. */
void run_fun_ar_create(run_t *run, rdata_var_t *obj, stree_fun_t *fun,
    run_fun_ar_t **rfun_ar)
{
	run_fun_ar_t *fun_ar;

	(void) run;

	/* Create function activation record. */
	fun_ar = run_fun_ar_new();
	fun_ar->obj = obj;
	fun_ar->fun_sym = fun_to_symbol(fun);
	list_init(&fun_ar->block_ar);

	fun_ar->retval = NULL;

	*rfun_ar = fun_ar;
}

/** Fill arguments in a function AR. */
void run_fun_ar_set_args(run_t *run, run_fun_ar_t *fun_ar, list_t *args)
{
	stree_fun_t *fun;
	run_block_ar_t *block_ar;
	list_node_t *rarg_n, *farg_n;
	list_node_t *cn;
	rdata_item_t *rarg;
	stree_fun_arg_t *farg;
	rdata_var_t *var;
	rdata_var_t *ref_var;
	rdata_ref_t *ref;
	rdata_array_t *array;
	int n_vargs, idx;

	/* AR should have been created with run_fun_ar_create(). */
	assert(fun_ar->fun_sym != NULL);
	assert(list_is_empty(&fun_ar->block_ar));

	fun = symbol_to_fun(fun_ar->fun_sym);
	assert(fun != NULL);

	/* Create special block activation record to hold function arguments. */
	block_ar = run_block_ar_new();
	intmap_init(&block_ar->vars);
	list_append(&fun_ar->block_ar, block_ar);

	/* Declare local variables to hold argument values. */
	rarg_n = list_first(args);
	farg_n = list_first(&fun->args);

	while (farg_n != NULL) {
		if (rarg_n == NULL) {
			printf("Error: Too few arguments to function '");
			symbol_print_fqn(run->program, fun_ar->fun_sym);
			printf("'.\n");
			exit(1);
		}

		rarg = list_node_data(rarg_n, rdata_item_t *);
		farg = list_node_data(farg_n, stree_fun_arg_t *);

		assert(rarg->ic == ic_value);

		/* Construct a variable from the argument value. */
		run_value_item_to_var(rarg, &var);

		/* Declare variable using name of formal argument. */
		intmap_set(&block_ar->vars, farg->name->sid, var);

		rarg_n = list_next(args, rarg_n);
		farg_n = list_next(&fun->args, farg_n);
	}

	if (fun->varg != NULL) {
		/* Function is variadic. Count number of variadic arguments. */
		cn = rarg_n;
		n_vargs = 0;
		while (cn != NULL) {
			n_vargs += 1;
			cn = list_next(args, cn);
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

			rdata_var_write(array->element[idx], rarg->u.value);

			rarg_n = list_next(args, rarg_n);
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
		intmap_set(&block_ar->vars, fun->varg->name->sid,
		    ref_var);
	}

	/* Check for excess real parameters. */
	if (rarg_n != NULL) {
		printf("Error: Too many arguments to function '");
		symbol_print_fqn(run->program, fun_ar->fun_sym);
		printf("'.\n");
		exit(1);
	}
}

/** Print function activation backtrace. */
void run_print_fun_bt(run_t *run)
{
	list_node_t *node;
	run_fun_ar_t *fun_ar;

	printf("Backtrace:\n");
	node = list_last(&run->thread_ar->fun_ar);
	while (node != NULL) {
		printf(" * ");
		fun_ar = list_node_data(node, run_fun_ar_t *);
		symbol_print_fqn(run->program, fun_ar->fun_sym);
		printf("\n");

		node = list_prev(&run->thread_ar->fun_ar, node);
	}
}

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

run_fun_ar_t *run_fun_ar_new(void)
{
	run_fun_ar_t *fun_ar;

	fun_ar = calloc(1, sizeof(run_fun_ar_t));
	if (fun_ar == NULL) {
		printf("Memory allocation failed.\n");
		exit(1);
	}

	return fun_ar;
}

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
