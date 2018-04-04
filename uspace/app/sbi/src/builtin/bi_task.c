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

/** @file Task builtin binding. */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "../builtin.h"
#include "../list.h"
#include "../mytypes.h"
#include "../os/os.h"
#include "../run.h"
#include "../strtab.h"

#include "bi_task.h"

static void bi_task_exec(run_t *run);

/** Declare task builtin.
 *
 * @param bi	Builtin object
 */
void bi_task_declare(builtin_t *bi)
{
	builtin_code_snippet(bi,
	    "class Task is\n"
	    "fun Exec(args : string[], packed), static, builtin;\n"
	    "end\n");
}

/** Bind builtin functions.
 *
 * @param bi	Builtin object
 */
void bi_task_bind(builtin_t *bi)
{
	builtin_fun_bind(bi, "Task", "Exec", bi_task_exec);
}

/** Start an executable and wait for it to finish.
 *
 * @param run	Runner object
 */
static void bi_task_exec(run_t *run)
{
	rdata_var_t *args;
	rdata_var_t *var;
	rdata_array_t *array;
	rdata_var_t *arg;
	int idx, dim;
	const char **cmd;

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

	cmd[dim] = NULL;

	if (os_exec((char *const *)cmd) != EOK) {
		printf("Error: Exec failed.\n");
		exit(1);
	}
}
