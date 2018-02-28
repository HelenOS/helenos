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
#include "os/os.h"
#include "mytypes.h"
#include "stype.h"
#include "input.h"
#include "lex.h"
#include "parse.h"

#include "program.h"

#define MAX_NAME_SIZE 64
static char name_buf[MAX_NAME_SIZE + 1];

/** Process one specified source file.
 *
 * @param program 	The program to which the parsed code is added.
 * @param fname		Name of file to read from.
 * @return		EOK on success, EIO if file is not found,
 *			EINVAL if file has syntax errors.
 */
errno_t program_file_process(stree_program_t *program, const char *fname)
{
	input_t *input;
	lex_t lex;
	parse_t parse;
	errno_t rc;

	rc = input_new_file(&input, fname);
	if (rc != EOK) {
		printf("Failed opening source file '%s'.\n", fname);
		return EIO;
	}

	/* Parse input file. */
	lex_init(&lex, input);
	parse_init(&parse, program, &lex);
	parse_module(&parse);

	/* Check for parse errors. */
	if (parse.error)
		return EINVAL;

	return EOK;
}

/** Process sources of the library.
 *
 * Processes all source files in the library. The list of library source files
 * is read from '<libdir>/libflist'. Each line of the file contains one file
 * name relative to <libdir>.
 *
 * @param program 	The program to which the parsed code is added.
 * @return		EOK on success, EIO if some file comprising the
 *			library is not found, EINVAL if the library
 *			has syntax errors.
 */
errno_t program_lib_process(stree_program_t *program)
{
	errno_t rc;
	char *path, *fname;
	char *tmp;
	char *cp;

	FILE *f;

	path = os_get_lib_path();
	fname = os_str_acat(path, "/libflist");

	f = fopen(fname, "rt");
	if (f == NULL) {
		printf("Failed opening library list file '%s'.\n", fname);
		free(path);
		free(fname);
		return EIO;
	}

	free(fname);

	while (b_true) {
		if (fgets(name_buf, MAX_NAME_SIZE + 1, f) == NULL)
			break;

		/*
		 * Remove trailing newline, if present.
		 */
		cp = name_buf;
		while (*cp != '\0' && *cp != '\n')
			++cp;

		if (*cp == '\n')
			*cp = '\0';

		tmp = os_str_acat(path, "/");
		fname = os_str_acat(tmp, name_buf);
		free(tmp);

		rc = program_file_process(program, fname);
		if (rc != EOK) {
			free(fname);
			free(path);
			fclose(f);
			return rc;
		}

		free(fname);
	}

	if (ferror(f)) {
		printf("Error reading from library list file.\n");
		free(path);
		fclose(f);
		return EIO;
	}

	free(path);
	fclose(f);

	return EOK;
}
