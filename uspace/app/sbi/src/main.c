/*
 * SPDX-FileCopyrightText: 2010 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
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
