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

/** @file TextFile builtin binding. */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "../bigint.h"
#include "../builtin.h"
#include "../debug.h"
#include "../mytypes.h"
#include "../os/os.h"
#include "../rdata.h"
#include "../run.h"
#include "../strtab.h"

#include "bi_textfile.h"

#define LINE_BUF_SIZE 256
static char line_buf[LINE_BUF_SIZE];

static void bi_textfile_openread(run_t *run);
static void bi_textfile_openwrite(run_t *run);
static void bi_textfile_close(run_t *run);
static void bi_textfile_readline(run_t *run);
static void bi_textfile_writeline(run_t *run);
static void bi_textfile_is_eof(run_t *run);

/** Declare TextFile builtin.
 *
 * @param bi	Builtin object
 */
void bi_textfile_declare(builtin_t *bi)
{
	/* Declare class TextFile. */

	builtin_code_snippet(bi,
	    "class TextFile is\n"
	    "    var f : resource;\n"
	    "\n"
	    "    fun OpenRead(fname : string), builtin;\n"
	    "    fun OpenWrite(fname : string), builtin;\n"
	    "    fun Close(), builtin;\n"
	    "    fun ReadLine() : string, builtin;\n"
	    "    fun WriteLine(line : string), builtin;\n"
	    "\n"
	    "    prop EOF : bool is\n"
	    "        get is\n"
	    "            return is_eof();\n"
	    "        end\n"
	    "    end\n"
	    "\n"
	    "    fun is_eof() : bool, builtin;\n"
	    "end\n");

}

/** Bind TextFile builtin.
 *
 * @param bi	Builtin object
 */
void bi_textfile_bind(builtin_t *bi)
{
	builtin_fun_bind(bi, "TextFile", "OpenRead", bi_textfile_openread);
	builtin_fun_bind(bi, "TextFile", "OpenWrite", bi_textfile_openwrite);
	builtin_fun_bind(bi, "TextFile", "Close", bi_textfile_close);
	builtin_fun_bind(bi, "TextFile", "ReadLine", bi_textfile_readline);
	builtin_fun_bind(bi, "TextFile", "WriteLine", bi_textfile_writeline);
	builtin_fun_bind(bi, "TextFile", "is_eof", bi_textfile_is_eof);
}

/** Open a text file for reading.
 *
 * @param run	Runner object
 */
static void bi_textfile_openread(run_t *run)
{
	rdata_var_t *fname_var;
	const char *fname;
	FILE *file;

	rdata_resource_t *resource;
	rdata_var_t *res_var;
	rdata_value_t *res_val;
	rdata_var_t *self_f_var;

#ifdef DEBUG_RUN_TRACE
	printf("Called TextFile.OpenRead()\n");
#endif
	fname_var = run_local_vars_lookup(run, strtab_get_sid("fname"));
	assert(fname_var);
	assert(fname_var->vc == vc_string);

	fname = fname_var->u.string_v->value;
	file = fopen(fname, "rt");
	if (file == NULL) {
		printf("Error: Failed opening file '%s' for reading.\n",
		    fname);
		exit(1);
	}

	resource = rdata_resource_new();
	resource->data = (void *) file;
	res_var = rdata_var_new(vc_resource);
	res_var->u.resource_v = resource;

	res_val = rdata_value_new();
	res_val->var = res_var;

	/* Store resource handle into self.f */
	self_f_var = builtin_get_self_mbr_var(run, "f");
	rdata_var_write(self_f_var, res_val);
}

/** Open a text file for writing.
 *
 * @param run	Runner object
 */
static void bi_textfile_openwrite(run_t *run)
{
	rdata_var_t *fname_var;
	const char *fname;
	FILE *file;

	rdata_resource_t *resource;
	rdata_var_t *res_var;
	rdata_value_t *res_val;
	rdata_var_t *self_f_var;

#ifdef DEBUG_RUN_TRACE
	printf("Called TextFile.OpenWrite()\n");
#endif
	fname_var = run_local_vars_lookup(run, strtab_get_sid("fname"));
	assert(fname_var);
	assert(fname_var->vc == vc_string);

	fname = fname_var->u.string_v->value;
	file = fopen(fname, "wt");
	if (file == NULL) {
		printf("Error: Failed opening file '%s' for writing.\n",
		    fname);
		exit(1);
	}

	resource = rdata_resource_new();
	resource->data = (void *) file;
	res_var = rdata_var_new(vc_resource);
	res_var->u.resource_v = resource;

	res_val = rdata_value_new();
	res_val->var = res_var;

	/* Store resource handle into self.f */
	self_f_var = builtin_get_self_mbr_var(run, "f");
	rdata_var_write(self_f_var, res_val);
}

/** Close a text file.
 *
 * @param run	Runner object
 */
static void bi_textfile_close(run_t *run)
{
	FILE *file;
	rdata_var_t *self_f_var;
	run_proc_ar_t *proc_ar;

	/* Extract pointer to file structure. */
	self_f_var = builtin_get_self_mbr_var(run, "f");
	assert(self_f_var->vc == vc_resource);
	file = (FILE *) self_f_var->u.resource_v->data;

	if (file == NULL) {
		printf("Error: TextFile not valid for Close.\n");
		exit(1);
	}

	/* Close the file. */

#ifdef DEBUG_RUN_TRACE
	printf("Close text file.\n");
#endif
	if (fclose(file) != 0) {
		printf("Error: I/O error while closing file.\n");
		exit(1);
	}

	/* Invalidate the resource handle. */
	self_f_var->u.resource_v->data = NULL;

	proc_ar = run_get_current_proc_ar(run);
	proc_ar->retval = NULL;
}

/** Read one line from a text file.
 *
 * @param run	Runner object
 */
static void bi_textfile_readline(run_t *run)
{
	FILE *file;
	rdata_var_t *self_f_var;

	rdata_string_t *str;
	rdata_var_t *str_var;
	rdata_value_t *str_val;
	rdata_item_t *str_item;

	run_proc_ar_t *proc_ar;
	char *cp;

	/* Extract pointer to file structure. */
	self_f_var = builtin_get_self_mbr_var(run, "f");
	assert(self_f_var->vc == vc_resource);
	file = (FILE *) self_f_var->u.resource_v->data;

	if (file == NULL) {
		printf("Error: TextFile not valid for ReadLine.\n");
		exit(1);
	}

	/* Check and read. */

	if (feof(file)) {
		printf("Error: Reading beyond end of file.\n");
		exit(1);
	}

	if (fgets(line_buf, LINE_BUF_SIZE, file) == NULL)
		line_buf[0] = '\0';

	if (ferror(file)) {
		printf("Error: I/O error while reading file.\n");
		exit(1);
	}

	/* Remove trailing newline, if present. */

	cp = line_buf;
	while (*cp != '\0')
		++cp;

	if (cp != line_buf && cp[-1] == '\n')
		cp[-1] = '\0';

#ifdef DEBUG_RUN_TRACE
	printf("Read '%s' from file.\n", line_buf);
#endif
	/* Construct return value. */
	str = rdata_string_new();
	str->value = os_str_dup(line_buf);

	str_var = rdata_var_new(vc_string);
	str_var->u.string_v = str;
	str_val = rdata_value_new();
	str_val->var = str_var;

	str_item = rdata_item_new(ic_value);
	str_item->u.value = str_val;

	proc_ar = run_get_current_proc_ar(run);
	proc_ar->retval = str_item;
}

/** Write one line to a text file.
 *
 * @param run	Runner object
 */
static void bi_textfile_writeline(run_t *run)
{
	FILE *file;
	rdata_var_t *self_f_var;
	rdata_var_t *line_var;
	const char *line;

	run_proc_ar_t *proc_ar;

	/* Get 'line' argument. */
	line_var = run_local_vars_lookup(run, strtab_get_sid("line"));
	assert(line_var);
	assert(line_var->vc == vc_string);
	line = line_var->u.string_v->value;

	/* Extract pointer to file structure. */
	self_f_var = builtin_get_self_mbr_var(run, "f");
	assert(self_f_var->vc == vc_resource);
	file = (FILE *) self_f_var->u.resource_v->data;

	if (file == NULL) {
		printf("Error: TextFile not valid for WriteLine.\n");
		exit(1);
	}

	/* Write and check. */

#ifdef DEBUG_RUN_TRACE
	printf("Write '%s' to file.\n", line);
#endif
	if (fprintf(file, "%s\n", line) < 0) {
		printf("Error: I/O error while writing file.\n");
		exit(1);
	}

	proc_ar = run_get_current_proc_ar(run);
	proc_ar->retval = NULL;
}

/** Return value of EOF flag.
 *
 * @param run	Runner object
 */
static void bi_textfile_is_eof(run_t *run)
{
	FILE *file;
	rdata_var_t *self_f_var;

	bool_t eof_flag;
	rdata_bool_t *eof_bool;
	rdata_var_t *eof_var;
	rdata_value_t *eof_val;
	rdata_item_t *eof_item;

	run_proc_ar_t *proc_ar;

	/* Extract pointer to file structure. */
	self_f_var = builtin_get_self_mbr_var(run, "f");
	assert(self_f_var->vc == vc_resource);
	file = (FILE *) self_f_var->u.resource_v->data;

	if (file == NULL) {
		printf("Error: TextFile not valid for EOF check.\n");
		exit(1);
	}

	/* Get status of EOF flag. */

	eof_flag = feof(file) ? b_true : b_false;

#ifdef DEBUG_RUN_TRACE
	printf("Read EOF flag '%s'.\n", eof_flag ? "true" : "false");
#endif
	/* Construct return value. */
	eof_bool = rdata_bool_new();
	eof_bool->value = eof_flag;

	eof_var = rdata_var_new(vc_bool);
	eof_var->u.bool_v = eof_bool;
	eof_val = rdata_value_new();
	eof_val->var = eof_var;

	eof_item = rdata_item_new(ic_value);
	eof_item->u.value = eof_val;

	proc_ar = run_get_current_proc_ar(run);
	proc_ar->retval = eof_item;
}
