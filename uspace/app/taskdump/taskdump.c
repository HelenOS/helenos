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

/** @addtogroup taskdump
 * @{
 */
/** @file
 */

#include <async.h>
#include <elf/elf_linux.h>
#include <fibrildump.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdbool.h>
#include <str_error.h>
#include <errno.h>
#include <udebug.h>
#include <task.h>
#include <as.h>
#include <libarch/istate.h>
#include <macros.h>
#include <assert.h>
#include <str.h>

#include <symtab.h>
#include <elf_core.h>
#include <stacktrace.h>
#include <taskdump.h>

#define LINE_BYTES 16
#define STACK_FRAMES_MAX 20

static async_sess_t *sess;
static task_id_t task_id;
static bool write_core_file;
static char *core_file_name;
static char *app_name;
static symtab_t *app_symtab;

static errno_t connect_task(task_id_t task_id);
static int parse_args(int argc, char *argv[]);
static void print_syntax(void);
static errno_t threads_dump(void);
static errno_t thread_dump(uintptr_t thash);
static errno_t areas_dump(void);
static errno_t td_read_uintptr(void *arg, uintptr_t addr, uintptr_t *value);

static void autoload_syms(void);
static char *get_app_task_name(void);
static char *fmt_sym_address(uintptr_t addr);

static istate_t reg_state;

static stacktrace_ops_t td_stacktrace_ops = {
	.read_uintptr = td_read_uintptr
};

int main(int argc, char *argv[])
{
	errno_t rc;

	printf("Task Dump Utility\n");
	write_core_file = false;

	if (parse_args(argc, argv) < 0)
		return 1;

	rc = connect_task(task_id);
	if (rc != EOK) {
		printf("Failed connecting to task %" PRIu64 ".\n", task_id);
		return 1;
	}

	app_name = get_app_task_name();
	app_symtab = NULL;

	printf("Dumping task '%s' (task ID %" PRIu64 ").\n", app_name, task_id);
	autoload_syms();
	putchar('\n');

	rc = threads_dump();
	if (rc != EOK)
		printf("Failed dumping threads.\n");

	rc = areas_dump();
	if (rc != EOK)
		printf("Failed dumping address space areas.\n");

	rc = fibrils_dump(app_symtab, sess);
	if (rc != EOK)
		printf("Failed dumping fibrils.\n");

	udebug_end(sess);
	async_hangup(sess);

	return 0;
}

static errno_t connect_task(task_id_t task_id)
{
	errno_t rc;
	async_sess_t *ksess = async_connect_kbox(task_id, &rc);

	if (!ksess) {
		if (rc == ENOTSUP) {
			printf("You do not have userspace debugging support "
			    "compiled in the kernel.\n");
			printf("Compile kernel with 'Support for userspace debuggers' "
			    "(CONFIG_UDEBUG) enabled.\n");
			return rc;
		}

		printf("Error connecting\n");
		printf("async_connect_kbox(%" PRIu64 ") -> %s", task_id, str_error_name(errno));
		return rc;
	}

	rc = udebug_begin(ksess);
	if (rc != EOK) {
		printf("udebug_begin() -> %s\n", str_error_name(rc));
		return rc;
	}

	sess = ksess;
	return 0;
}

static int parse_args(int argc, char *argv[])
{
	char *arg;
	char *err_p;

	task_id = 0;

	--argc;
	++argv;

	while (argc > 0) {
		arg = *argv;
		if (arg[0] == '-') {
			if (arg[1] == 't' && arg[2] == '\0') {
				/* Task ID */
				--argc;
				++argv;
				task_id = strtol(*argv, &err_p, 10);
				if (*err_p) {
					printf("Task ID syntax error\n");
					print_syntax();
					return -1;
				}
			} else if (arg[1] == 'c' && arg[2] == '\0') {
				write_core_file = true;

				--argc;
				++argv;
				core_file_name = *argv;
			} else {
				printf("Uknown option '%c'\n", arg[0]);
				print_syntax();
				return -1;
			}
		} else {
			break;
		}

		--argc;
		++argv;
	}

	if (task_id == 0) {
		printf("Missing task ID argument\n");
		print_syntax();
		return -1;
	}

	if (argc != 0) {
		printf("Extra arguments\n");
		print_syntax();
		return -1;
	}

	return 0;
}

static void print_syntax(void)
{
	printf("Syntax: taskdump [-c <core_file>] -t <task_id>\n");
	printf("\t-c <core_file_id>\tName of core file to write.\n");
	printf("\t-t <task_id>\tWhich task to dump.\n");
}

static errno_t threads_dump(void)
{
	uintptr_t *thash_buf;
	uintptr_t dummy_buf;
	size_t buf_size, n_threads;

	size_t copied;
	size_t needed;
	size_t i;
	errno_t rc;

	/* TODO: See why NULL does not work. */
	rc = udebug_thread_read(sess, &dummy_buf, 0, &copied, &needed);
	if (rc != EOK) {
		printf("udebug_thread_read() -> %s\n", str_error_name(rc));
		return rc;
	}

	if (needed == 0) {
		printf("No threads.\n\n");
		return 0;
	}

	buf_size = needed;
	thash_buf = malloc(buf_size);

	rc = udebug_thread_read(sess, thash_buf, buf_size, &copied, &needed);
	if (rc != EOK) {
		printf("udebug_thread_read() -> %s\n", str_error_name(rc));
		return rc;
	}

	assert(copied == buf_size);
	assert(needed == buf_size);

	n_threads = copied / sizeof(uintptr_t);

	printf("Threads:\n");
	for (i = 0; i < n_threads; i++) {
		printf(" [%zu] hash: %p\n", 1 + i, (void *) thash_buf[i]);

		thread_dump(thash_buf[i]);
	}
	putchar('\n');

	free(thash_buf);

	return 0;
}

static errno_t areas_dump(void)
{
	as_area_info_t *ainfo_buf;
	as_area_info_t dummy_buf;
	size_t buf_size, n_areas;

	size_t copied;
	size_t needed;
	size_t i;
	errno_t rc;

	rc = udebug_areas_read(sess, &dummy_buf, 0, &copied, &needed);
	if (rc != EOK) {
		printf("udebug_areas_read() -> %s\n", str_error_name(rc));
		return rc;
	}

	buf_size = needed;
	ainfo_buf = malloc(buf_size);

	rc = udebug_areas_read(sess, ainfo_buf, buf_size, &copied, &needed);
	if (rc != EOK) {
		printf("udebug_areas_read() -> %s\n", str_error_name(rc));
		return rc;
	}

	assert(copied == buf_size);
	assert(needed == buf_size);

	n_areas = copied / sizeof(as_area_info_t);

	printf("Address space areas:\n");
	for (i = 0; i < n_areas; i++) {
		printf(" [%zu] flags: %c%c%c%c base: %p size: %zu\n", 1 + i,
		    (ainfo_buf[i].flags & AS_AREA_READ) ? 'R' : '-',
		    (ainfo_buf[i].flags & AS_AREA_WRITE) ? 'W' : '-',
		    (ainfo_buf[i].flags & AS_AREA_EXEC) ? 'X' : '-',
		    (ainfo_buf[i].flags & AS_AREA_CACHEABLE) ? 'C' : '-',
		    (void *) ainfo_buf[i].start_addr, ainfo_buf[i].size);
	}

	putchar('\n');

	if (write_core_file) {
		printf("Writing core file '%s'\n", core_file_name);

		rc = elf_core_save(core_file_name, ainfo_buf, n_areas, sess,
		    &reg_state);

		if (rc != EOK) {
			printf("Failed writing core file.\n");
			return EIO;
		}
	}

	free(ainfo_buf);

	return 0;
}

errno_t td_stacktrace(uintptr_t fp, uintptr_t pc)
{
	int cnt = 0;
	uintptr_t nfp;
	stacktrace_t st;
	char *sym_pc;
	errno_t rc;

	st.op_arg = NULL;
	st.ops = &td_stacktrace_ops;

	while (cnt++ < STACK_FRAMES_MAX && stacktrace_fp_valid(&st, fp)) {
		sym_pc = fmt_sym_address(pc);
		printf("  %p: %s\n", (void *) fp, sym_pc);
		free(sym_pc);

		rc = stacktrace_ra_get(&st, fp, &pc);
		if (rc != EOK)
			return rc;

		rc = stacktrace_fp_prev(&st, fp, &nfp);
		if (rc != EOK)
			return rc;

		fp = nfp;
	}

	return EOK;
}

static errno_t thread_dump(uintptr_t thash)
{
	istate_t istate;
	uintptr_t pc, fp;
	char *sym_pc;
	errno_t rc;

	rc = udebug_regs_read(sess, thash, &istate);
	if (rc != EOK) {
		printf("Failed reading registers: %s.\n", str_error_name(rc));
		return EIO;
	}

	pc = istate_get_pc(&istate);
	fp = istate_get_fp(&istate);

	/* Save register state for dumping to core file later. */
	reg_state = istate;

	sym_pc = fmt_sym_address(pc);
	printf("Thread %p: PC = %s. FP = %p\n", (void *) thash,
	    sym_pc, (void *) fp);
	free(sym_pc);

	(void) td_stacktrace(fp, pc);

	return EOK;
}

static errno_t td_read_uintptr(void *arg, uintptr_t addr, uintptr_t *value)
{
	uintptr_t data;
	errno_t rc;

	(void) arg;

	rc = udebug_mem_read(sess, &data, addr, sizeof(data));
	if (rc != EOK) {
		printf("Warning: udebug_mem_read() failed.\n");
		return rc;
	}

	*value = data;
	return EOK;
}

/** Attempt to find the right executable file and load the symbol table. */
static void autoload_syms(void)
{
	assert(app_name != NULL);
	assert(app_symtab == NULL);

	if (app_name[0] != '/') {
		printf("Task name is not path. Can't autoload symbol table.\n");
		return;
	}

	errno_t rc = symtab_load(app_name, &app_symtab);
	if (rc != EOK) {
		printf("Failed autoloading symbol table: %s\n",
		    str_error_name(rc));
		return;
	}

	printf("Loaded symbol table from %s\n", app_name);
}

static char *get_app_task_name(void)
{
	char dummy_buf;
	size_t copied, needed, name_size;
	char *name;
	errno_t rc;

	rc = udebug_name_read(sess, &dummy_buf, 0, &copied, &needed);
	if (rc != EOK)
		return NULL;

	name_size = needed;
	name = malloc(name_size + 1);
	rc = udebug_name_read(sess, name, name_size, &copied, &needed);
	if (rc != EOK) {
		free(name);
		return NULL;
	}

	assert(copied == name_size);
	assert(copied == needed);
	name[copied] = '\0';

	return name;
}

/** Format address in symbolic form.
 *
 * Formats address as <symbol_name>+<offset> (<address>), if possible,
 * otherwise as <address>.
 *
 * @param addr	Address to format.
 * @return	Newly allocated string, address in symbolic form.
 */
static char *fmt_sym_address(uintptr_t addr)
{
	char *name;
	size_t offs;
	errno_t rc;
	int ret;
	char *str;

	if (app_symtab != NULL) {
		rc = symtab_addr_to_name(app_symtab, addr, &name, &offs);
	} else {
		rc = ENOTSUP;
	}

	if (rc == EOK) {
		ret = asprintf(&str, "%p (%s+%zu)", (void *) addr, name, offs);
	} else {
		ret = asprintf(&str, "%p", (void *) addr);
	}

	if (ret < 0) {
		printf("Memory allocation error.\n");
		exit(1);
	}

	return str;
}

/** @}
 */
