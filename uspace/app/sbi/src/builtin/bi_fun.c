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
#include "../bigint.h"
#include "../builtin.h"
#include "../list.h"
#include "../mytypes.h"
#include "../os/os.h"
#include "../run.h"
#include "../stree.h"
#include "../strtab.h"
#include "../symbol.h"

#include "bi_fun.h"

static void bi_fun_builtin_writeline(run_t *run);
static void bi_fun_task_exec(run_t *run);

/** Declare builtin functions. */
void bi_fun_declare(builtin_t *bi)
{
	stree_modm_t *modm;
	stree_csi_t *csi;
	stree_ident_t *ident;
	stree_symbol_t *symbol;
	stree_symbol_t *fun_sym;

	/* Declare class Builtin */

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

	list_append(&bi->program->module->members, modm);

	/* Declare Builtin.WriteLine(). */

	fun_sym = builtin_declare_fun(csi, "WriteLine");
	builtin_fun_add_arg(fun_sym, "arg");

	/* Declare class Task. */

	builtin_code_snippet(bi,
		"class Task is\n"
			"fun Exec(args : string[], packed), builtin;\n"
		"end\n");
}

/** Bind builtin functions. */
void bi_fun_bind(builtin_t *bi)
{
	builtin_fun_bind(bi, "Builtin", "WriteLine", bi_fun_builtin_writeline);
	builtin_fun_bind(bi, "Task", "Exec", bi_fun_task_exec);
}

/** Write a line of output. */
static void bi_fun_builtin_writeline(run_t *run)
{
	rdata_var_t *var;

#ifdef DEBUG_RUN_TRACE
	printf("Called Builtin.WriteLine()\n");
#endif
	var = run_local_vars_lookup(run, strtab_get_sid("arg"));
	assert(var);

	switch (var->vc) {
	case vc_int:
		bigint_print(&var->u.int_v->value);
		putchar('\n');
		break;
	case vc_string:
		printf("%s\n", var->u.string_v->value);
		break;
	default:
		printf("Unimplemented: writeLine() with unsupported type.\n");
		exit(1);
	}
}

/** Start an executable and wait for it to finish. */
static void bi_fun_task_exec(run_t *run)
{
	rdata_var_t *args;
	rdata_var_t *var;
	rdata_array_t *array;
	rdata_var_t *arg;
	int idx, dim;
	char **cmd;

#ifdef DEBUG_RUN_TRACE
	printf("Called Task.Exec()\n");
#endif
	args = run_local_vars_lookup(run, strtab_get_sid("args"));

	assert(args);
	assert(args->vc == vc_ref);

	var = args->u.ref_v->vref;
	assert(var->vc == vc_array);

	array = var->u.array_v;
	assert(array->rank == 1);
	dim = array->extent[0];

	if (dim == 0) {
		printf("Error: Builtin.Exec() expects at least one argument.\n");
		exit(1);
	}

	cmd = calloc(dim + 1, sizeof(char *));
	if (cmd == NULL) {
		printf("Memory allocation failed.\n");
		exit(1);
	}

	for (idx = 0; idx < dim; ++idx) {
		arg = array->element[idx];
		if (arg->vc != vc_string) {
			printf("Error: Argument to Builtin.Exec() must be "
			    "string (found %d).\n", arg->vc);
			exit(1);
		}

		cmd[idx] = arg->u.string_v->value;
	}

	cmd[dim] = '\0';

	if (os_exec(cmd) != EOK) {
		printf("Error: Exec failed.\n");
		exit(1);
	}
}
