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

/** @file Main module. */

#include <stdio.h>
#include <stdlib.h>
#include "ancr.h"
#include "builtin.h"
#include "mytypes.h"
#include "strtab.h"
#include "stree.h"
#include "stype.h"
#include "input.h"
#include "lex.h"
#include "parse.h"
#include "run.h"

void syntax_print(void);

int main(int argc, char *argv[])
{
	input_t *input;
	lex_t lex;
	parse_t parse;
	stree_program_t *program;
	stype_t stype;
	run_t run;
	int rc;

	if (argc != 2) {
		syntax_print();
		exit(1);
	}

	rc = input_new(&input, argv[1]);
	if (rc != EOK) {
		printf("Failed opening source file '%s'.\n", argv[1]);
		exit(1);
	}

	strtab_init();
	program = stree_program_new();
	program->module = stree_module_new();

	/* Declare builtin symbols. */
	builtin_declare(program);

	/* Parse input file. */
	lex_init(&lex, input);
	parse_init(&parse, program, &lex);
	parse_module(&parse);

	/* Resolve ancestry. */
	ancr_module_process(program, parse.cur_mod);

	/* Type program. */
	stype.program = program;
	stype_module(&stype, program->module);

	/* Run program. */
	run_init(&run);
	run_program(&run, program);

	return 0;
}

void syntax_print(void)
{
	printf("Missing or invalid arguments.\n");
	printf("Syntax: sbi <source_file.sy>\n");
}
