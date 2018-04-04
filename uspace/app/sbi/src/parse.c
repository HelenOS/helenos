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

/** @file Parser
 *
 * Consumes a sequence of lexical elements and produces a syntax tree (stree).
 * Parsing primitives, module members, statements.
 */

#include <assert.h>
#include <stdlib.h>
#include "cspan.h"
#include "debug.h"
#include "lex.h"
#include "list.h"
#include "mytypes.h"
#include "p_expr.h"
#include "p_type.h"
#include "stree.h"
#include "strtab.h"
#include "symbol.h"

#include "parse.h"

/*
 * Module and CSI members
 */
static stree_csi_t *parse_csi(parse_t *parse, lclass_t dclass,
    stree_csi_t *outer_csi);
static stree_csimbr_t *parse_csimbr(parse_t *parse, stree_csi_t *outer_csi);

static stree_ctor_t *parse_ctor(parse_t *parse, stree_csi_t *outer_csi);

static stree_enum_t *parse_enum(parse_t *parse, stree_csi_t *outer_csi);
static stree_embr_t *parse_embr(parse_t *parse, stree_enum_t *outer_enum);

static stree_deleg_t *parse_deleg(parse_t *parse, stree_csi_t *outer_csi);
static stree_fun_t *parse_fun(parse_t *parse, stree_csi_t *outer_csi);
static stree_var_t *parse_var(parse_t *parse, stree_csi_t *outer_csi);
static stree_prop_t *parse_prop(parse_t *parse, stree_csi_t *outer_csi);

static void parse_symbol_attrs(parse_t *parse, stree_symbol_t *symbol);
static stree_symbol_attr_t *parse_symbol_attr(parse_t *parse);

static stree_proc_arg_t *parse_proc_arg(parse_t *parse);
static stree_arg_attr_t *parse_arg_attr(parse_t *parse);
static stree_fun_sig_t *parse_fun_sig(parse_t *parse);

static void parse_prop_get(parse_t *parse, stree_prop_t *prop);
static void parse_prop_set(parse_t *parse, stree_prop_t *prop);

/*
 * Statements
 */
static stree_block_t *parse_block(parse_t *parse);

static stree_vdecl_t *parse_vdecl(parse_t *parse);
static stree_if_t *parse_if(parse_t *parse);
static stree_switch_t *parse_switch(parse_t *parse);
static stree_while_t *parse_while(parse_t *parse);
static stree_for_t *parse_for(parse_t *parse);
static stree_raise_t *parse_raise(parse_t *parse);
static stree_break_t *parse_break(parse_t *parse);
static stree_return_t *parse_return(parse_t *parse);
static stree_wef_t *parse_wef(parse_t *parse);
static stree_exps_t *parse_exps(parse_t *parse);

static stree_except_t *parse_except(parse_t *parse);

/** Initialize parser object.
 *
 * Set up parser @a parse to use lexer @a lex for input and to store
 * output (i.e. new declarations) to program @a prog. @a prog is not
 * necessarily empty, the declarations being parsed are simply added
 * to it.
 *
 * @param parse		Parser object.
 * @param prog		Destination program stree.
 * @param lex		Input lexer.
 */
void parse_init(parse_t *parse, stree_program_t *prog, struct lex *lex)
{
	parse->program = prog;
	parse->cur_mod = parse->program->module;
	parse->lex = lex;

	parse->error = b_false;
	parse->error_bailout = b_false;

	lex_next(parse->lex);
}

/** Parse module.
 *
 * Parse a program module.
 *
 * The input is read using the lexer associated with @a parse. The resulting
 * declarations are added to existing declarations in the program associated
 * with @a parse.
 *
 * If any parse error occurs, parse->error will @c b_true when this function
 * returns. parse->error_bailout will be @c b_true if the error has not
 * been recovered yet. Similar holds for other parsing functions in this
 * module.
 *
 * @param parse		Parser object.
 */
void parse_module(parse_t *parse)
{
	stree_csi_t *csi;
	stree_enum_t *enum_d;
	stree_modm_t *modm;

	while (lcur_lc(parse) != lc_eof && !parse_is_error(parse)) {
		switch (lcur_lc(parse)) {
		case lc_class:
		case lc_struct:
		case lc_interface:
			csi = parse_csi(parse, lcur_lc(parse), NULL);
			modm = stree_modm_new(mc_csi);
			modm->u.csi = csi;

			list_append(&parse->cur_mod->members, modm);
			break;
		case lc_enum:
			enum_d = parse_enum(parse, NULL);
			modm = stree_modm_new(mc_enum);
			modm->u.enum_d = enum_d;

			list_append(&parse->cur_mod->members, modm);
			break;
		default:
			lunexpected_error(parse);
			lex_next(parse->lex);
			break;
		}

	}
}

/** Parse class, struct or interface declaration.
 *
 * @param parse		Parser object.
 * @param dclass	What to parse: @c lc_class, @c lc_struct or @c lc_csi.
 * @param outer_csi	CSI containing this declaration or @c NULL if global.
 * @return		New syntax tree node.
 */
static stree_csi_t *parse_csi(parse_t *parse, lclass_t dclass,
    stree_csi_t *outer_csi)
{
	stree_csi_t *csi;
	csi_class_t cc;
	stree_csimbr_t *csimbr;
	stree_symbol_t *symbol;
	stree_ident_t *targ_name;
	stree_targ_t *targ;
	stree_texpr_t *pref;

	switch (dclass) {
	case lc_class:
		cc = csi_class;
		break;
	case lc_struct:
		cc = csi_struct;
		break;
	case lc_interface:
		cc = csi_interface;
		break;
	default:
		assert(b_false);
	}

	lskip(parse);

	csi = stree_csi_new(cc);
	csi->name = parse_ident(parse);

	list_init(&csi->targ);

	while (lcur_lc(parse) == lc_slash) {
		lskip(parse);
		targ_name = parse_ident(parse);

		targ = stree_targ_new();
		targ->name = targ_name;

		list_append(&csi->targ, targ);
	}

	symbol = stree_symbol_new(sc_csi);
	symbol->u.csi = csi;
	symbol->outer_csi = outer_csi;
	csi->symbol = symbol;

#ifdef DEBUG_PARSE_TRACE
	printf("parse_csi: csi=%p, csi->name = %p (%s)\n", csi, csi->name,
	    strtab_get_str(csi->name->sid));
#endif
	if (lcur_lc(parse) == lc_colon) {
		/* Inheritance list */
		lskip(parse);

		while (b_true) {
			pref = parse_texpr(parse);
			if (parse_is_error(parse))
				break;

			list_append(&csi->inherit, pref);
			if (lcur_lc(parse) != lc_plus)
				break;

			lskip(parse);
		}
	}

	lmatch(parse, lc_is);
	list_init(&csi->members);

	/* Parse class, struct or interface members. */
	while (lcur_lc(parse) != lc_end && !parse_is_error(parse)) {
		csimbr = parse_csimbr(parse, csi);
		if (csimbr == NULL)
			continue;

		list_append(&csi->members, csimbr);
	}

	lmatch(parse, lc_end);

	if (outer_csi != NULL) {
		switch (outer_csi->cc) {
		case csi_class:
		case csi_struct:
			break;
		case csi_interface:
			cspan_print(csi->name->cspan);
			printf(" Error: CSI declared inside interface.\n");
			parse_note_error(parse);
			/* XXX Free csi */
			return NULL;
		}
	}

	return csi;
}

/** Parse class, struct or interface member.
 *
 * @param parse		Parser object.
 * @param outer_csi	CSI containing this declaration.
 * @return		New syntax tree node. In case of parse error,
 *			@c NULL may (but need not) be returned.
 */
static stree_csimbr_t *parse_csimbr(parse_t *parse, stree_csi_t *outer_csi)
{
	stree_csimbr_t *csimbr;

	stree_csi_t *csi;
	stree_ctor_t *ctor;
	stree_deleg_t *deleg;
	stree_enum_t *enum_d;
	stree_fun_t *fun;
	stree_var_t *var;
	stree_prop_t *prop;

	csimbr = NULL;

	switch (lcur_lc(parse)) {
	case lc_class:
	case lc_struct:
	case lc_interface:
		csi = parse_csi(parse, lcur_lc(parse), outer_csi);
		if (csi != NULL) {
			csimbr = stree_csimbr_new(csimbr_csi);
			csimbr->u.csi = csi;
		}
		break;
	case lc_new:
		ctor = parse_ctor(parse, outer_csi);
		if (ctor != NULL) {
			csimbr = stree_csimbr_new(csimbr_ctor);
			csimbr->u.ctor = ctor;
		}
		break;
	case lc_deleg:
		deleg = parse_deleg(parse, outer_csi);
		if (deleg != NULL) {
			csimbr = stree_csimbr_new(csimbr_deleg);
			csimbr->u.deleg = deleg;
		}
		break;
	case lc_enum:
		enum_d = parse_enum(parse, outer_csi);
		if (enum_d != NULL) {
			csimbr = stree_csimbr_new(csimbr_enum);
			csimbr->u.enum_d = enum_d;
		}
		break;
	case lc_fun:
		fun = parse_fun(parse, outer_csi);
		csimbr = stree_csimbr_new(csimbr_fun);
		csimbr->u.fun = fun;
		break;
	case lc_var:
		var = parse_var(parse, outer_csi);
		if (var != NULL) {
			csimbr = stree_csimbr_new(csimbr_var);
			csimbr->u.var = var;
		}
		break;
	case lc_prop:
		prop = parse_prop(parse, outer_csi);
		csimbr = stree_csimbr_new(csimbr_prop);
		csimbr->u.prop = prop;
		break;
	default:
		lunexpected_error(parse);
		lex_next(parse->lex);
		break;
	}

	return csimbr;
}

/** Parse constructor.
 *
 * @param parse		Parser object.
 * @param outer_csi	CSI containing this declaration or @c NULL if global.
 * @return		New syntax tree node.
 */
static stree_ctor_t *parse_ctor(parse_t *parse, stree_csi_t *outer_csi)
{
	stree_ctor_t *ctor;
	stree_symbol_t *symbol;
	cspan_t *cspan;

	ctor = stree_ctor_new();
	symbol = stree_symbol_new(sc_ctor);

	symbol->u.ctor = ctor;
	symbol->outer_csi = outer_csi;
	ctor->symbol = symbol;

	lmatch(parse, lc_new);
	cspan = lprev_span(parse);

	/* Fake identifier. */
	ctor->name = stree_ident_new();
	ctor->name->sid = strtab_get_sid(CTOR_IDENT);
	ctor->name->cspan = lprev_span(parse);

#ifdef DEBUG_PARSE_TRACE
	printf("Parsing constructor of CSI '");
	symbol_print_fqn(csi_to_symbol(outer_csi));
	printf("'.\n");
#endif
	ctor->sig = parse_fun_sig(parse);
	if (ctor->sig->rtype != NULL) {
		cspan_print(cspan);
		printf(" Error: Constructor of CSI '");
		symbol_print_fqn(csi_to_symbol(outer_csi));
		printf("' has a return type.\n");
		parse_note_error(parse);
	}

	/* Parse attributes. */
	parse_symbol_attrs(parse, symbol);

	ctor->proc = stree_proc_new();
	ctor->proc->outer_symbol = symbol;

	if (lcur_lc(parse) == lc_scolon) {
		lskip(parse);

		/* This constructor has no body. */
		cspan_print(cspan);
		printf(" Error: Constructor of CSI '");
		symbol_print_fqn(csi_to_symbol(outer_csi));
		printf("' has no body.\n");
		parse_note_error(parse);

		ctor->proc->body = NULL;
	} else {
		lmatch(parse, lc_is);
		ctor->proc->body = parse_block(parse);
		lmatch(parse, lc_end);
	}

	switch (outer_csi->cc) {
	case csi_class:
	case csi_struct:
		break;
	case csi_interface:
		cspan_print(ctor->name->cspan);
		printf(" Error: Constructor declared inside interface.\n");
		parse_note_error(parse);
		/* XXX Free ctor */
		return NULL;
	}

	return ctor;
}

/** Parse @c enum declaration.
 *
 * @param parse		Parser object.
 * @param outer_csi	CSI containing this declaration or @c NULL if global.
 * @return		New syntax tree node.
 */
static stree_enum_t *parse_enum(parse_t *parse, stree_csi_t *outer_csi)
{
	stree_enum_t *enum_d;
	stree_symbol_t *symbol;
	stree_embr_t *embr;

	enum_d = stree_enum_new();
	symbol = stree_symbol_new(sc_enum);

	symbol->u.enum_d = enum_d;
	symbol->outer_csi = outer_csi;
	enum_d->symbol = symbol;

	lmatch(parse, lc_enum);
	enum_d->name = parse_ident(parse);
	list_init(&enum_d->members);

#ifdef DEBUG_PARSE_TRACE
	printf("Parse enum '%s'.\n", strtab_get_str(enum_d->name->sid));
#endif
	lmatch(parse, lc_is);

	/* Parse enum members. */
	while (lcur_lc(parse) != lc_end && !parse_is_error(parse)) {
		embr = parse_embr(parse, enum_d);
		if (embr == NULL)
			break;

		list_append(&enum_d->members, embr);
	}

	if (list_is_empty(&enum_d->members)) {
		cspan_print(enum_d->name->cspan);
		printf("Error: Enum type '%s' has no members.\n",
		    strtab_get_str(enum_d->name->sid));
		parse_note_error(parse);
	}

	lmatch(parse, lc_end);

	if (outer_csi != NULL) {
		switch (outer_csi->cc) {
		case csi_class:
		case csi_struct:
			break;
		case csi_interface:
			cspan_print(enum_d->name->cspan);
			printf(" Error: Enum declared inside interface.\n");
			parse_note_error(parse);
			/* XXX Free enum */
			return NULL;
		}
	}

	return enum_d;
}

/** Parse enum member.
 *
 * @param parse		Parser object.
 * @param outer_enum	Enum containing this declaration.
 * @return		New syntax tree node. In case of parse error,
 *			@c NULL may (but need not) be returned.
 */
static stree_embr_t *parse_embr(parse_t *parse, stree_enum_t *outer_enum)
{
	stree_embr_t *embr;

	embr = stree_embr_new();
	embr->outer_enum = outer_enum;
	embr->name = parse_ident(parse);

	lmatch(parse, lc_scolon);

	return embr;
}

/** Parse delegate.
 *
 * @param parse		Parser object.
 * @param outer_csi	CSI containing this declaration or @c NULL if global.
 * @return		New syntax tree node.
 */
static stree_deleg_t *parse_deleg(parse_t *parse, stree_csi_t *outer_csi)
{
	stree_deleg_t *deleg;
	stree_symbol_t *symbol;

	deleg = stree_deleg_new();
	symbol = stree_symbol_new(sc_deleg);

	symbol->u.deleg = deleg;
	symbol->outer_csi = outer_csi;
	deleg->symbol = symbol;

	lmatch(parse, lc_deleg);
	deleg->name = parse_ident(parse);

#ifdef DEBUG_PARSE_TRACE
	printf("Parsing delegate '%s'.\n", strtab_get_str(deleg->name->sid));
#endif

	deleg->sig = parse_fun_sig(parse);

	/* Parse attributes. */
	parse_symbol_attrs(parse, symbol);

	lmatch(parse, lc_scolon);

	switch (outer_csi->cc) {
	case csi_class:
	case csi_struct:
		break;
	case csi_interface:
		cspan_print(deleg->name->cspan);
		printf(" Error: Delegate declared inside interface.\n");
		parse_note_error(parse);
		/* XXX Free deleg */
		return NULL;
	}

	return deleg;
}

/** Parse member function.
 *
 * @param parse		Parser object.
 * @param outer_csi	CSI containing this declaration or @c NULL if global.
 * @return		New syntax tree node.
 */
static stree_fun_t *parse_fun(parse_t *parse, stree_csi_t *outer_csi)
{
	stree_fun_t *fun;
	stree_symbol_t *symbol;
	bool_t body_expected;

	fun = stree_fun_new();
	symbol = stree_symbol_new(sc_fun);

	symbol->u.fun = fun;
	symbol->outer_csi = outer_csi;
	fun->symbol = symbol;

	lmatch(parse, lc_fun);
	fun->name = parse_ident(parse);

#ifdef DEBUG_PARSE_TRACE
	printf("Parsing function '%s'.\n", strtab_get_str(fun->name->sid));
#endif
	fun->sig = parse_fun_sig(parse);

	/* Parse attributes. */
	parse_symbol_attrs(parse, symbol);

	body_expected = !stree_symbol_has_attr(symbol, sac_builtin) &&
	    (outer_csi->cc != csi_interface);

	fun->proc = stree_proc_new();
	fun->proc->outer_symbol = symbol;

	if (lcur_lc(parse) == lc_scolon) {
		lskip(parse);

		/* Body not present */
		if (body_expected) {
			cspan_print(fun->name->cspan);
			printf(" Error: Function '");
			symbol_print_fqn(symbol);
			printf("' should have a body.\n");
			parse_note_error(parse);
		}

		fun->proc->body = NULL;
	} else {
		lmatch(parse, lc_is);
		fun->proc->body = parse_block(parse);
		lmatch(parse, lc_end);

		/* Body present */
		if (!body_expected) {
			cspan_print(fun->name->cspan);
			printf(" Error: Function declaration '");
			symbol_print_fqn(symbol);
			printf("' should not have a body.\n");
			parse_note_error(parse);
		}
	}

	return fun;
}

/** Parse member variable.
 *
 * @param parse		Parser object.
 * @param outer_csi	CSI containing this declaration or @c NULL if global.
 * @return		New syntax tree node.
 */
static stree_var_t *parse_var(parse_t *parse, stree_csi_t *outer_csi)
{
	stree_var_t *var;
	stree_symbol_t *symbol;

	var = stree_var_new();
	symbol = stree_symbol_new(sc_var);
	symbol->u.var = var;
	symbol->outer_csi = outer_csi;
	var->symbol = symbol;

	lmatch(parse, lc_var);
	var->name = parse_ident(parse);
	lmatch(parse, lc_colon);
	var->type =  parse_texpr(parse);

	parse_symbol_attrs(parse, symbol);

	lmatch(parse, lc_scolon);

	switch (outer_csi->cc) {
	case csi_class:
	case csi_struct:
		break;
	case csi_interface:
		cspan_print(var->name->cspan);
		printf(" Error: Variable declared inside interface.\n");
		parse_note_error(parse);
		/* XXX Free var */
		return NULL;
	}

	return var;
}

/** Parse member property.
 *
 * @param parse		Parser object.
 * @param outer_csi	CSI containing this declaration or @c NULL if global.
 * @return		New syntax tree node.
 */
static stree_prop_t *parse_prop(parse_t *parse, stree_csi_t *outer_csi)
{
	stree_prop_t *prop;
	stree_symbol_t *symbol;

	stree_ident_t *ident;
	stree_proc_arg_t *arg;

	prop = stree_prop_new();
	list_init(&prop->args);

	symbol = stree_symbol_new(sc_prop);
	symbol->u.prop = prop;
	symbol->outer_csi = outer_csi;
	prop->symbol = symbol;

	lmatch(parse, lc_prop);

	if (lcur_lc(parse) == lc_self) {
		/* Indexed property set */

		/* Use some name that is impossible as identifier. */
		ident = stree_ident_new();
		ident->sid = strtab_get_sid(INDEXER_IDENT);
		prop->name = ident;

		lskip(parse);
		lmatch(parse, lc_lsbr);

		/* Parse formal parameters. */
		while (!parse_is_error(parse)) {
			arg = parse_proc_arg(parse);
			if (stree_arg_has_attr(arg, aac_packed)) {
				prop->varg = arg;
				break;
			} else {
				list_append(&prop->args, arg);
			}

			if (lcur_lc(parse) == lc_rsbr)
				break;

			lmatch(parse, lc_scolon);
		}

		lmatch(parse, lc_rsbr);
	} else {
		/* Named property */
		prop->name = parse_ident(parse);
	}

	lmatch(parse, lc_colon);
	prop->type = parse_texpr(parse);

	/* Parse attributes. */
	parse_symbol_attrs(parse, symbol);

	lmatch(parse, lc_is);

	while (lcur_lc(parse) != lc_end && !parse_is_error(parse)) {
		switch (lcur_lc(parse)) {
		case lc_get:
			parse_prop_get(parse, prop);
			break;
		case lc_set:
			parse_prop_set(parse, prop);
			break;
		default:
			lunexpected_error(parse);
		}
	}

	lmatch(parse, lc_end);

	return prop;
}

/** Parse symbol attributes.
 *
 * Parse list of attributes and add them to @a symbol.
 *
 * @param parse		Parser object
 * @param symbol	Symbol to add these attributes to
 */
static void parse_symbol_attrs(parse_t *parse, stree_symbol_t *symbol)
{
	stree_symbol_attr_t *attr;

	/* Parse attributes. */
	while (lcur_lc(parse) == lc_comma && !parse_is_error(parse)) {
		lskip(parse);
		attr = parse_symbol_attr(parse);
		list_append(&symbol->attr, attr);
	}
}

/** Parse symbol attribute.
 *
 * @param parse		Parser object
 * @return		New syntax tree node
 */
static stree_symbol_attr_t *parse_symbol_attr(parse_t *parse)
{
	stree_symbol_attr_t *attr;
	symbol_attr_class_t sac;

	/* Make compiler happy. */
	sac = 0;

	switch (lcur_lc(parse)) {
	case lc_builtin:
		sac = sac_builtin;
		break;
	case lc_static:
		sac = sac_static;
		break;
	default:
		cspan_print(lcur_span(parse));
		printf(" Error: Unexpected attribute '");
		lem_print(lcur(parse));
		printf("'.\n");
		parse_note_error(parse);
		break;
	}

	lskip(parse);

	attr = stree_symbol_attr_new(sac);
	return attr;
}

/** Parse formal function argument.
 *
 * @param parse		Parser object.
 * @return		New syntax tree node.
 */
static stree_proc_arg_t *parse_proc_arg(parse_t *parse)
{
	stree_proc_arg_t *arg;
	stree_arg_attr_t *attr;

	arg = stree_proc_arg_new();
	arg->name = parse_ident(parse);
	lmatch(parse, lc_colon);
	arg->type = parse_texpr(parse);

#ifdef DEBUG_PARSE_TRACE
	printf("Parse procedure argument.\n");
#endif
	list_init(&arg->attr);

	/* Parse attributes. */
	while (lcur_lc(parse) == lc_comma && !parse_is_error(parse)) {
		lskip(parse);
		attr = parse_arg_attr(parse);
		list_append(&arg->attr, attr);
	}

	return arg;
}

/** Parse argument attribute.
 *
 * @param parse		Parser object.
 * @return		New syntax tree node.
 */
static stree_arg_attr_t *parse_arg_attr(parse_t *parse)
{
	stree_arg_attr_t *attr;

	if (lcur_lc(parse) != lc_packed) {
		cspan_print(lcur_span(parse));
		printf(" Error: Unexpected attribute '");
		lem_print(lcur(parse));
		printf("'.\n");
		parse_note_error(parse);
	}

	lskip(parse);

	attr = stree_arg_attr_new(aac_packed);
	return attr;
}

/** Parse function signature.
 *
 * @param parse		Parser object.
 * @return		New syntax tree node.
 */
static stree_fun_sig_t *parse_fun_sig(parse_t *parse)
{
	stree_fun_sig_t *sig;
	stree_proc_arg_t *arg;

	sig = stree_fun_sig_new();

	lmatch(parse, lc_lparen);

#ifdef DEBUG_PARSE_TRACE
	printf("Parsing function signature.\n");
#endif

	list_init(&sig->args);

	if (lcur_lc(parse) != lc_rparen) {

		/* Parse formal parameters. */
		while (!parse_is_error(parse)) {
			arg = parse_proc_arg(parse);

			if (stree_arg_has_attr(arg, aac_packed)) {
				sig->varg = arg;
				break;
			} else {
				list_append(&sig->args, arg);
			}

			if (lcur_lc(parse) == lc_rparen)
				break;

			lmatch(parse, lc_scolon);
		}
	}

	lmatch(parse, lc_rparen);

	if (lcur_lc(parse) == lc_colon) {
		lskip(parse);
		sig->rtype = parse_texpr(parse);
	} else {
		sig->rtype = NULL;
	}

	return sig;
}

/** Parse member property getter.
 *
 * @param parse		Parser object.
 * @param prop		Property containing this declaration.
 */
static void parse_prop_get(parse_t *parse, stree_prop_t *prop)
{
	cspan_t *cspan;
	stree_block_t *block;
	stree_proc_t *getter;
	bool_t body_expected;

	body_expected = (prop->symbol->outer_csi->cc != csi_interface);

	lskip(parse);
	cspan = lprev_span(parse);

	if (prop->getter != NULL) {
		cspan_print(cspan);
		printf(" Error: Duplicate getter.\n");
		parse_note_error(parse);
		return;
	}

	if (lcur_lc(parse) == lc_scolon) {
		/* Body not present */
		lskip(parse);
		block = NULL;

		if (body_expected) {
			cspan_print(prop->name->cspan);
			printf(" Error: Property '");
			symbol_print_fqn(prop->symbol);
			printf("' getter should have "
			    "a body.\n");
			parse_note_error(parse);
		}
	} else {
		/* Body present */
		lmatch(parse, lc_is);
		block = parse_block(parse);
		lmatch(parse, lc_end);

		if (!body_expected) {
			cspan_print(prop->name->cspan);
			printf(" Error: Property '");
			symbol_print_fqn(prop->symbol);
			printf("' getter declaration should "
			    "not have a body.\n");
			parse_note_error(parse);

			/* XXX Free block */
			block = NULL;
		}
	}

	/* Create getter procedure */
	getter = stree_proc_new();
	getter->body = block;
	getter->outer_symbol = prop->symbol;

	/* Store getter in property. */
	prop->getter = getter;
}


/** Parse member property setter.
 *
 * @param parse		Parser object.
 * @param prop		Property containing this declaration.
 */
static void parse_prop_set(parse_t *parse, stree_prop_t *prop)
{
	cspan_t *cspan;
	stree_block_t *block;
	stree_proc_t *setter;
	bool_t body_expected;

	body_expected = (prop->symbol->outer_csi->cc != csi_interface);

	lskip(parse);
	cspan = lprev_span(parse);

	if (prop->setter != NULL) {
		cspan_print(cspan);
		printf(" Error: Duplicate setter.\n");
		parse_note_error(parse);
		return;
	}

	prop->setter_arg = stree_proc_arg_new();
	prop->setter_arg->name = parse_ident(parse);
	prop->setter_arg->type = prop->type;

	if (lcur_lc(parse) == lc_scolon) {
		/* Body not present */
		lskip(parse);

		block = NULL;

		if (body_expected) {
			cspan_print(prop->name->cspan);
			printf(" Error: Property '");
			symbol_print_fqn(prop->symbol);
			printf("' setter should have "
			    "a body.\n");
			parse_note_error(parse);
		}
	} else {
		/* Body present */
		lmatch(parse, lc_is);
		block = parse_block(parse);
		lmatch(parse, lc_end);

		if (!body_expected) {
			cspan_print(prop->name->cspan);
			printf(" Error: Property '");
			symbol_print_fqn(prop->symbol);
			printf("' setter declaration should "
			    "not have a body.\n");
			parse_note_error(parse);
		}
	}


	/* Create setter procedure */
	setter = stree_proc_new();
	setter->body = block;
	setter->outer_symbol = prop->symbol;

	/* Store setter in property. */
	prop->setter = setter;
}

/** Parse statement block.
 *
 * @param parse		Parser object.
 * @return		New syntax tree node.
 */
static stree_block_t *parse_block(parse_t *parse)
{
	stree_block_t *block;
	stree_stat_t *stat;

	block = stree_block_new();
	list_init(&block->stats);

	/* Avoid peeking if there is an error condition. */
	if (parse_is_error(parse))
		return block;

	while (terminates_block(lcur_lc(parse)) != b_true &&
	    !parse_is_error(parse)) {

		stat = parse_stat(parse);
		list_append(&block->stats, stat);
	}

	return block;
}

/** Parse statement.
 *
 * @param parse		Parser object.
 * @return		New syntax tree node.
 */
stree_stat_t *parse_stat(parse_t *parse)
{
	stree_stat_t *stat;

	stree_vdecl_t *vdecl_s;
	stree_if_t *if_s;
	stree_switch_t *switch_s;
	stree_while_t *while_s;
	stree_for_t *for_s;
	stree_raise_t *raise_s;
	stree_break_t *break_s;
	stree_return_t *return_s;
	stree_wef_t *wef_s;
	stree_exps_t *exp_s;

#ifdef DEBUG_PARSE_TRACE
	printf("Parse statement.\n");
#endif
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
	case lc_switch:
		switch_s = parse_switch(parse);
		stat = stree_stat_new(st_switch);
		stat->u.switch_s = switch_s;
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
	case lc_break:
		break_s = parse_break(parse);
		stat = stree_stat_new(st_break);
		stat->u.break_s = break_s;
		break;
	case lc_return:
		return_s = parse_return(parse);
		stat = stree_stat_new(st_return);
		stat->u.return_s = return_s;
		break;
	case lc_do:
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

/** Parse variable declaration statement.
 *
 * @param parse		Parser object.
 * @return		New syntax tree node.
 */
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

/** Parse @c if statement.
 *
 * @param parse		Parser object.
 * @return		New syntax tree node.
 */
static stree_if_t *parse_if(parse_t *parse)
{
	stree_if_t *if_s;
	stree_if_clause_t *if_c;

#ifdef DEBUG_PARSE_TRACE
	printf("Parse 'if' statement.\n");
#endif
	if_s = stree_if_new();
	list_init(&if_s->if_clauses);

	/* Parse @c if clause. */
	lmatch(parse, lc_if);

	if_c = stree_if_clause_new();
	if_c->cond = parse_expr(parse);
	lmatch(parse, lc_then);
	if_c->block = parse_block(parse);

	list_append(&if_s->if_clauses, if_c);

	/* Parse @c elif clauses. */
	while (lcur_lc(parse) == lc_elif) {
		lskip(parse);
		if_c = stree_if_clause_new();
		if_c->cond = parse_expr(parse);
		lmatch(parse, lc_then);
		if_c->block = parse_block(parse);

		list_append(&if_s->if_clauses, if_c);
	}

	/* Parse @c else clause. */
	if (lcur_lc(parse) == lc_else) {
		lskip(parse);
		if_s->else_block = parse_block(parse);
	} else {
		if_s->else_block = NULL;
	}

	lmatch(parse, lc_end);
	return if_s;
}

/** Parse @c switch statement.
 *
 * @param parse		Parser object.
 * @return		New syntax tree node.
 */
static stree_switch_t *parse_switch(parse_t *parse)
{
	stree_switch_t *switch_s;
	stree_when_t *when_c;
	stree_expr_t *expr;

#ifdef DEBUG_PARSE_TRACE
	printf("Parse 'switch' statement.\n");
#endif
	lmatch(parse, lc_switch);

	switch_s = stree_switch_new();
	list_init(&switch_s->when_clauses);

	switch_s->expr = parse_expr(parse);
	lmatch(parse, lc_is);

	/* Parse @c when clauses. */
	while (lcur_lc(parse) == lc_when) {
		lskip(parse);
		when_c = stree_when_new();
		list_init(&when_c->exprs);
		while (b_true) {
			expr = parse_expr(parse);
			list_append(&when_c->exprs, expr);
			if (lcur_lc(parse) != lc_comma)
				break;
			lskip(parse);
		}

		lmatch(parse, lc_do);
		when_c->block = parse_block(parse);

		list_append(&switch_s->when_clauses, when_c);
	}

	/* Parse @c else clause. */
	if (lcur_lc(parse) == lc_else) {
		lskip(parse);
		lmatch(parse, lc_do);
		switch_s->else_block = parse_block(parse);
	} else {
		switch_s->else_block = NULL;
	}

	lmatch(parse, lc_end);
	return switch_s;
}

/** Parse @c while statement.
 *
 * @param parse		Parser object.
 */
static stree_while_t *parse_while(parse_t *parse)
{
	stree_while_t *while_s;

#ifdef DEBUG_PARSE_TRACE
	printf("Parse 'while' statement.\n");
#endif
	while_s = stree_while_new();

	lmatch(parse, lc_while);
	while_s->cond = parse_expr(parse);
	lmatch(parse, lc_do);
	while_s->body = parse_block(parse);
	lmatch(parse, lc_end);

	return while_s;
}

/** Parse @c for statement.
 *
 * @param parse		Parser object.
 * @return		New syntax tree node.
 */
static stree_for_t *parse_for(parse_t *parse)
{
	stree_for_t *for_s;

#ifdef DEBUG_PARSE_TRACE
	printf("Parse 'for' statement.\n");
#endif
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

/** Parse @c raise statement.
 *
 * @param parse		Parser object.
 */
static stree_raise_t *parse_raise(parse_t *parse)
{
	stree_raise_t *raise_s;

#ifdef DEBUG_PARSE_TRACE
	printf("Parse 'raise' statement.\n");
#endif
	raise_s = stree_raise_new();
	lmatch(parse, lc_raise);
	raise_s->expr = parse_expr(parse);
	lmatch(parse, lc_scolon);

	return raise_s;
}

/** Parse @c break statement.
 *
 * @param parse		Parser object.
 * @return		New syntax tree node.
 */
static stree_break_t *parse_break(parse_t *parse)
{
	stree_break_t *break_s;

#ifdef DEBUG_PARSE_TRACE
	printf("Parse 'break' statement.\n");
#endif
	break_s = stree_break_new();

	lmatch(parse, lc_break);
	lmatch(parse, lc_scolon);

	return break_s;
}

/** Parse @c return statement.
 *
 * @param parse		Parser object.
 * @return		New syntax tree node.
 */
static stree_return_t *parse_return(parse_t *parse)
{
	stree_return_t *return_s;

#ifdef DEBUG_PARSE_TRACE
	printf("Parse 'return' statement.\n");
#endif
	return_s = stree_return_new();

	lmatch(parse, lc_return);

	if (lcur_lc(parse) != lc_scolon)
		return_s->expr = parse_expr(parse);

	lmatch(parse, lc_scolon);

	return return_s;
}

/* Parse @c with-except-finally statement.
 *
 * @param parse		Parser object.
 * @return		New syntax tree node.
 */
static stree_wef_t *parse_wef(parse_t *parse)
{
	stree_wef_t *wef_s;
	stree_except_t *except_c;

#ifdef DEBUG_PARSE_TRACE
	printf("Parse WEF statement.\n");
#endif
	wef_s = stree_wef_new();
	list_init(&wef_s->except_clauses);

	if (lcur_lc(parse) == lc_with) {
		lmatch(parse, lc_with);
		lmatch(parse, lc_ident);
		lmatch(parse, lc_colon);
		(void) parse_texpr(parse);
		lmatch(parse, lc_assign);
		(void) parse_expr(parse);
	}

	lmatch(parse, lc_do);
	wef_s->with_block = parse_block(parse);

	while (lcur_lc(parse) == lc_except && !parse_is_error(parse)) {
		except_c = parse_except(parse);
		list_append(&wef_s->except_clauses, except_c);
	}

	if (lcur_lc(parse) == lc_finally) {
		lmatch(parse, lc_finally);
		lmatch(parse, lc_do);
		wef_s->finally_block = parse_block(parse);
	} else {
		wef_s->finally_block = NULL;
	}

	lmatch(parse, lc_end);

	return wef_s;
}

/* Parse expression statement.
 *
 * @param parse		Parser object.
 * @return		New syntax tree node.
 */
static stree_exps_t *parse_exps(parse_t *parse)
{
	stree_expr_t *expr;
	stree_exps_t *exps;

#ifdef DEBUG_PARSE_TRACE
	printf("Parse expression statement.\n");
#endif
	expr = parse_expr(parse);
	lmatch(parse, lc_scolon);

	exps = stree_exps_new();
	exps->expr = expr;

	return exps;
}

/* Parse @c except clause.
 *
 * @param parse		Parser object.
 * @return		New syntax tree node.
 */
static stree_except_t *parse_except(parse_t *parse)
{
	stree_except_t *except_c;

#ifdef DEBUG_PARSE_TRACE
	printf("Parse 'except' statement.\n");
#endif
	except_c = stree_except_new();

	lmatch(parse, lc_except);
	except_c->evar = parse_ident(parse);
	lmatch(parse, lc_colon);
	except_c->etype = parse_texpr(parse);
	lmatch(parse, lc_do);

	except_c->block = parse_block(parse);

	return except_c;
}

/** Parse identifier.
 *
 * @param parse		Parser object.
 * @return		New syntax tree node.
 */
stree_ident_t *parse_ident(parse_t *parse)
{
	stree_ident_t *ident;

#ifdef DEBUG_PARSE_TRACE
	printf("Parse identifier.\n");
#endif
	lcheck(parse, lc_ident);
	ident = stree_ident_new();
	ident->sid = lcur(parse)->u.ident.sid;
	ident->cspan = lcur_span(parse);
	lskip(parse);

	return ident;
}

/** Signal a parse error, start bailing out from parser.
 *
 * @param parse		Parser object.
 */
void parse_raise_error(parse_t *parse)
{
	parse->error = b_true;
	parse->error_bailout = b_true;
}

/** Note a parse error that has been immediately recovered.
 *
 * @param parse		Parser object.
 */
void parse_note_error(parse_t *parse)
{
	parse->error = b_true;
}

/** Check if we are currently bailing out of parser due to a parse error.
 *
 * @param parse		Parser object.
 */
bool_t parse_is_error(parse_t *parse)
{
	return parse->error_bailout;
}

/** Recover from parse error bailout.
 *
 * Still remember that there was an error, but stop bailing out.
 *
 * @param parse		Parser object.
 */
void parse_recover_error(parse_t *parse)
{
	assert(parse->error == b_true);
	assert(parse->error_bailout == b_true);

	parse->error_bailout = b_false;
}

/** Return current lem.
 *
 * @param parse		Parser object.
 * @return		Pointer to current lem. Only valid until the lexing
 *			position is advanced.
 */
lem_t *lcur(parse_t *parse)
{
#ifdef DEBUG_LPARSE_TRACE
	printf("lcur()\n");
#endif
	return lex_get_current(parse->lex);
}

/** Return current lem lclass.
 *
 * @param parse		Parser object
 * @return		Lclass of the current lem
 */
lclass_t lcur_lc(parse_t *parse)
{
	lem_t *lem;

	/*
	 * This allows us to skip error checking in many places. If there is an
	 * active error, lcur_lc() returns lc_invalid without reading input.
	 *
	 * Without this measure we would have to check for error all the time
	 * or risk requiring extra input from the user (in interactive mode)
	 * before actually bailing out from the parser.
	 */
	if (parse_is_error(parse))
		return lc_invalid;

	lem = lcur(parse);
	return lem->lclass;
}

/** Return coordinate span of current lem.
 *
 * @param parse		Parser object
 * @return		Coordinate span of current lem or @c NULL if a
 *			parse error is active
 */
cspan_t *lcur_span(parse_t *parse)
{
	lem_t *lem;

	if (parse_is_error(parse))
		return NULL;

	lem = lcur(parse);
	return lem->cspan;
}

/** Return coordinate span of previous lem.
 *
 * @param parse		Parser object
 * @return		Coordinate span of previous lem or @c NULL if
 * 			parse error is active or previous lem is not
 *			available.
 */
cspan_t *lprev_span(parse_t *parse)
{
	lem_t *lem;

	if (parse_is_error(parse))
		return NULL;

	lem = lex_peek_prev(parse->lex);
	if (lem == NULL)
		return NULL;

	return lem->cspan;
}

/** Skip to next lem.
 *
 * @param parse		Parser object.
 */
void lskip(parse_t *parse)
{
#ifdef DEBUG_LPARSE_TRACE
	printf("lskip()\n");
#endif
	lex_next(parse->lex);
}

/** Verify that lclass of current lem is @a lc.
 *
 * If a lem of different lclass is found, a parse error is raised and
 * a message is printed.
 *
 * @param parse		Parser object.
 * @param lc		Expected lclass.
 */
void lcheck(parse_t *parse, lclass_t lc)
{
#ifdef DEBUG_LPARSE_TRACE
	printf("lcheck(");
	lclass_print(lc);
	printf(")\n");
#endif
	if (lcur(parse)->lclass != lc) {
		lem_print_coords(lcur(parse));
		printf(" Error: expected '");
		lclass_print(lc);
		printf("', got '");
		lem_print(lcur(parse));
		printf("'.\n");
		parse_raise_error(parse);
	}
}

/** Verify that lclass of current lem is @a lc and go to next lem.
 *
 * If a lem of different lclass is found, a parse error is raised and
 * a message is printed.
 *
 * @param parse		Parser object.
 * @param lc		Expected lclass.
 */
void lmatch(parse_t *parse, lclass_t lc)
{
#ifdef DEBUG_LPARSE_TRACE
	printf("lmatch(");
	lclass_print(lc);
	printf(")\n");
#endif
	/*
	 * This allows us to skip error checking in many places. If there is an
	 * active error, lmatch() does nothing (similar to parse_block(), etc.
	 *
	 * Without this measure we would have to check for error all the time
	 * or risk requiring extra input from the user (in interactive mode)
	 * before actually bailing out from the parser.
	 */
	if (parse_is_error(parse))
		return;

	lcheck(parse, lc);
	lskip(parse);
}

/** Raise and display generic parsing error.
 *
 * @param parse		Parser object.
 */
void lunexpected_error(parse_t *parse)
{
	lem_print_coords(lcur(parse));
	printf(" Error: unexpected token '");
	lem_print(lcur(parse));
	printf("'.\n");
	parse_raise_error(parse);
}

/** Determine whether @a lclass is in follow(block).
 *
 * Tests whether @a lclass belongs to the follow(block) set, i.e. if it is
 * lclass of a lem that can follow a block in the program.
 *
 * @param lclass	Lclass.
 */
bool_t terminates_block(lclass_t lclass)
{
	switch (lclass) {
	case lc_elif:
	case lc_else:
	case lc_end:
	case lc_except:
	case lc_finally:
	case lc_when:
		return b_true;
	default:
		return b_false;
	}
}
