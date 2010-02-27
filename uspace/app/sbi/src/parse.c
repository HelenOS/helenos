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

/** @file Parser
 *
 * Consumes a sequence of lexical elements and produces a syntax tree (stree).
 * Parsing primitives, module members, statements.
 */

#include <assert.h>
#include <stdlib.h>
#include "lex.h"
#include "list.h"
#include "mytypes.h"
#include "p_expr.h"
#include "p_type.h"
#include "stree.h"
#include "strtab.h"

#include "parse.h"

/*
 * Module members
 */
static stree_csi_t *parse_csi(parse_t *parse, lclass_t dclass);
static stree_csimbr_t *parse_csimbr(parse_t *parse, stree_csi_t *outer_csi);

static stree_fun_t *parse_fun(parse_t *parse);
static stree_var_t *parse_var(parse_t *parse);
static stree_prop_t *parse_prop(parse_t *parse);

static stree_fun_arg_t *parse_fun_arg(parse_t *parse);

/*
 * Statements
 */
static stree_block_t *parse_block(parse_t *parse);
static stree_stat_t *parse_stat(parse_t *parse);

static stree_vdecl_t *parse_vdecl(parse_t *parse);
static stree_if_t *parse_if(parse_t *parse);
static stree_while_t *parse_while(parse_t *parse);
static stree_for_t *parse_for(parse_t *parse);
static stree_raise_t *parse_raise(parse_t *parse);
static stree_return_t *parse_return(parse_t *parse);
static stree_wef_t *parse_wef(parse_t *parse);
static stree_exps_t *parse_exps(parse_t *parse);

void parse_init(parse_t *parse, stree_program_t *prog, struct lex *lex)
{
	parse->program = prog;
	parse->cur_mod = parse->program->module;
	parse->lex = lex;
	lex_next(parse->lex);
}

/** Parse module. */
void parse_module(parse_t *parse)
{
	stree_csi_t *csi;
	stree_modm_t *modm;
	stree_symbol_t *symbol;

/*	do {
		lex_next(parse->lex);
		printf("Read token: "); lem_print(&parse->lex->current);
		putchar('\n');
	} while (parse->lex->current.lclass != lc_eof);
*/
	while (lcur_lc(parse) != lc_eof) {
		switch (lcur_lc(parse)) {
		case lc_class:
		case lc_struct:
		case lc_interface:
			csi = parse_csi(parse, lcur_lc(parse));
			modm = stree_modm_new(mc_csi);
			modm->u.csi = csi;

			symbol = stree_symbol_new(sc_csi);
			symbol->u.csi = csi;
			symbol->outer_csi = NULL;
			csi->symbol = symbol;

			list_append(&parse->cur_mod->members, modm);
			break;
		default:
			lunexpected_error(parse);
			lex_next(parse->lex);
			break;
		}

	}
}

/** Parse class, struct or interface declaration. */
static stree_csi_t *parse_csi(parse_t *parse, lclass_t dclass)
{
	stree_csi_t *csi;
	csi_class_t cc;
	stree_csimbr_t *csimbr;

	switch (dclass) {
	case lc_class: cc = csi_class; break;
	case lc_struct: cc = csi_struct; break;
	case lc_interface: cc = csi_interface; break;
	default: assert(b_false);
	}

	lskip(parse);

	csi = stree_csi_new(cc);
	csi->name = parse_ident(parse);
/*
	printf("parse_csi: csi=%p, csi->name = %p (%s)\n", csi, csi->name,
	    strtab_get_str(csi->name->sid));
*/
	if (lcur_lc(parse) == lc_colon) {
		/* Inheritance list */
		lskip(parse);
		csi->base_csi_ref = parse_texpr(parse);
	} else {
		csi->base_csi_ref = NULL;
	}

	lmatch(parse, lc_is);
	list_init(&csi->members);

	/* Parse class, struct or interface members. */
	while (lcur_lc(parse) != lc_end) {
		csimbr = parse_csimbr(parse, csi);
		list_append(&csi->members, csimbr);
	}

	lmatch(parse, lc_end);

	return csi;
}

/** Parse class, struct or interface member. */
static stree_csimbr_t *parse_csimbr(parse_t *parse, stree_csi_t *outer_csi)
{
	stree_csimbr_t *csimbr;

	stree_csi_t *csi;
	stree_fun_t *fun;
	stree_var_t *var;
	stree_prop_t *prop;

	stree_symbol_t *symbol;

	switch (lcur_lc(parse)) {
	case lc_class:
	case lc_struct:
	case lc_interface:
		csi = parse_csi(parse, lcur_lc(parse));
		csimbr = stree_csimbr_new(csimbr_csi);
		csimbr->u.csi = csi;

		symbol = stree_symbol_new(sc_csi);
		symbol->u.csi = csi;
		symbol->outer_csi = outer_csi;
		csi->symbol = symbol;
		break;
	case lc_fun:
		fun = parse_fun(parse);
		csimbr = stree_csimbr_new(csimbr_fun);
		csimbr->u.fun = fun;

		symbol = stree_symbol_new(sc_fun);
		symbol->u.fun = fun;
		symbol->outer_csi = outer_csi;
		fun->symbol = symbol;
		break;
	case lc_var:
		var = parse_var(parse);
		csimbr = stree_csimbr_new(csimbr_var);
		csimbr->u.var = var;

		symbol = stree_symbol_new(sc_var);
		symbol->u.var = var;
		symbol->outer_csi = outer_csi;
		var->symbol = symbol;
		break;
	case lc_prop:
		prop = parse_prop(parse);
		csimbr = stree_csimbr_new(csimbr_prop);
		csimbr->u.prop = prop;

		symbol = stree_symbol_new(sc_prop);
		symbol->u.prop = prop;
		symbol->outer_csi = outer_csi;
		prop->symbol = symbol;
		break;
	default:
		lunexpected_error(parse);
		lex_next(parse->lex);
	}

	return csimbr;
}


/** Parse member function. */
static stree_fun_t *parse_fun(parse_t *parse)
{
	stree_fun_t *fun;
	stree_fun_arg_t *arg;

	fun = stree_fun_new();

	lmatch(parse, lc_fun);
	fun->name = parse_ident(parse);
	lmatch(parse, lc_lparen);

	list_init(&fun->args);

	if (lcur_lc(parse) != lc_rparen) {

		/* Parse formal parameters. */
		while (b_true) {
			arg = parse_fun_arg(parse);
			list_append(&fun->args, arg);

			if (lcur_lc(parse) == lc_rparen)
				break;

			lmatch(parse, lc_scolon);
		}
	}

	lmatch(parse, lc_rparen);

	if (lcur_lc(parse) == lc_colon) {
	    	lskip(parse);
		fun->rtype = parse_texpr(parse);
	} else {
		fun->rtype = NULL;
	}

	lmatch(parse, lc_is);
	fun->body = parse_block(parse);
	lmatch(parse, lc_end);

	return fun;
}

/** Parse member variable. */
static stree_var_t *parse_var(parse_t *parse)
{
	stree_var_t *var;

	var = stree_var_new();

	lmatch(parse, lc_var);
	var->name = parse_ident(parse);
	lmatch(parse, lc_colon);
	var->type =  parse_texpr(parse);
	lmatch(parse, lc_scolon);

	return var;
}

/** Parse member property. */
static stree_prop_t *parse_prop(parse_t *parse)
{
	stree_prop_t *prop;

	prop = stree_prop_new();

	lmatch(parse, lc_prop);
	prop->name = parse_ident(parse);
	lmatch(parse, lc_colon);
	prop->type = parse_texpr(parse);
	lmatch(parse, lc_is);
	lmatch(parse, lc_end);

	return prop;
}

/** Parse formal function argument. */
static stree_fun_arg_t *parse_fun_arg(parse_t *parse)
{
	stree_fun_arg_t *arg;

	arg = stree_fun_arg_new();
	arg->name = parse_ident(parse);
	lmatch(parse, lc_colon);
	arg->type = parse_texpr(parse);

	return arg;
}

/** Parse statement block. */
static stree_block_t *parse_block(parse_t *parse)
{
	stree_block_t *block;
	stree_stat_t *stat;

	block = stree_block_new();
	list_init(&block->stats);

	while (terminates_block(lcur_lc(parse)) != b_true) {
		stat = parse_stat(parse);
		list_append(&block->stats, stat);
	}

	return block;
}

/** Parse statement. */
static stree_stat_t *parse_stat(parse_t *parse)
{
	stree_stat_t *stat;

	stree_vdecl_t *vdecl_s;
	stree_if_t *if_s;
	stree_while_t *while_s;
	stree_for_t *for_s;
	stree_raise_t *raise_s;
	stree_return_t *return_s;
	stree_wef_t *wef_s;
	stree_exps_t *exp_s;

	switch (lcur_lc(parse)) {
	case lc_var:
		vdecl_s = parse_vdecl(parse);
		stat = stree_stat_new(st_vdecl);
		stat->u.vdecl_s = vdecl_s;
		break;
	case lc_if:
		if_s = parse_if(parse);
		stat = stree_stat_new(st_if);
		stat->u.if_s = if_s;
		break;
	case lc_while:
		while_s = parse_while(parse);
		stat = stree_stat_new(st_while);
		stat->u.while_s = while_s;
		break;
	case lc_for:
		for_s = parse_for(parse);
		stat = stree_stat_new(st_for);
		stat->u.for_s = for_s;
		break;
	case lc_raise:
		raise_s = parse_raise(parse);
		stat = stree_stat_new(st_raise);
		stat->u.raise_s = raise_s;
		break;
	case lc_return:
		return_s = parse_return(parse);
		stat = stree_stat_new(st_return);
		stat->u.return_s = return_s;
		break;
	case lc_with:
		wef_s = parse_wef(parse);
		stat = stree_stat_new(st_wef);
		stat->u.wef_s = wef_s;
		break;
	default:
		exp_s = parse_exps(parse);
		stat = stree_stat_new(st_exps);
		stat->u.exp_s = exp_s;
		break;
	}

#ifdef DEBUG_PARSE_TRACE
	printf("Parsed statement %p\n", stat);
#endif
	return stat;
}

/** Parse variable declaration statement. */
static stree_vdecl_t *parse_vdecl(parse_t *parse)
{
	stree_vdecl_t *vdecl;

	vdecl = stree_vdecl_new();

	lmatch(parse, lc_var);
	vdecl->name = parse_ident(parse);
	lmatch(parse, lc_colon);
	vdecl->type = parse_texpr(parse);

	if (lcur_lc(parse) == lc_assign) {
		lskip(parse);
		(void) parse_expr(parse);
	}

	lmatch(parse, lc_scolon);

#ifdef DEBUG_PARSE_TRACE
	printf("Parsed vdecl for '%s'\n", strtab_get_str(vdecl->name->sid));
	printf("vdecl = %p, vdecl->name = %p, sid=%d\n",
	    vdecl, vdecl->name, vdecl->name->sid);
#endif
	return vdecl;
}

/** Parse @c if statement, */
static stree_if_t *parse_if(parse_t *parse)
{
	stree_if_t *if_s;

	if_s = stree_if_new();

	lmatch(parse, lc_if);
	if_s->cond = parse_expr(parse);
	lmatch(parse, lc_then);
	if_s->if_block = parse_block(parse);

	if (lcur_lc(parse) == lc_else) {
		lskip(parse);
		if_s->else_block = parse_block(parse);
	} else {
		if_s->else_block = NULL;
	}

	lmatch(parse, lc_end);
	return if_s;
}

/** Parse @c while statement. */
static stree_while_t *parse_while(parse_t *parse)
{
	stree_while_t *while_s;

	while_s = stree_while_new();

	lmatch(parse, lc_while);
	while_s->cond = parse_expr(parse);
	lmatch(parse, lc_do);
	while_s->body = parse_block(parse);
	lmatch(parse, lc_end);

	return while_s;
}

/** Parse @c for statement. */
static stree_for_t *parse_for(parse_t *parse)
{
	stree_for_t *for_s;

	for_s = stree_for_new();

	lmatch(parse, lc_for);
	lmatch(parse, lc_ident);
	lmatch(parse, lc_colon);
	(void) parse_texpr(parse);
	lmatch(parse, lc_in);
	(void) parse_expr(parse);
	lmatch(parse, lc_do);
	for_s->body = parse_block(parse);
	lmatch(parse, lc_end);

	return for_s;
}

/** Parse @c raise statement. */
static stree_raise_t *parse_raise(parse_t *parse)
{
	lmatch(parse, lc_raise);
	(void) parse_expr(parse);
	lmatch(parse, lc_scolon);

	return stree_raise_new();
}

/** Parse @c return statement. */
static stree_return_t *parse_return(parse_t *parse)
{
	stree_return_t *return_s;

	return_s = stree_return_new();

	lmatch(parse, lc_return);
	return_s->expr = parse_expr(parse);
	lmatch(parse, lc_scolon);

	return return_s;
}

/* Parse @c with-except-finally statement. */
static stree_wef_t *parse_wef(parse_t *parse)
{
	stree_wef_t *wef_s;
	stree_block_t *block;

	wef_s = stree_wef_new();
	list_init(&wef_s->except_blocks);

	lmatch(parse, lc_with);
	lmatch(parse, lc_ident);
	lmatch(parse, lc_colon);
	(void) parse_texpr(parse);
	lmatch(parse, lc_assign);
	(void) parse_expr(parse);
	lmatch(parse, lc_do);
	wef_s->with_block = parse_block(parse);

	while (lcur_lc(parse) == lc_except) {
		lmatch(parse, lc_except);
		lmatch(parse, lc_ident);
		lmatch(parse, lc_colon);
		(void) parse_texpr(parse);
		lmatch(parse, lc_do);

		block = parse_block(parse);
		list_append(&wef_s->except_blocks, block);
	}

	lmatch(parse, lc_finally);
	lmatch(parse, lc_do);
	wef_s->finally_block = parse_block(parse);
	lmatch(parse, lc_end);

	return wef_s;
}

/* Parse expression statement. */
static stree_exps_t *parse_exps(parse_t *parse)
{
	stree_expr_t *expr;
	stree_exps_t *exps;

	expr = parse_expr(parse);
	lmatch(parse, lc_scolon);

	exps = stree_exps_new();
	exps->expr = expr;

	return exps;
}

/** Parse identifier. */
stree_ident_t *parse_ident(parse_t *parse)
{
	stree_ident_t *ident;

	lcheck(parse, lc_ident);
	ident = stree_ident_new();
	ident->sid = lcur(parse)->u.ident.sid;
	lskip(parse);

	return ident;
}

/** Return current lem. */
lem_t *lcur(parse_t *parse)
{
	return &parse->lex->current;
}

/** Retturn current lem lclass. */
lclass_t lcur_lc(parse_t *parse)
{
	return parse->lex->current.lclass;
}

/** Skip to next lem. */
void lskip(parse_t *parse)
{
	lex_next(parse->lex);
}

/** Verify that lclass of current lem is @a lc. */
void lcheck(parse_t *parse, lclass_t lc)
{
	if (lcur(parse)->lclass != lc) {
		lem_print_coords(lcur(parse));
		printf(" Error: expected '"); lclass_print(lc);
		printf("', got '"); lem_print(lcur(parse));
		printf("'.\n");
		exit(1);
	}
}

/** Verify that lclass of current lem is @a lc and go to next lem. */
void lmatch(parse_t *parse, lclass_t lc)
{
	lcheck(parse, lc);
	lskip(parse);
}

/** Display generic parsing error. */
void lunexpected_error(parse_t *parse)
{
	lem_print_coords(lcur(parse));
	printf(" Error: unexpected token '");
	lem_print(lcur(parse));
	printf("'.\n");
	exit(1);
}

/** Basically tells us whether @a lclass is in next(block). */
bool_t terminates_block(lclass_t lclass)
{
	switch (lclass) {
	case lc_else:
	case lc_end:
	case lc_except:
	case lc_finally:
		return b_true;
	default:
		return b_false;
	}
}
