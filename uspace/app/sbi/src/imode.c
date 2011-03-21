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

/** @file Interactive mode.
 *
 * In interactive mode the user types in statements. As soon as the outermost
 * statement is complete (terminated with ';' or 'end'), the interpreter
 * executes it. Otherwise it prompts the user until the entire statement
 * is read in.
 *
 * The user interface depends on the OS. In HelenOS we use the CLUI library
 * which gives us rich line editing capabilities.
 */

#include <stdio.h>
#include <stdlib.h>
#include "os/os.h"
#include "ancr.h"
#include "assert.h"
#include "builtin.h"
#include "intmap.h"
#include "list.h"
#include "mytypes.h"
#include "program.h"
#include "rdata.h"
#include "stree.h"
#include "strtab.h"
#include "stype.h"
#include "input.h"
#include "lex.h"
#include "parse.h"
#include "run.h"

#include "imode.h"

/** Run in interactive mode.
 *
 * Repeatedly read in statements from the user and execute them.
 */
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
	stype_block_vr_t *block_vr;
	list_node_t *bvr_n;
	rdata_item_t *rexpr;
	rdata_item_t *rexpr_vi;

	run_t run;
	run_proc_ar_t *proc_ar;

	bool_t quit_im;

	/* Create an empty program. */
	program = stree_program_new();
	program->module = stree_module_new();

	/* Declare builtin symbols. */
	builtin_declare(program);

	/* Process the library. */
	if (program_lib_process(program) != EOK)
		exit(1);

	/* Resolve ancestry. */
	ancr_module_process(program, program->module);

	/* Bind internal interpreter references to symbols. */
	builtin_bind(program->builtin);

	/* Resolve ancestry. */
	ancr_module_process(program, program->module);

	/* Construct typing context. */
	stype.program = program;
	stype.proc_vr = stype_proc_vr_new();
	list_init(&stype.proc_vr->block_vr);
	stype.current_csi = NULL;
	proc = stree_proc_new();

	fun = stree_fun_new();
	fun_sym = stree_symbol_new(sc_fun);
	fun_sym->u.fun = fun;
	fun->name = stree_ident_new();
	fun->name->sid = strtab_get_sid("$imode");
	fun->sig = stree_fun_sig_new();

	stype.proc_vr->proc = proc;
	fun->symbol = fun_sym;
	proc->outer_symbol = fun_sym;

	/* Create block visit record. */
	block_vr = stype_block_vr_new();
	intmap_init(&block_vr->vdecls);

	/* Add block visit record to the stack. */
	list_append(&stype.proc_vr->block_vr, block_vr);

	/* Construct run context. */
	run_gdata_init(&run);

	run.thread_ar = run_thread_ar_new();
	list_init(&run.thread_ar->proc_ar);
	run_proc_ar_create(&run, run.gdata, proc, &proc_ar);
	list_append(&run.thread_ar->proc_ar, proc_ar);

	printf("SBI interactive mode. ");
	os_input_disp_help();

	quit_im = b_false;
	while (quit_im != b_true) {
		parse.error = b_false;
		stype.error = b_false;
		run.thread_ar->exc_payload = NULL;
		run.thread_ar->bo_mode = bm_none;

		input_new_interactive(&input);

		/* Parse input. */
		lex_init(&lex, input);
		parse_init(&parse, program, &lex);

		if (lcur_lc(&parse) == lc_eof)
			break;

		stat = parse_stat(&parse);

		if (parse.error != b_false)
			continue;

		/* Type statement. */
		stype_stat(&stype, stat, b_true);

		if (stype.error != b_false)
			continue;

		/* Run statement. */
		run_init(&run);
		run.program = program;
		run_stat(&run, stat, &rexpr);

		/* Check for unhandled exceptions. */
		run_exc_check_unhandled(&run);

		if (rexpr != NULL) {
			/* Convert expression result to value item. */
			run_cvt_value_item(&run, rexpr, &rexpr_vi);
			rdata_item_destroy(rexpr);

			/* Check for unhandled exceptions. */
			run_exc_check_unhandled(&run);
		} else {
			rexpr_vi = NULL;
		}

		/*
		 * rexpr_vi can be NULL if either repxr was null or
		 * if the conversion to value item raised an exception.
		 */
		if (rexpr_vi != NULL) {
			assert(rexpr_vi->ic == ic_value);

			/* Print result. */
			printf("Result: ");
			rdata_value_print(rexpr_vi->u.value);
			printf("\n");

			rdata_item_destroy(rexpr_vi);
		}
	}

	run_proc_ar_destroy(&run, proc_ar);

	/* Remove block visit record from the stack, */
	bvr_n = list_last(&stype.proc_vr->block_vr);
	assert(list_node_data(bvr_n, stype_block_vr_t *) == block_vr);
	list_remove(&stype.proc_vr->block_vr, bvr_n);

	printf("\nBye!\n");
}
