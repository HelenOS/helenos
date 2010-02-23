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

/** @file Builtin functions. */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "list.h"
#include "mytypes.h"
#include "run.h"
#include "stree.h"
#include "strtab.h"

#include "builtin.h"

static void builtin_write_line(run_t *run);

static stree_symbol_t *bi_write_line;

/** Declare builtin symbols in the program.
 *
 * Declares symbols that will be hooked to builtin interpreter functions.
 */
void builtin_declare(stree_program_t *program)
{
	stree_modm_t *modm;
	stree_csi_t *csi;
	stree_ident_t *ident;
	stree_csimbr_t *csimbr;
	stree_fun_t *fun;
	stree_fun_arg_t *fun_arg;
	stree_symbol_t *symbol;

	ident = stree_ident_new();
	ident->sid = strtab_get_sid("Builtin");

	csi = stree_csi_new(csi_class);
	csi->name = ident;
	list_init(&csi->members);

	modm = stree_modm_new(mc_csi);
	modm->u.csi = csi;

	symbol = stree_symbol_new(sc_csi);
	symbol->u.csi = csi;
	symbol->outer_csi = NULL;
	csi->symbol = symbol;

	list_append(&program->module->members, modm);

	/* Declare builtin functions. */
	ident = stree_ident_new();
	ident->sid = strtab_get_sid("WriteLine");

	fun = stree_fun_new();
	fun->name = ident;
	fun->body = NULL;
	list_init(&fun->args);

	csimbr = stree_csimbr_new(csimbr_fun);
	csimbr->u.fun = fun;

	symbol = stree_symbol_new(sc_fun);
	symbol->u.fun = fun;
	symbol->outer_csi = csi;
	fun->symbol = symbol;

	list_append(&csi->members, csimbr);

	fun_arg = stree_fun_arg_new();
	fun_arg->name = stree_ident_new();
	fun_arg->name->sid = strtab_get_sid("arg");
	fun_arg->type = NULL; /* XXX */

	list_append(&fun->args, fun_arg);

	bi_write_line = symbol;
}

void builtin_run_fun(run_t *run, stree_symbol_t *fun_sym)
{
#ifdef DEBUG_RUN_TRACE
	printf("Run builtin function.\n");
#endif
	if (fun_sym == bi_write_line) {
		builtin_write_line(run);
	} else {
		assert(b_false);
	}
}

static void builtin_write_line(run_t *run)
{
	rdata_var_t *var;

#ifdef DEBUG_RUN_TRACE
	printf("Called Builtin.writeLine()\n");
#endif
	var = run_local_vars_lookup(run, strtab_get_sid("arg"));
	assert(var);

	switch (var->vc) {
	case vc_int:
		printf("%d\n", var->u.int_v->value);
		break;
	case vc_string:
		printf("%s\n", var->u.string_v->value);
		break;
	default:
		printf("Unimplemented: writeLine() with unsupported type.\n");
		exit(1);
	}
}
