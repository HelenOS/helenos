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

/** @file Interactive mode. */

#include <stdio.h>
#include <stdlib.h>
#include "ancr.h"
#include "builtin.h"
#include "list.h"
#include "mytypes.h"
#include "stree.h"
#include "strtab.h"
#include "stype.h"
#include "input.h"
#include "lex.h"
#include "parse.h"
#include "run.h"

#include "imode.h"

void imode_run(void)
{
	input_t *input;
	lex_t lex;
	parse_t parse;
	stree_program_t *program;
	stree_stat_t *stat;
	stree_proc_t *proc;
	stree_fun_t *fun;
	stree_symbol_t *fun_sym;
	stype_t stype;

	run_t run;
	run_proc_ar_t *proc_ar;

	bool_t quit_im;

	/* Create an empty program. */
	program = stree_program_new();
	program->module = stree_module_new();

	/* Declare builtin symbols. */
	builtin_declare(program);

	/* Resolve ancestry. */
	ancr_module_process(program, program->module);

	quit_im = b_false;
	while (quit_im != b_true) {
		input_new_interactive(&input);

		/* Parse input. */
		lex_init(&lex, input);
		parse_init(&parse, program, &lex);

		if (lcur_lc(&parse) == lc_eof)
			break;

		stat = parse_stat(&parse);

		/* Construct typing context. */
		stype.program = program;
		stype.proc_vr = stype_proc_vr_new();
		stype.current_csi = NULL;
		proc = stree_proc_new();

		fun = stree_fun_new();
		fun_sym = stree_symbol_new(sc_fun);
		fun_sym->u.fun = fun;
		fun->name = stree_ident_new();
		fun->name->sid = strtab_get_sid("$imode");

		stype.proc_vr->proc = proc;
		fun->symbol = fun_sym;
		proc->outer_symbol = fun_sym;

		/* Construct run context. */
		run.thread_ar = run_thread_ar_new();
		list_init(&run.thread_ar->proc_ar);
		run_proc_ar_create(&run, NULL, proc, &proc_ar);
		list_append(&run.thread_ar->proc_ar, proc_ar);

		/* Type statement. */
		stype_stat(&stype, stat);

		/* Run statement. */
		run_init(&run);
		run.program = program;
		run_stat(&run, stat);
	}

	printf("\nBye!\n");
}
