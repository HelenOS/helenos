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

/** @file Main module.
 *
 * Main entry point for SBI, the Sysel Bootstrap Interpreter.
 * When run without parameters, the interpreter will enter interactive
 * mode.
 */

#include <stdio.h>
#include <stdlib.h>
#include "ancr.h"
#include "os/os.h"
#include "builtin.h"
#include "imode.h"
#include "mytypes.h"
#include "program.h"
#include "strtab.h"
#include "stree.h"
#include "stype.h"
#include "input.h"
#include "lex.h"
#include "parse.h"
#include "run.h"

static void syntax_print(void);

/** Main entry point.
 *
 * @return	Zero on success, non-zero on error.
 */
int main(int argc, char *argv[])
{
	stree_program_t *program;
	stype_t stype;
	run_t run;
	errno_t rc;

	/* Store executable file path under which we have been invoked. */
	os_store_ef_path(*argv);

	argv += 1;
	argc -= 1;

	if (argc == 0) {
		/* Enter interactive mode */
		strtab_init();
		imode_run();
		return 0;
	}

	if (os_str_cmp(*argv, "-h") == 0) {
		syntax_print();
		return 0;
	}

	strtab_init();
	program = stree_program_new();
	program->module = stree_module_new();

	/* Declare builtin symbols. */
	builtin_declare(program);

	/* Process source files in the library. */
	if (program_lib_process(program) != EOK)
		return 1;

	/* Resolve ancestry. */
	ancr_module_process(program, program->module);

	/* Bind internal interpreter references to symbols. */
	builtin_bind(program->builtin);

	/* Process all source files specified in command-line arguments. */
	while (argc > 0) {
		rc = program_file_process(program, *argv);
		if (rc != EOK)
			return 1;

		argv += 1;
		argc -= 1;
	}

	/* Resolve ancestry. */
	ancr_module_process(program, program->module);

	/* Type program. */
	stype.program = program;
	stype.error = b_false;
	stype_module(&stype, program->module);

	/* Check for typing errors. */
	if (stype.error)
		return 1;

	/* Run program. */
	run_init(&run);
	run_program(&run, program);

	/* Check for run-time errors. */
	if (run.thread_ar->error)
		return 1;

	return 0;
}

/** Print command-line syntax help. */
static void syntax_print(void)
{
	printf("Syntax: sbi <source_file.sy>\n");
}
