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
#include "cspan.h"
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
static void stype_ctor(stype_t *stype, stree_ctor_t *ctor);
static void stype_ctor_body(stype_t *stype, stree_ctor_t *ctor);
static void stype_fun(stype_t *stype, stree_fun_t *fun);
static void stype_var(stype_t *stype, stree_var_t *var);
static void stype_prop(stype_t *stype, stree_prop_t *prop);

static void stype_fun_sig(stype_t *stype, stree_csi_t *outer_csi,
    stree_fun_sig_t *sig, tdata_fun_sig_t **rtsig);
static void stype_fun_body(stype_t *stype, stree_fun_t *fun);
static void stype_block(stype_t *stype, stree_block_t *block);

static void stype_class_impl_check(stype_t *stype, stree_csi_t *csi);
static void stype_class_impl_check_if(stype_t *stype, stree_csi_t *csi,
    tdata_item_t *iface_ti);
static void stype_class_impl_check_mbr(stype_t *stype, stree_csi_t *csi,
    tdata_tvv_t *if_tvv, stree_csimbr_t *ifmbr);
static void stype_class_impl_check_fun(stype_t *stype,
    stree_symbol_t *cfun_sym, tdata_tvv_t *if_tvv, stree_symbol_t *ifun_sym);
static void stype_class_impl_check_prop(stype_t *stype,
    stree_symbol_t *cprop_sym, tdata_tvv_t *if_tvv, stree_symbol_t *iprop_sym);

static void stype_vdecl(stype_t *stype, stree_vdecl_t *vdecl_s);
static void stype_if(stype_t *stype, stree_if_t *if_s);
static void stype_switch(stype_t *stype, stree_switch_t *switch_s);
static void stype_while(stype_t *stype, stree_while_t *while_s);
static void stype_for(stype_t *stype, stree_for_t *for_s);
static void stype_raise(stype_t *stype, stree_raise_t *raise_s);
static void stype_break(stype_t *stype, stree_break_t *break_s);
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
static stree_expr_t *stype_convert_tenum(stype_t *stype, stree_expr_t *expr,
    tdata_item_t *dest);
static stree_expr_t *stype_convert_tfun_tdeleg(stype_t *stype,
    stree_expr_t *expr, tdata_item_t *dest);
static stree_expr_t *stype_convert_tvref(stype_t *stype, stree_expr_t *expr,
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

		switch (mbr->mc) {
		case mc_csi:
			stype_csi(stype, mbr->u.csi);
			break;
		case mc_enum:
			stype_enum(stype, mbr->u.enum_d);
			break;
		}

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
		case csimbr_csi:
			stype_csi(stype, csimbr->u.csi);
			break;
		case csimbr_ctor:
			stype_ctor(stype, csimbr->u.ctor);
			break;
		case csimbr_deleg:
			stype_deleg(stype, csimbr->u.deleg);
			break;
		case csimbr_enum:
			stype_enum(stype, csimbr->u.enum_d);
			break;
		case csimbr_fun:
			stype_fun(stype, csimbr->u.fun);
			break;
		case csimbr_var:
			stype_var(stype, csimbr->u.var);
			break;
		case csimbr_prop:
			stype_prop(stype, csimbr->u.prop);
			break;
		}

		csimbr_n = list_next(&csi->members, csimbr_n);
	}

	if (csi->cc == csi_class)
		stype_class_impl_check(stype, csi);

	stype->current_csi = prev_ctx;
}

/** Type a constructor.
 *
 * @param stype		Static typing object.
 * @param ctor		Constructor to type.
 */
static void stype_ctor(stype_t *stype, stree_ctor_t *ctor)
{
#ifdef DEBUG_TYPE_TRACE
	printf("Type constructor '");
	symbol_print_fqn(ctor_to_symbol(ctor));
	printf("'.\n");
#endif
	if (ctor->titem == NULL)
		stype_ctor_header(stype, ctor);

	stype_ctor_body(stype, ctor);
}

/** Type constructor header.
 *
 * @param stype		Static typing object.
 * @param ctor		Constructor to type.
 */
void stype_ctor_header(stype_t *stype, stree_ctor_t *ctor)
{
	stree_symbol_t *ctor_sym;
	tdata_item_t *ctor_ti;
	tdata_fun_t *tfun;
	tdata_fun_sig_t *tsig;

#ifdef DEBUG_TYPE_TRACE
	printf("Type constructor '");
	symbol_print_fqn(ctor_to_symbol(ctor));
	printf("' header.\n");
#endif
	if (ctor->titem != NULL)
		return; /* Constructor header has already been typed. */

	ctor_sym = ctor_to_symbol(ctor);

	/* Type function signature. */
	stype_fun_sig(stype, ctor_sym->outer_csi, ctor->sig, &tsig);

	ctor_ti = tdata_item_new(tic_tfun);
	tfun = tdata_fun_new();
	ctor_ti->u.tfun = tfun;
	tfun->tsig = tsig;

	ctor->titem = ctor_ti;
}

/** Type constructor body.
 *
 * @param stype		Static typing object
 * @param ctor		Constructor
 */
static void stype_ctor_body(stype_t *stype, stree_ctor_t *ctor)
{
#ifdef DEBUG_TYPE_TRACE
	printf("Type constructor '");
	symbol_print_fqn(ctor_to_symbol(ctor));
	printf("' body.\n");
#endif
	assert(stype->proc_vr == NULL);

	stype->proc_vr = stype_proc_vr_new();
	stype->proc_vr->proc = ctor->proc;
	list_init(&stype->proc_vr->block_vr);

	stype_block(stype, ctor->proc->body);

	free(stype->proc_vr);
	stype->proc_vr = NULL;
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

/** Type enum.
 *
 * @param stype		Static typing object
 * @param enum_d	Enum to type
 */
void stype_enum(stype_t *stype, stree_enum_t *enum_d)
{
	tdata_item_t *titem;
	tdata_enum_t *tenum;

	(void) stype;

#ifdef DEBUG_TYPE_TRACE
	printf("Type enum '");
	symbol_print_fqn(enum_to_symbol(enum_d));
	printf("'.\n");
#endif
	if (enum_d->titem == NULL) {
		titem = tdata_item_new(tic_tenum);
		tenum = tdata_enum_new();
		titem->u.tenum = tenum;
		tenum->enum_d = enum_d;

		enum_d->titem = titem;
	}
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

	/* Declarations and builtin functions do not have a body. */
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

	run_texpr(stype->program, stype->current_csi, var->type,
	    &titem);
	if (titem->tic == tic_ignore) {
		/* An error occured. */
		stype_note_error(stype);
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
	if (prop->titem == NULL)
		stype_prop_header(stype, prop);

	stype->proc_vr = stype_proc_vr_new();
	list_init(&stype->proc_vr->block_vr);

	/* Property declarations do not have a getter body. */
	if (prop->getter != NULL && prop->getter->body != NULL) {
		stype->proc_vr->proc = prop->getter;
		stype_block(stype, prop->getter->body);
	}

	/* Property declarations do not have a setter body. */
	if (prop->setter != NULL && prop->setter->body != NULL) {
		stype->proc_vr->proc = prop->setter;
		stype_block(stype, prop->setter->body);
	}

	free(stype->proc_vr);
	stype->proc_vr = NULL;
}

/** Type property header.
 *
 * @param stype		Static typing object
 * @param prop		Property
 */
void stype_prop_header(stype_t *stype, stree_prop_t *prop)
{
	tdata_item_t *titem;

#ifdef DEBUG_TYPE_TRACE
	printf("Type property '");
	symbol_print_fqn(prop_to_symbol(prop));
	printf("' header.\n");
#endif
	run_texpr(stype->program, stype->current_csi, prop->type,
	    &titem);
	if (titem->tic == tic_ignore) {
		/* An error occured. */
		stype_note_error(stype);
		return;
	}

	prop->titem = titem;
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

/** Verify that class fully implements all interfaces as it claims.
 *
 * @param stype		Static typing object
 * @param csi		CSI to check
 */
static void stype_class_impl_check(stype_t *stype, stree_csi_t *csi)
{
	list_node_t *pred_n;
	stree_texpr_t *pred_te;
	tdata_item_t *pred_ti;

#ifdef DEBUG_TYPE_TRACE
	printf("Verify that class implements all interfaces.\n");
#endif
	assert(csi->cc == csi_class);

	pred_n = list_first(&csi->inherit);
	while (pred_n != NULL) {
		pred_te = list_node_data(pred_n, stree_texpr_t *);
		run_texpr(stype->program, csi, pred_te, &pred_ti);

		assert(pred_ti->tic == tic_tobject);
		switch (pred_ti->u.tobject->csi->cc) {
		case csi_class:
			break;
		case csi_struct:
			assert(b_false);
			/* Fallthrough */
		case csi_interface:
			/* Store to impl_if_ti for later use. */
			list_append(&csi->impl_if_ti, pred_ti);

			/* Check implementation of this interface. */
			stype_class_impl_check_if(stype, csi, pred_ti);
			break;
		}

		pred_n = list_next(&csi->inherit, pred_n);
	}
}

/** Verify that class fully implements an interface.
 *
 * @param stype		Static typing object
 * @param csi		CSI to check
 * @param iface		Interface that must be fully implemented
 */
static void stype_class_impl_check_if(stype_t *stype, stree_csi_t *csi,
    tdata_item_t *iface_ti)
{
	tdata_tvv_t *iface_tvv;
	list_node_t *pred_n;
	tdata_item_t *pred_ti;
	tdata_item_t *pred_sti;

	stree_csi_t *iface;
	list_node_t *ifmbr_n;
	stree_csimbr_t *ifmbr;

	assert(csi->cc == csi_class);

	assert(iface_ti->tic == tic_tobject);
	iface = iface_ti->u.tobject->csi;
	assert(iface->cc == csi_interface);

#ifdef DEBUG_TYPE_TRACE
	printf("Verify that class fully implements interface.\n");
#endif
	/* Compute TVV for this interface reference. */
	stype_titem_to_tvv(stype, iface_ti, &iface_tvv);

	/*
	 * Recurse to accumulated interfaces.
	 */
	pred_n = list_first(&iface->impl_if_ti);
	while (pred_n != NULL) {
		pred_ti = list_node_data(pred_n, tdata_item_t *);
		assert(pred_ti->tic == tic_tobject);
		assert(pred_ti->u.tobject->csi->cc == csi_interface);

		/* Substitute real type parameters to predecessor reference. */
		tdata_item_subst(pred_ti, iface_tvv, &pred_sti);

		/* Check accumulated interface. */
		stype_class_impl_check_if(stype, csi, pred_sti);

		pred_n = list_next(&iface->impl_if_ti, pred_n);
	}

	/*
	 * Check all interface members.
	 */
	ifmbr_n = list_first(&iface->members);
	while (ifmbr_n != NULL) {
		ifmbr = list_node_data(ifmbr_n, stree_csimbr_t *);
		stype_class_impl_check_mbr(stype, csi, iface_tvv, ifmbr);

		ifmbr_n = list_next(&iface->members, ifmbr_n);
	}
}

/** Verify that class fully implements an interface member.
 *
 * @param stype		Static typing object
 * @param csi		CSI to check
 * @param if_tvv	TVV for @a ifmbr
 * @param ifmbr		Interface that must be fully implemented
 */
static void stype_class_impl_check_mbr(stype_t *stype, stree_csi_t *csi,
    tdata_tvv_t *if_tvv, stree_csimbr_t *ifmbr)
{
	stree_symbol_t *cmbr_sym;
	stree_symbol_t *ifmbr_sym;
	stree_ident_t *ifmbr_name;

	assert(csi->cc == csi_class);

#ifdef DEBUG_TYPE_TRACE
	printf("Verify that class implements interface member.\n");
#endif
	ifmbr_name = stree_csimbr_get_name(ifmbr);

	cmbr_sym = symbol_search_csi(stype->program, csi, ifmbr_name);
	if (cmbr_sym == NULL) {
		printf("Error: CSI '");
		symbol_print_fqn(csi_to_symbol(csi));
		printf("' should implement '");
		symbol_print_fqn(csimbr_to_symbol(ifmbr));
		printf("' but it does not.\n");
		stype_note_error(stype);
		return;
	}

	ifmbr_sym = csimbr_to_symbol(ifmbr);
	if (cmbr_sym->sc != ifmbr_sym->sc) {
		printf("Error: CSI '");
		symbol_print_fqn(csi_to_symbol(csi));
		printf("' implements '");
		symbol_print_fqn(csimbr_to_symbol(ifmbr));
		printf("' as a different kind of symbol.\n");
		stype_note_error(stype);
	}

	switch (cmbr_sym->sc) {
	case sc_csi:
	case sc_ctor:
	case sc_deleg:
	case sc_enum:
		/*
		 * Checked at parse time. Interface should not have these
		 * member types.
		 */
		assert(b_false);
		/* Fallthrough */
	case sc_fun:
		stype_class_impl_check_fun(stype, cmbr_sym, if_tvv, ifmbr_sym);
		break;
	case sc_var:
		/*
		 * Checked at parse time. Interface should not have these
		 * member types.
		 */
		assert(b_false);
		/* Fallthrough */
	case sc_prop:
		stype_class_impl_check_prop(stype, cmbr_sym, if_tvv, ifmbr_sym);
		break;
	}
}

/** Verify that class properly implements a function from an interface.
 *
 * @param stype		Static typing object
 * @param cfun_sym	Function symbol in class
 * @param if_tvv	TVV for @a ifun_sym
 * @param ifun_sym	Function declaration symbol in interface
 */
static void stype_class_impl_check_fun(stype_t *stype,
    stree_symbol_t *cfun_sym, tdata_tvv_t *if_tvv, stree_symbol_t *ifun_sym)
{
	stree_fun_t *cfun;
	tdata_fun_t *tcfun;
	stree_fun_t *ifun;
	tdata_item_t *sifun_ti;
	tdata_fun_t *tifun;

#ifdef DEBUG_TYPE_TRACE
	printf("Verify that class '");
	symbol_print_fqn(csi_to_symbol(cfun_sym->outer_csi));
	printf("' implements function '");
	symbol_print_fqn(ifun_sym);
	printf("' properly.\n");
#endif
	assert(cfun_sym->sc == sc_fun);
	cfun = cfun_sym->u.fun;

	assert(ifun_sym->sc == sc_fun);
	ifun = ifun_sym->u.fun;

	assert(cfun->titem->tic == tic_tfun);
	tcfun = cfun->titem->u.tfun;

	tdata_item_subst(ifun->titem, if_tvv, &sifun_ti);
	assert(sifun_ti->tic == tic_tfun);
	tifun = sifun_ti->u.tfun;

	if (!stype_fun_sig_equal(stype, tcfun->tsig, tifun->tsig)) {
		cspan_print(cfun->name->cspan);
		printf(" Error: Type of function '");
		symbol_print_fqn(cfun_sym);
		printf("' (");
		tdata_item_print(cfun->titem);
		printf(") does not match type of '");
		symbol_print_fqn(ifun_sym);
		printf("' (");
		tdata_item_print(sifun_ti);
		printf(") which it should implement.\n");
		stype_note_error(stype);
	}
}

/** Verify that class properly implements a function from an interface.
 *
 * @param stype		Static typing object
 * @param cprop_sym	Property symbol in class
 * @param if_tvv	TVV for @a ifun_sym
 * @param iprop_sym	Property declaration symbol in interface
 */
static void stype_class_impl_check_prop(stype_t *stype,
    stree_symbol_t *cprop_sym, tdata_tvv_t *if_tvv, stree_symbol_t *iprop_sym)
{
	stree_prop_t *cprop;
	stree_prop_t *iprop;
	tdata_item_t *siprop_ti;

#ifdef DEBUG_TYPE_TRACE
	printf("Verify that class '");
	symbol_print_fqn(csi_to_symbol(cprop_sym->outer_csi));
	printf("' implements property '");
	symbol_print_fqn(iprop_sym);
	printf("' properly.\n");
#endif
	assert(cprop_sym->sc == sc_prop);
	cprop = cprop_sym->u.prop;

	assert(iprop_sym->sc == sc_prop);
	iprop = iprop_sym->u.prop;

	tdata_item_subst(iprop->titem, if_tvv, &siprop_ti);

	if (!tdata_item_equal(cprop->titem, siprop_ti)) {
		cspan_print(cprop->name->cspan);
		printf(" Error: Type of property '");
		symbol_print_fqn(cprop_sym);
		printf("' (");
		tdata_item_print(cprop->titem);
		printf(") does not match type of '");
		symbol_print_fqn(iprop_sym);
		printf("' (");
		tdata_item_print(siprop_ti);
		printf(") which it should implement.\n");
		stype_note_error(stype);
	}

	if (iprop->getter != NULL && cprop->getter == NULL) {
		cspan_print(cprop->name->cspan);
		printf(" Error: Property '");
		symbol_print_fqn(cprop_sym);
		printf("' is missing a getter, which is required by '");
		symbol_print_fqn(iprop_sym);
		printf("'.\n");
		stype_note_error(stype);
	}

	if (iprop->setter != NULL && cprop->setter == NULL) {
		cspan_print(cprop->name->cspan);
		printf(" Error: Property '");
		symbol_print_fqn(cprop_sym);
		printf("' is missing a setter, which is required by '");
		symbol_print_fqn(iprop_sym);
		printf("'.\n");
		stype_note_error(stype);
	}
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
	case st_vdecl:
		stype_vdecl(stype, stat->u.vdecl_s);
		break;
	case st_if:
		stype_if(stype, stat->u.if_s);
		break;
	case st_switch:
		stype_switch(stype, stat->u.switch_s);
		break;
	case st_while:
		stype_while(stype, stat->u.while_s);
		break;
	case st_for:
		stype_for(stype, stat->u.for_s);
		break;
	case st_raise:
		stype_raise(stype, stat->u.raise_s);
		break;
	case st_break:
		stype_break(stype, stat->u.break_s);
		break;
	case st_return:
		stype_return(stype, stat->u.return_s);
		break;
	case st_exps:
		stype_exps(stype, stat->u.exp_s, want_value);
		break;
	case st_wef:
		stype_wef(stype, stat->u.wef_s);
		break;
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

	/* Annotate with variable type */
	vdecl_s->titem = titem;

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
	list_node_t *ifc_node;
	stree_if_clause_t *ifc;

#ifdef DEBUG_TYPE_TRACE
	printf("Type 'if' statement.\n");
#endif
	ifc_node = list_first(&if_s->if_clauses);

	/* Walk through all if/elif clauses. */

	while (ifc_node != NULL) {
		/* Get if/elif clause */
		ifc = list_node_data(ifc_node, stree_if_clause_t *);

		/* Convert condition to boolean type. */
		stype_expr(stype, ifc->cond);
		ccond = stype_convert(stype, ifc->cond,
		    stype_boolean_titem(stype));

		/* Patch code with augmented expression. */
		ifc->cond = ccond;

		/* Type the @c if/elif block */
		stype_block(stype, ifc->block);

		ifc_node = list_next(&if_s->if_clauses, ifc_node);
	}

	/* Type the @c else block */
	if (if_s->else_block != NULL)
		stype_block(stype, if_s->else_block);
}

/** Type @c switch statement.
 *
 * @param stype		Static typing object
 * @param switch_s	@c switch statement
 */
static void stype_switch(stype_t *stype, stree_switch_t *switch_s)
{
	stree_expr_t *expr, *cexpr;
	list_node_t *whenc_node;
	stree_when_t *whenc;
	list_node_t *expr_node;
	tdata_item_t *titem1, *titem2;

#ifdef DEBUG_TYPE_TRACE
	printf("Type 'switch' statement.\n");
#endif
	stype_expr(stype, switch_s->expr);

	titem1 = switch_s->expr->titem;
	if (titem1 == NULL) {
		cspan_print(switch_s->expr->cspan);
		printf(" Error: Switch expression has no value.\n");
		stype_note_error(stype);
		return;
	}

	/* Walk through all when clauses. */
	whenc_node = list_first(&switch_s->when_clauses);

	while (whenc_node != NULL) {
		/* Get when clause */
		whenc = list_node_data(whenc_node, stree_when_t *);

		/* Walk through all expressions of the when clause */
		expr_node = list_first(&whenc->exprs);
		while (expr_node != NULL) {
			expr = list_node_data(expr_node, stree_expr_t *);

			stype_expr(stype, expr);
			titem2 = expr->titem;
			if (titem2 == NULL) {
				cspan_print(expr->cspan);
				printf(" Error: When expression has no value.\n");
				stype_note_error(stype);
				return;
			}

			/* Convert expression to same type as switch expr. */
			cexpr = stype_convert(stype, expr, titem1);

			/* Patch code with augmented expression. */
			list_node_setdata(expr_node, cexpr);

			expr_node = list_next(&whenc->exprs, expr_node);
		}

		/* Type the @c when block */
		stype_block(stype, whenc->block);

		whenc_node = list_next(&switch_s->when_clauses, whenc_node);
	}

	/* Type the @c else block */
	if (switch_s->else_block != NULL)
		stype_block(stype, switch_s->else_block);
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

	/* While is a breakable statement. Increment counter. */
	stype->proc_vr->bstat_cnt += 1;

	/* Type the body of the loop */
	stype_block(stype, while_s->body);

	stype->proc_vr->bstat_cnt -= 1;
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
	/* For is a breakable statement. Increment counter. */
	stype->proc_vr->bstat_cnt += 1;

	stype_block(stype, for_s->body);

	stype->proc_vr->bstat_cnt -= 1;
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

/** Type @c break statement */
static void stype_break(stype_t *stype, stree_break_t *break_s)
{
#ifdef DEBUG_TYPE_TRACE
	printf("Type 'break' statement.\n");
#endif
	(void) break_s;

	/* Check whether there is an active statement to break from. */
	if (stype->proc_vr->bstat_cnt == 0) {
		printf("Error: Break statement outside of while or for.\n");
		stype_note_error(stype);
	}
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
	if (return_s->expr != NULL)
		stype_expr(stype, return_s->expr);

	/* Determine the type we need to return. */

	outer_sym = stype->proc_vr->proc->outer_symbol;
	switch (outer_sym->sc) {
	case sc_fun:
		fun = symbol_to_fun(outer_sym);
		assert(fun != NULL);

		/* XXX Memoize to avoid recomputing. */
		if (fun->sig->rtype != NULL) {
			run_texpr(stype->program, outer_sym->outer_csi,
			    fun->sig->rtype, &dtype);

			if (return_s->expr == NULL) {
				printf("Error: Return without a value in "
				    "function returning value.\n");
				stype_note_error(stype);
			}
		} else {
			dtype = NULL;

			if (return_s->expr != NULL) {
				printf("Error: Return with a value in "
				    "value-less function.\n");
				stype_note_error(stype);
			}
		}
		break;
	case sc_prop:
		prop = symbol_to_prop(outer_sym);
		assert(prop != NULL);

		if (stype->proc_vr->proc == prop->getter) {
			if (return_s->expr == NULL) {
				printf("Error: Return without a value in "
				    "getter.\n");
				stype_note_error(stype);
			}
		} else {
			if (return_s->expr == NULL) {
				printf("Error: Return with a value in "
				    "setter.\n");
				stype_note_error(stype);
			}
		}

		/* XXX Memoize to avoid recomputing. */
		run_texpr(stype->program, outer_sym->outer_csi, prop->type,
		    &dtype);
		break;
	default:
		assert(b_false);
	}

	if (dtype != NULL && return_s->expr != NULL) {
		/* Convert to the return type. */
		cexpr = stype_convert(stype, return_s->expr, dtype);

		/* Patch code with the augmented expression. */
		return_s->expr = cexpr;
	}
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

	if (want_value == b_false && exp_s->expr->titem != NULL) {
		cspan_print(exp_s->expr->cspan);
		printf(" Warning: Expression value ignored.\n");
	}
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
		run_texpr(stype->program, stype->current_csi, ec->etype,
		    &ec->titem);
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
		cspan_print(expr->cspan);
		printf(" Error: Conversion source is not valid.\n");
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

	if (src->tic == tic_tebase) {
		stype_convert_failure(stype, convc_implicit, expr, dest);
		printf("Invalid use of reference to enum type in "
		    "expression.\n");
		return expr;
	}

	if (src->tic != dest->tic) {
		stype_convert_failure(stype, convc_implicit, expr, dest);
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
	case tic_tebase:
		/* Conversion destination should never be enum-base */
		assert(b_false);
		/* Fallthrough */
	case tic_tenum:
		expr = stype_convert_tenum(stype, expr, dest);
		break;
	case tic_tfun:
		assert(b_false);
		/* Fallthrough */
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
		stype_convert_failure(stype, convc_implicit, expr, dest);

	return expr;
}

/** Convert expression of primitive type to object type.
 *
 * This function implements autoboxing. It modifies the code by inserting
 * the boxing operation.
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

	/* Make compiler happy. */
	bp_sym = NULL;

	switch (src->u.tprimitive->tpc) {
	case tpc_bool:
		bp_sym = bi->boxed_bool;
		break;
	case tpc_char:
		bp_sym = bi->boxed_char;
		break;
	case tpc_int:
		bp_sym = bi->boxed_int;
		break;
	case tpc_nil:
		assert(b_false);
		/* Fallthrough */
	case tpc_string:
		bp_sym = bi->boxed_string;
		break;
	case tpc_resource:
		stype_convert_failure(stype, convc_implicit, expr, dest);
		return expr;
	}

	/* Target type must be boxed @a src or Object */
	if (csi_sym != bp_sym && csi_sym != bi->gf_class)
		stype_convert_failure(stype, convc_implicit, expr, dest);

	/* Patch the code to box the primitive value */
	box = stree_box_new();
	box->arg = expr;
	bexpr = stree_expr_new(ec_box);
	bexpr->u.box = box;
	bexpr->titem = dest;

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
	tdata_item_t *pred_ti;

#ifdef DEBUG_TYPE_TRACE
	printf("Convert object type.\n");
#endif
	src = expr->titem;
	assert(src->tic == tic_tobject);
	assert(dest->tic == tic_tobject);

	/*
	 * Find predecessor of the right type. This determines the
	 * type arguments that the destination type should have.
	 */
	pred_ti = stype_tobject_find_pred(stype, src, dest);
	if (pred_ti == NULL) {
		stype_convert_failure(stype, convc_implicit, expr, dest);
		printf("Not a base class or implemented or accumulated "
		    "interface.\n");
		return expr;
	}

	/*
	 * Verify that type arguments match with those specified for
	 * conversion destination.
	 */
	if (stype_targs_check_equal(stype, pred_ti, dest) != EOK) {
		stype_convert_failure(stype, convc_implicit, expr, dest);
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
		stype_convert_failure(stype, convc_implicit, expr, dest);
		return expr;
	}

	/* XXX Should we convert each element? */
	if (tdata_item_equal(src->u.tarray->base_ti,
	    dest->u.tarray->base_ti) != b_true) {
		stype_convert_failure(stype, convc_implicit, expr, dest);
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
		stype_convert_failure(stype, convc_implicit, expr, dest);
		return expr;
	}

	return expr;
}

/** Convert expression of enum type to enum type.
 *
 * @param stype		Static typing object
 * @param expr		Expression
 * @param dest		Destination type
 */
static stree_expr_t *stype_convert_tenum(stype_t *stype, stree_expr_t *expr,
    tdata_item_t *dest)
{
	tdata_item_t *src;
	tdata_enum_t *senum, *denum;

#ifdef DEBUG_TYPE_TRACE
	printf("Convert enum type.\n");
#endif
	src = expr->titem;
	assert(src->tic == tic_tenum);
	assert(dest->tic == tic_tenum);

	senum = src->u.tenum;
	denum = dest->u.tenum;

	/*
	 * XXX How should enum types interact with generics?
	 */

	/* Both must be of the same enum type (with the same declaration). */
	if (senum->enum_d != denum->enum_d) {
		stype_convert_failure(stype, convc_implicit, expr, dest);
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
		stype_convert_failure(stype, convc_implicit, expr, dest);
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
		stype_convert_failure(stype, convc_implicit, expr, dest);
		return expr;
	}

	return expr;
}

/** Display conversion error message and note error.
 *
 * @param stype		Static typing object
 * @param expr		Original expression
 * @param dest		Destination type
 */
void stype_convert_failure(stype_t *stype, stype_conv_class_t convc,
    stree_expr_t *expr, tdata_item_t *dest)
{
	cspan_print(expr->cspan);
	printf(" Error: ");
	switch (convc) {
	case convc_implicit:
		printf("Cannot implicitly convert '");
		break;
	case convc_as:
		printf("Cannot use 'as' to convert '");
		break;
	}

	tdata_item_print(expr->titem);
	printf(" to ");
	tdata_item_print(dest);
	printf(".\n");

	stype_note_error(stype);
}

/** Box value.
 *
 * This function implements implicit boxing. It modifies the code by inserting
 * the boxing operation.
 *
 * @param stype		Static typing object
 * @param expr		Expression
 * @return		Modified expression.
 */
stree_expr_t *stype_box_expr(stype_t *stype, stree_expr_t *expr)
{
	tdata_item_t *src;
	builtin_t *bi;
	stree_symbol_t *bp_sym;
	stree_box_t *box;
	stree_expr_t *bexpr;
	tdata_object_t *tobject;

#ifdef DEBUG_TYPE_TRACE
	printf("Boxing.\n");
#endif
	src = expr->titem;
	assert(src->tic == tic_tprimitive);

	bi = stype->program->builtin;

	/* Make compiler happy. */
	bp_sym = NULL;

	switch (src->u.tprimitive->tpc) {
	case tpc_bool:
		bp_sym = bi->boxed_bool;
		break;
	case tpc_char:
		bp_sym = bi->boxed_char;
		break;
	case tpc_int:
		bp_sym = bi->boxed_int;
		break;
	case tpc_nil:
		assert(b_false);
		/* Fallthrough */
	case tpc_string:
		bp_sym = bi->boxed_string;
		break;
	case tpc_resource:
		cspan_print(expr->cspan);
		printf(" Error: Cannot use ");
		tdata_item_print(expr->titem);
		printf(" as an object.\n");

		stype_note_error(stype);
		return expr;
	}

	/* Patch the code to box the primitive value */
	box = stree_box_new();
	box->arg = expr;
	bexpr = stree_expr_new(ec_box);
	bexpr->u.box = box;
	bexpr->titem = tdata_item_new(tic_tobject);
	tobject = tdata_object_new();
	bexpr->titem->u.tobject = tobject;

	tobject->csi = symbol_to_csi(bp_sym);
	assert(tobject->csi != NULL);

	return bexpr;
}

/** Find predecessor CSI and return its type item.
 *
 * Looks for predecessor of CSI type @a src that matches @a dest.
 * The type maches if they use the same generic CSI definition, type
 * arguments are ignored. If found, returns the type arguments that
 * @a dest should have in order to be a true predecessor of @a src.
 *
 * @param stype		Static typing object
 * @param src		Source type
 * @param dest		Destination type
 * @return		Type matching @a dest with correct type arguments
 */
tdata_item_t *stype_tobject_find_pred(stype_t *stype, tdata_item_t *src,
    tdata_item_t *dest)
{
	stree_csi_t *src_csi;
	tdata_tvv_t *tvv;
	tdata_item_t *b_ti, *bs_ti;

	list_node_t *pred_n;
	stree_texpr_t *pred_te;

	tdata_item_t *res_ti;

#ifdef DEBUG_TYPE_TRACE
	printf("Find CSI predecessor.\n");
#endif
	assert(src->tic == tic_tobject);
	assert(dest->tic == tic_tobject);

	if (src->u.tobject->csi == dest->u.tobject->csi)
		return src;

	src_csi = src->u.tobject->csi;
	stype_titem_to_tvv(stype, src, &tvv);

	res_ti = NULL;

	switch (dest->u.tobject->csi->cc) {
	case csi_class:
		/* Destination is a class. Look at base class. */
		pred_te = symbol_get_base_class_ref(stype->program,
		    src_csi);
		if (pred_te != NULL) {
			run_texpr(stype->program, src_csi, pred_te,
			    &b_ti);
		} else if (src_csi->base_csi != NULL &&
		    src->u.tobject->csi->cc == csi_class) {
			/* No explicit reference. Use grandfather class. */
			b_ti = tdata_item_new(tic_tobject);
			b_ti->u.tobject = tdata_object_new();
			b_ti->u.tobject->csi = src_csi->base_csi;
			b_ti->u.tobject->static_ref = sn_nonstatic;

			list_init(&b_ti->u.tobject->targs);
		} else {
			/* No match */
			return NULL;
		}

		/* Substitute type variables to get predecessor type. */
		tdata_item_subst(b_ti, tvv, &bs_ti);
		assert(bs_ti->tic == tic_tobject);

		/* Recurse to compute the rest of the path. */
		res_ti = stype_tobject_find_pred(stype, bs_ti, dest);
		if (b_ti->tic == tic_ignore) {
			/* An error occured. */
			return NULL;
		}
		break;
	case csi_struct:
		assert(b_false);
		/* Fallthrough */
	case csi_interface:
		/*
		 * Destination is an interface. Look at implemented
		 * or accumulated interfaces.
		 */
		pred_n = list_first(&src_csi->inherit);
		while (pred_n != NULL) {
			pred_te = list_node_data(pred_n, stree_texpr_t *);
			run_texpr(stype->program, src_csi, pred_te,
			    &b_ti);

			/* Substitute type variables to get predecessor type. */
			tdata_item_subst(b_ti, tvv, &bs_ti);
			assert(bs_ti->tic == tic_tobject);

			/* Recurse to compute the rest of the path. */
			res_ti = stype_tobject_find_pred(stype, bs_ti, dest);
			if (res_ti != NULL)
				break;

			pred_n = list_next(&src_csi->inherit, pred_n);
		}
		break;
	}

	return res_ti;
}

/** Check whether type arguments of expression type and another type are equal.
 *
 * Compare type arguments of the type of @a expr and of type @a b_ti.
 * @a convc denotes the type of conversion for which we perform this check
 * (for sake of error reporting).
 *
 * If the type arguments are not equal a typing error and a conversion error
 * message is generated.
 *
 * @param stype		Static typing object
 * @param expr		Expression
 * @param b_ti		b_tiination type
 * @return		EOK if equal, EINVAL if not.
 */
errno_t stype_targs_check_equal(stype_t *stype, tdata_item_t *a_ti,
    tdata_item_t *b_ti)
{
	list_node_t *arg_a_n, *arg_b_n;
	tdata_item_t *arg_a, *arg_b;

	(void) stype;

#ifdef DEBUG_TYPE_TRACE
	printf("Check if type arguments match.\n");
#endif
	assert(a_ti->tic == tic_tobject);
	assert(b_ti->tic == tic_tobject);

	/*
	 * Verify that type arguments match with those specified for
	 * conversion b_tiination.
	 */
	arg_a_n = list_first(&a_ti->u.tobject->targs);
	arg_b_n = list_first(&b_ti->u.tobject->targs);

	while (arg_a_n != NULL && arg_b_n != NULL) {
		arg_a = list_node_data(arg_a_n, tdata_item_t *);
		arg_b = list_node_data(arg_b_n, tdata_item_t *);

		if (tdata_item_equal(arg_a, arg_b) != b_true) {
			/* Diferent argument type */
			printf("Different argument type '");
			tdata_item_print(arg_a);
			printf("' vs. '");
			tdata_item_print(arg_b);
			printf("'.\n");
			return EINVAL;
		}

		arg_a_n = list_next(&a_ti->u.tobject->targs, arg_a_n);
		arg_b_n = list_next(&b_ti->u.tobject->targs, arg_b_n);
	}

	if (arg_a_n != NULL || arg_b_n != NULL) {
		/* Diferent number of arguments */
		printf("Different number of arguments.\n");
		return EINVAL;
	}

	return EOK;
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

	if (asig->rtype != NULL || bsig->rtype != NULL) {
		if (asig->rtype == NULL ||
		    bsig->rtype == NULL) {
			return b_false;
		}

		if (!tdata_item_equal(asig->rtype, bsig->rtype)) {
			return b_false;
		}
	}

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
	stree_ctor_t *ctor;
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

	/* Make compiler happy. */
	args = NULL;
	varg = NULL;

	switch (outer_sym->sc) {
	case sc_ctor:
		ctor = symbol_to_ctor(outer_sym);
		assert(ctor != NULL);
		args = &ctor->sig->args;
		varg = ctor->sig->varg;
		break;
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
	case sc_csi:
	case sc_deleg:
	case sc_enum:
	case sc_var:
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
