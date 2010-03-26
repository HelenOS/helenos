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

/** @file Builtin symbol binding. */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "ancr.h"
#include "builtin/bi_fun.h"
#include "builtin/bi_textfile.h"
#include "input.h"
#include "intmap.h"
#include "lex.h"
#include "list.h"
#include "mytypes.h"
#include "os/os.h"
#include "parse.h"
#include "run.h"
#include "stree.h"
#include "strtab.h"
#include "symbol.h"

#include "builtin.h"

static builtin_t *builtin_new(void);

/** Declare builtin symbols in the program.
 *
 * Declares symbols that will be hooked to builtin interpreter procedures.
 */
void builtin_declare(stree_program_t *program)
{
	builtin_t *bi;

	bi = builtin_new();
	bi->program = program;
	program->builtin = bi;

	/*
	 * Declare grandfather class.
	 */

	builtin_code_snippet(bi,
		"class Object is\n"
		"end\n");
	bi->gf_class = builtin_find_lvl0(bi, "Object");

	/*
	 * Declare other builtin classes/functions.
	 */

	bi_fun_declare(bi);
	bi_textfile_declare(bi);

	/* Need to process ancestry so that symbol lookups work. */
	ancr_module_process(program, program->module);

	bi_fun_bind(bi);
	bi_textfile_bind(bi);
}

/** Get grandfather class. */
stree_csi_t *builtin_get_gf_class(builtin_t *builtin)
{
	if (builtin->gf_class == NULL)
		return NULL;

	return symbol_to_csi(builtin->gf_class);
}

static builtin_t *builtin_new(void)
{
	builtin_t *builtin;

	builtin = calloc(1, sizeof(builtin_t));
	if (builtin == NULL) {
		printf("Memory allocation failed.\n");
		exit(1);
	}

	return builtin;
}

/** Parse a declaration code snippet.
 *
 * Parses a piece of code from a string at the module level. This can be
 * used to declare builtin symbols easily and without need for an external
 * file.
 */
void builtin_code_snippet(builtin_t *bi, const char *snippet)
{
	input_t *input;
	lex_t lex;
	parse_t parse;

	input_new_string(&input, snippet);
	lex_init(&lex, input);
	parse_init(&parse, bi->program, &lex);
	parse_module(&parse);
}

/** Simplifed search for a global symbol. */
stree_symbol_t *builtin_find_lvl0(builtin_t *bi, const char *sym_name)
{
	stree_symbol_t *sym;
	stree_ident_t *ident;

	ident = stree_ident_new();

	ident->sid = strtab_get_sid(sym_name);
	sym = symbol_lookup_in_csi(bi->program, NULL, ident);

	return sym;
}

/** Simplifed search for a level 1 symbol. */
stree_symbol_t *builtin_find_lvl1(builtin_t *bi, const char *csi_name,
    const char *sym_name)
{
	stree_symbol_t *csi_sym;
	stree_csi_t *csi;

	stree_symbol_t *mbr_sym;
	stree_ident_t *ident;

	ident = stree_ident_new();

	ident->sid = strtab_get_sid(csi_name);
	csi_sym = symbol_lookup_in_csi(bi->program, NULL, ident);
	csi = symbol_to_csi(csi_sym);
	assert(csi != NULL);

	ident->sid = strtab_get_sid(sym_name);
	mbr_sym = symbol_lookup_in_csi(bi->program, csi, ident);

	return mbr_sym;
}

void builtin_fun_bind(builtin_t *bi, const char *csi_name,
    const char *sym_name, builtin_proc_t bproc)
{
	stree_symbol_t *fun_sym;
	stree_fun_t *fun;

	fun_sym = builtin_find_lvl1(bi, csi_name, sym_name);
	assert(fun_sym != NULL);
	fun = symbol_to_fun(fun_sym);
	assert(fun != NULL);

	fun->proc->bi_handler = bproc;
}

void builtin_run_proc(run_t *run, stree_proc_t *proc)
{
	stree_symbol_t *fun_sym;
	builtin_t *bi;
	builtin_proc_t bproc;

#ifdef DEBUG_RUN_TRACE
	printf("Run builtin procedure.\n");
#endif
	fun_sym = proc->outer_symbol;
	bi = run->program->builtin;

	bproc = proc->bi_handler;
	if (bproc == NULL) {
		printf("Error: Unrecognized builtin function '");
		symbol_print_fqn(fun_sym);
		printf("'.\n");
		exit(1);
	}

	/* Run the builtin procedure handler. */
	(*bproc)(run);
}

/** Get pointer to member var of current object. */
rdata_var_t *builtin_get_self_mbr_var(run_t *run, const char *mbr_name)
{
	run_proc_ar_t *proc_ar;
	rdata_object_t *object;
	sid_t mbr_name_sid;
	rdata_var_t *mbr_var;

	proc_ar = run_get_current_proc_ar(run);
	assert(proc_ar->obj->vc == vc_object);
	object = proc_ar->obj->u.object_v;

	mbr_name_sid = strtab_get_sid(mbr_name);
	mbr_var = intmap_get(&object->fields, mbr_name_sid);
	assert(mbr_var != NULL);

	return mbr_var;
}

/** Declare a builtin function in @a csi. */
stree_symbol_t *builtin_declare_fun(stree_csi_t *csi, const char *name)
{
	stree_ident_t *ident;
	stree_fun_t *fun;
	stree_csimbr_t *csimbr;
	stree_symbol_t *fun_sym;

	ident = stree_ident_new();
	ident->sid = strtab_get_sid(name);

	fun = stree_fun_new();
	fun->name = ident;
	fun->proc = stree_proc_new();
	fun->proc->body = NULL;
	list_init(&fun->args);

	csimbr = stree_csimbr_new(csimbr_fun);
	csimbr->u.fun = fun;

	fun_sym = stree_symbol_new(sc_fun);
	fun_sym->u.fun = fun;
	fun_sym->outer_csi = csi;
	fun->symbol = fun_sym;
	fun->proc->outer_symbol = fun_sym;

	list_append(&csi->members, csimbr);

	return fun_sym;
}

/** Add one formal parameter to function. */
void builtin_fun_add_arg(stree_symbol_t *fun_sym, const char *name)
{
	stree_proc_arg_t *proc_arg;
	stree_fun_t *fun;

	fun = symbol_to_fun(fun_sym);
	assert(fun != NULL);

	proc_arg = stree_proc_arg_new();
	proc_arg->name = stree_ident_new();
	proc_arg->name->sid = strtab_get_sid(name);
	proc_arg->type = NULL; /* XXX */

	list_append(&fun->args, proc_arg);
}
